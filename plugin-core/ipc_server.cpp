#include "ipc_server.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include "logger.hpp"

#include <array>
#include <cctype>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace kraken::plugin {
namespace {

constexpr DWORD kPipeBufferBytes = 64 * 1024;
constexpr std::wstring_view kPipeNameEnvironment = L"KRAKEN_PIPE_NAME";

std::string JsonEscape(std::string_view value) {
    std::ostringstream stream;

    for (const unsigned char character : value) {
        switch (character) {
            case '\\':
                stream << "\\\\";
                break;
            case '"':
                stream << "\\\"";
                break;
            case '\n':
                stream << "\\n";
                break;
            case '\r':
                stream << "\\r";
                break;
            case '\t':
                stream << "\\t";
                break;
            default:
                if (character < 0x20) {
                    stream << "\\u"
                           << std::hex << std::uppercase << std::setw(4)
                           << std::setfill('0')
                           << static_cast<int>(character)
                           << std::dec << std::nouppercase;
                } else {
                    stream << static_cast<char>(character);
                }
                break;
        }
    }

    return stream.str();
}

std::optional<std::string> ExtractJsonString(std::string_view json,
                                             std::string_view key) {
    const std::string quotedKey = "\"" + std::string(key) + "\"";
    std::size_t position = json.find(quotedKey);
    if (position == std::string_view::npos) {
        return std::nullopt;
    }

    position = json.find(':', position + quotedKey.size());
    if (position == std::string_view::npos) {
        return std::nullopt;
    }

    ++position;
    while (position < json.size() &&
           std::isspace(static_cast<unsigned char>(json[position])) != 0) {
        ++position;
    }

    if (position >= json.size() || json[position] != '"') {
        return std::nullopt;
    }

    ++position;
    std::string value;
    while (position < json.size()) {
        const char character = json[position++];
        if (character == '"') {
            return value;
        }

        if (character != '\\') {
            value.push_back(character);
            continue;
        }

        if (position >= json.size()) {
            return std::nullopt;
        }

        const char escaped = json[position++];
        switch (escaped) {
            case '"':
            case '\\':
            case '/':
                value.push_back(escaped);
                break;
            case 'b':
                value.push_back('\b');
                break;
            case 'f':
                value.push_back('\f');
                break;
            case 'n':
                value.push_back('\n');
                break;
            case 'r':
                value.push_back('\r');
                break;
            case 't':
                value.push_back('\t');
                break;
            default:
                return std::nullopt;
        }
    }

    return std::nullopt;
}

std::wstring Utf8ToWide(std::string_view value) {
    if (value.empty()) {
        return {};
    }

    const int required = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        value.data(),
        static_cast<int>(value.size()),
        nullptr,
        0);

    if (required <= 0) {
        return {};
    }

    std::wstring wide(static_cast<std::size_t>(required), L'\0');
    const int written = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        value.data(),
        static_cast<int>(value.size()),
        wide.data(),
        required);

    if (written <= 0) {
        return {};
    }

    return wide;
}

struct MainWindowSearch {
    DWORD processId = 0;
    HWND window = nullptr;
};

BOOL CALLBACK FindMainWindowCallback(HWND window, LPARAM parameter) {
    auto* search = reinterpret_cast<MainWindowSearch*>(parameter);

    DWORD processId = 0;
    GetWindowThreadProcessId(window, &processId);
    if (processId != search->processId) {
        return TRUE;
    }

    if (!IsWindowVisible(window) || GetWindow(window, GW_OWNER) != nullptr) {
        return TRUE;
    }

    search->window = window;
    return FALSE;
}

HWND FindCurrentProcessMainWindow() {
    MainWindowSearch search;
    search.processId = GetCurrentProcessId();

    EnumWindows(FindMainWindowCallback, reinterpret_cast<LPARAM>(&search));
    return search.window;
}

bool SetCurrentProcessWindowTitle(std::string_view title,
                                  std::string& errorMessage) {
    HWND window = FindCurrentProcessMainWindow();
    if (window == nullptr) {
        errorMessage = "Could not find the current process main window.";
        return false;
    }

    const std::wstring wideTitle = Utf8ToWide(title);
    if (!title.empty() && wideTitle.empty()) {
        errorMessage = "Window title was not valid UTF-8.";
        return false;
    }

    DWORD_PTR result = 0;
    if (!SendMessageTimeoutW(
            window,
            WM_SETTEXT,
            0,
            reinterpret_cast<LPARAM>(wideTitle.c_str()),
            SMTO_ABORTIFHUNG | SMTO_BLOCK,
            1000,
            &result)) {
        errorMessage = "WM_SETTEXT timed out or failed with Win32 error " +
                       std::to_string(GetLastError());
        return false;
    }

    return true;
}

} // namespace

IpcServer::~IpcServer() {
    Stop();
}

bool IpcServer::Start(std::wstring pipeName) {
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true)) {
        return false;
    }

    worker_ = std::thread(&IpcServer::Run, this, std::move(pipeName));
    return true;
}

void IpcServer::Stop() {
    const bool wasRunning = running_.exchange(false);

    HANDLE pipe = GetPipeHandle();
    if (wasRunning && pipe != INVALID_HANDLE_VALUE) {
        CancelIoEx(pipe, nullptr);
        DisconnectNamedPipe(pipe);
    }

    if (worker_.joinable() &&
        worker_.get_id() != std::this_thread::get_id()) {
        worker_.join();
    }
}

