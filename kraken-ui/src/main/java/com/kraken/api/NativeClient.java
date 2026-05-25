package com.kraken.api;

import com.kraken.ipc.CommandBus;
import com.kraken.ipc.NativeResult;

import java.util.Map;
import java.util.Objects;
import java.util.concurrent.CompletableFuture;

public final class NativeClient {
    private final CommandBus commandBus;

    /**
     * Creates the small Java-facing native client API.
     *
     * @param commandBus command bus connected to plugin-core.dll
     */
    public NativeClient(CommandBus commandBus) {
        this.commandBus = Objects.requireNonNull(commandBus, "commandBus");
    }

    /**
     * Requests a title change on the injected native client's main window.
     *
     * @param title new title to apply to the OSRS native window
     * @return asynchronous native command result
     */
    public CompletableFuture<NativeResult> setWindowTitle(String title) {
        return commandBus.send(
            "client.windowTitle.set",
            Map.of("title", Objects.requireNonNull(title, "title")));
    }
}
