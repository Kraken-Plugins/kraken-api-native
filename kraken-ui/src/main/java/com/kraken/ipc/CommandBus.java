package com.kraken.ipc;

import java.io.IOException;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.atomic.AtomicLong;

public final class CommandBus {
    private final PipeClient pipeClient;
    private final ExecutorService writer = Executors.newSingleThreadExecutor(runnable -> {
        Thread thread = new Thread(runnable, "kraken-command-writer");
        thread.setDaemon(true);
        return thread;
    });
    private final AtomicLong nextId = new AtomicLong(1);
    private final ConcurrentHashMap<String, CompletableFuture<NativeResult>> pending =
        new ConcurrentHashMap<>();

    /**
     * Creates a command bus over an established named-pipe client.
     *
     * @param pipeClient connected pipe client
     */
    public CommandBus(PipeClient pipeClient) {
        this.pipeClient = pipeClient;
    }

    /**
     * Sends a command to the injected native plugin core.
     *
     * @param name native command name
     * @param fields string command fields
     * @return future completed by the matching native result message
     */
    public CompletableFuture<NativeResult> send(String name, Map<String, String> fields) {
        String id = Long.toString(nextId.getAndIncrement());
        CompletableFuture<NativeResult> future = new CompletableFuture<>();
        pending.put(id, future);

        writer.execute(() -> {
            try {
                pipeClient.writeLine(Protocol.command(id, name, fields));
            } catch (IOException ex) {
                pending.remove(id);
                future.completeExceptionally(ex);
            }
        });

        return future;
    }

    /**
     * Handles a native IPC message if it is a command result.
     *
     * @param line serialized IPC message
     * @return true when the message was consumed as a result
     */
    public boolean handleMessage(String line) {
        Optional<String> type = Protocol.stringField(line, "type");
        if (type.isEmpty() || !"result".equals(type.get())) {
            return false;
        }

        Optional<String> id = Protocol.stringField(line, "id");
        if (id.isEmpty()) {
            return true;
        }

        CompletableFuture<NativeResult> future = pending.remove(id.get());
        if (future == null) {
            return true;
        }

        boolean ok = Protocol.booleanField(line, "ok").orElse(false);
        String message = Protocol.stringField(line, "message").orElse("");
        future.complete(new NativeResult(ok, message));
        return true;
    }
}
