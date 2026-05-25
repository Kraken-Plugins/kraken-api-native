#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <jawt.h>
#include <jawt_md.h>
#include <jni.h>

#include <cstdint>

namespace {

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

HWND FromJlong(jlong value) {
    return reinterpret_cast<HWND>(static_cast<std::uintptr_t>(value));
}

jlong ToJlong(HWND window) {
    return static_cast<jlong>(reinterpret_cast<std::uintptr_t>(window));
}

} // namespace

extern "C" JNIEXPORT jlong JNICALL
Java_com_kraken_nativewin_Win32Window_getCanvasWindowHandle(
    JNIEnv* env,
    jclass,
    jobject canvas) {
    if (canvas == nullptr) {
        return 0;
    }

    JAWT awt{};
    awt.version = JAWT_VERSION_1_4;
    if (!JAWT_GetAWT(env, &awt)) {
        return 0;
    }

    JAWT_DrawingSurface* surface = awt.GetDrawingSurface(env, canvas);
    if (surface == nullptr) {
        return 0;
    }

    HWND window = nullptr;
    const jint lock = surface->Lock(surface);
    if ((lock & JAWT_LOCK_ERROR) == 0) {
        JAWT_DrawingSurfaceInfo* surfaceInfo =
            surface->GetDrawingSurfaceInfo(surface);

        if (surfaceInfo != nullptr && surfaceInfo->platformInfo != nullptr) {
            const auto* win32Info =
                static_cast<JAWT_Win32DrawingSurfaceInfo*>(
                    surfaceInfo->platformInfo);
            window = win32Info->hwnd;
        }

        if (surfaceInfo != nullptr) {
            surface->FreeDrawingSurfaceInfo(surfaceInfo);
        }

        surface->Unlock(surface);
    }

    awt.FreeDrawingSurface(surface);
    return ToJlong(window);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_kraken_nativewin_Win32Window_findMainWindowByProcessId(
    JNIEnv*,
    jclass,
    jlong processId) {
    MainWindowSearch search;
    search.processId = static_cast<DWORD>(processId);

    EnumWindows(FindMainWindowCallback, reinterpret_cast<LPARAM>(&search));
    return ToJlong(search.window);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_kraken_nativewin_Win32Window_embedWindow(
    JNIEnv*,
    jclass,
    jlong childHwnd,
    jlong parentHwnd) {
    HWND child = FromJlong(childHwnd);
    HWND parent = FromJlong(parentHwnd);
    if (child == nullptr || parent == nullptr) {
        return JNI_FALSE;
    }

    SetLastError(0);
    HWND previousParent = SetParent(child, parent);
    if (previousParent == nullptr && GetLastError() != 0) {
        return JNI_FALSE;
    }

    SetLastError(0);
    LONG_PTR style = GetWindowLongPtrW(child, GWL_STYLE);
    if (style == 0 && GetLastError() != 0) {
        return JNI_FALSE;
    }

    style &= ~(WS_POPUP | WS_CAPTION | WS_THICKFRAME);
    style |= WS_CHILD | WS_VISIBLE;

    if (SetWindowLongPtrW(child, GWL_STYLE, style) == 0 &&
        GetLastError() != 0) {
        return JNI_FALSE;
    }

    SetWindowPos(
        child,
        nullptr,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED |
            SWP_SHOWWINDOW);

    return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_kraken_nativewin_Win32Window_moveWindow(
    JNIEnv*,
    jclass,
    jlong hwnd,
    jint x,
    jint y,
    jint width,
    jint height) {
    HWND window = FromJlong(hwnd);
    if (window == nullptr) {
        return JNI_FALSE;
    }

    return MoveWindow(window, x, y, width, height, TRUE) ? JNI_TRUE : JNI_FALSE;
}