void IpcServer::Run(std::wstring pipeName) {
    HANDLE pipe = CreateNamedPipeW(
        pipeName.c_str(),
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        1,
        kPipeBufferBytes,
        kPipeBufferBytes,
        0,
        nullptr);

    if (pipe == INVALID_HANDLE_VALUE) {
        LogError("CreateNamedPipeW failed with Win32 error " +
                 std::to_string(GetLastError()));
        running_ = false;
        return;
    }

    SetPipeHandle(pipe);
    LogInfo("IPC named pipe server waiting for the UI.");

    const BOOL connected = ConnectNamedPipe(pipe, nullptr)
        ? TRUE
        : GetLastError() == ERROR_PIPE_CONNECTED;

    if (!connected) {
        LogError("ConnectNamedPipe failed with Win32 error " +
                 std::to_string(GetLastError()));
        SetPipeHandle(INVALID_HANDLE_VALUE);
        CloseHandle(pipe);
        running_ = false;
        return;
    }

    LogInfo("IPC UI connected.");
    SendClientProcessEvent();
    SetLogSink([this](const LogRecord& record) {
        SendLog(record);
    });

    std::string pending;
    std::array<char, 4096> buffer{};

    while (running_) {
        DWORD bytesRead = 0;
        const BOOL read = ReadFile(
            pipe,
            buffer.data(),
            static_cast<DWORD>(buffer.size()),
            &bytesRead,
            nullptr);

        if (!read || bytesRead == 0) {
            break;
        }

        pending.append(buffer.data(), bytesRead);

        std::size_t newline = pending.find('\n');
        while (newline != std::string::npos) {
            std::string line = pending.substr(0, newline);
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            if (!line.empty()) {
                HandleLine(line);
            }

            pending.erase(0, newline + 1);
            newline = pending.find('\n');
        }
    }

    SetLogSink(nullptr);
    SetPipeHandle(INVALID_HANDLE_VALUE);
    CloseHandle(pipe);
    running_ = false;
    LogWarn("IPC UI disconnected.");
}

void IpcServer::HandleLine(std::string_view line) {
    const std::optional<std::string> type = ExtractJsonString(line, "type");
    if (!type || *type != "command") {
        LogWarn("Ignoring IPC message with an unsupported type.");
        return;
    }

    const std::optional<std::string> id = ExtractJsonString(line, "id");
    const std::optional<std::string> name = ExtractJsonString(line, "name");

    if (!id || id->empty() || !name || name->empty()) {
        SendResult(id.value_or(""), false, "Command was missing id or name.");
        return;
    }

    if (*name != "client.windowTitle.set") {
        SendResult(*id, false, "Unsupported command: " + *name);
        return;
    }

    const std::optional<std::string> title = ExtractJsonString(line, "title");
    if (!title) {
        SendResult(*id, false, "client.windowTitle.set requires title.");
        return;
    }

    std::string errorMessage;
    if (!SetCurrentProcessWindowTitle(*title, errorMessage)) {
        SendResult(*id, false, errorMessage);
        return;
    }

    LogInfo("Window title updated through IPC command.");
    SendResult(*id, true, "Window title updated.");
}

void IpcServer::SendLog(const LogRecord& record) {
    std::ostringstream stream;
    stream << "{\"v\":1,\"type\":\"log\",\"level\":\""
           << LogLevelName(record.level)
           << "\",\"message\":\""
           << JsonEscape(record.message)
           << "\"}\n";

    SendLine(stream.str());
}

void IpcServer::SendClientProcessEvent() {
    std::ostringstream stream;
    stream << "{\"v\":1,\"type\":\"client\",\"pid\":\""
           << GetCurrentProcessId()
           << "\"}\n";

    SendLine(stream.str());
}

void IpcServer::SendResult(std::string_view id,
                           bool ok,
                           std::string_view message) {
    std::ostringstream stream;
    stream << "{\"v\":1,\"type\":\"result\",\"id\":\""
           << JsonEscape(id)
           << "\",\"ok\":"
           << (ok ? "true" : "false")
           << ",\"message\":\""
           << JsonEscape(message)
           << "\"}\n";

    SendLine(stream.str());
}

bool IpcServer::SendLine(std::string_view line) {
    HANDLE pipe = GetPipeHandle();
    if (pipe == INVALID_HANDLE_VALUE) {
        return false;
    }

    std::lock_guard lock(writeMutex_);

    DWORD bytesWritten = 0;
    return WriteFile(
        pipe,
        line.data(),
        static_cast<DWORD>(line.size()),
        &bytesWritten,
        nullptr) &&
        bytesWritten == line.size();
}

HANDLE IpcServer::GetPipeHandle() {
    std::lock_guard lock(pipeMutex_);
    return pipe_;
}

void IpcServer::SetPipeHandle(HANDLE pipe) {
    std::lock_guard lock(pipeMutex_);
    pipe_ = pipe;
}

std::wstring ResolveConfiguredPipeName() {
    DWORD required = GetEnvironmentVariableW(
        kPipeNameEnvironment.data(),
        nullptr,
        0);

    if (required == 0) {
        return {};
    }

    std::vector<wchar_t> buffer(required);
    const DWORD written = GetEnvironmentVariableW(
        kPipeNameEnvironment.data(),
        buffer.data(),
        static_cast<DWORD>(buffer.size()));

    if (written == 0 || written >= buffer.size()) {
        return {};
    }

    return std::wstring(buffer.data(), written);
}

} // namespace kraken::plugin
