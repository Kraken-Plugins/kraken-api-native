package com.kraken.ui;

import com.kraken.api.NativeClient;
import com.kraken.ipc.CommandBus;
import com.kraken.ipc.PipeClient;
import com.kraken.ipc.Protocol;

import javax.swing.SwingUtilities;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.UUID;
import java.util.function.BiConsumer;
import java.util.function.Consumer;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

final class LauncherController {
    private static final Pattern CLIENT_PID_PATTERN =
        Pattern.compile("\\[kraken:event] client_pid=(\\d+)");

    private final BiConsumer<String, String> logSink;
    private final Consumer<NativeClient> connectedConsumer;
    private final Consumer<Long> clientPidConsumer;
    private CommandBus commandBus;
    private PipeClient pipeClient;
    private Process launcherProcess;

    LauncherController(BiConsumer<String, String> logSink,
                       Consumer<NativeClient> connectedConsumer,
                       Consumer<Long> clientPidConsumer) {
        this.logSink = logSink;
        this.connectedConsumer = connectedConsumer;
        this.clientPidConsumer = clientPidConsumer;
    }

    void launch(AppConfig config) {
        if (launcherProcess != null && launcherProcess.isAlive()) {
            logSink.accept("WARN", "Launcher is already running.");
            return;
        }

        String pipeName = "\\\\.\\pipe\\kraken-api-native-" +
            ProcessHandle.current().pid() + "-" + UUID.randomUUID();

        List<String> command = new ArrayList<>();
        command.add(config.launcherPath().toString());
        command.add("--pipe");
        command.add(pipeName);
        config.clientPath().ifPresent(path -> {
            command.add("--client");
            command.add(path.toString());
        });
        config.pluginPath().ifPresent(path -> {
            command.add("--plugin");
            command.add(path.toString());
        });

        try {
            ProcessBuilder processBuilder = new ProcessBuilder(command);
            processBuilder.redirectErrorStream(true);
            launcherProcess = processBuilder.start();
        } catch (IOException ex) {
            logSink.accept("ERROR", "Failed to start launcher: " + ex.getMessage());
            return;
        }

        logSink.accept("UI", "Started launcher: " + config.launcherPath());
        startLauncherOutputReader(launcherProcess);
        startPipeConnector(pipeName);
    }

    void sendSetWindowTitle(NativeClient nativeClient) {
        nativeClient.setWindowTitle("Kraken attached")
            .thenAccept(result -> {
                String level = result.ok() ? "INFO" : "ERROR";
                logSink.accept(level, "client.windowTitle.set: " + result.message());
            })
            .exceptionally(ex -> {
                logSink.accept("ERROR", "client.windowTitle.set failed: " + ex.getMessage());
                return null;
            });
    }

    private void startLauncherOutputReader(Process process) {
        Thread reader = new Thread(() -> {
            try (BufferedReader bufferedReader = new BufferedReader(
                new InputStreamReader(process.getInputStream(), Charset.defaultCharset()))) {
                String line;
                while ((line = bufferedReader.readLine()) != null) {
                    logSink.accept("LAUNCHER", line);
                    parseClientPid(line).ifPresent(clientPidConsumer);
                }
            } catch (IOException ex) {
                logSink.accept("WARN", "Launcher output reader stopped: " + ex.getMessage());
            }
        }, "kraken-launcher-output");

        reader.setDaemon(true);
        reader.start();
    }

    private void startPipeConnector(String pipeName) {
        Thread connector = new Thread(() -> {
            long deadline = System.currentTimeMillis() + 30_000;
            while (System.currentTimeMillis() < deadline) {
                try {
                    pipeClient = PipeClient.connect(
                        pipeName,
                        this::handlePipeLine,
                        ex -> logSink.accept("WARN", "IPC disconnected: " + ex.getMessage()));
                    commandBus = new CommandBus(pipeClient);
                    pipeClient.start();

                    NativeClient nativeClient = new NativeClient(commandBus);
                    SwingUtilities.invokeLater(() -> connectedConsumer.accept(nativeClient));
                    logSink.accept("INFO", "Connected to plugin-core IPC.");
                    return;
                } catch (IOException ex) {
                    sleep(250);
                }
            }

            logSink.accept("ERROR", "Timed out connecting to plugin-core IPC.");
        }, "kraken-pipe-connector");

        connector.setDaemon(true);
        connector.start();
    }

    private void handlePipeLine(String line) {
        if (commandBus != null && commandBus.handleMessage(line)) {
            return;
        }

        Optional<String> type = Protocol.stringField(line, "type");
        if (type.isPresent() && "log".equals(type.get())) {
            String level = Protocol.stringField(line, "level").orElse("INFO");
            String message = Protocol.stringField(line, "message").orElse("");
            logSink.accept(level, message);
        }
    }

    private static Optional<Long> parseClientPid(String line) {
        Matcher matcher = CLIENT_PID_PATTERN.matcher(line);
        if (!matcher.find()) {
            return Optional.empty();
        }

        return Optional.of(Long.parseLong(matcher.group(1)));
    }

    private static void sleep(long millis) {
        try {
            Thread.sleep(millis);
        } catch (InterruptedException ex) {
            Thread.currentThread().interrupt();
        }
    }
}
