package com.kraken.ipc;

import java.io.Closeable;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.charset.StandardCharsets;
import java.util.Objects;
import java.util.function.Consumer;

public final class PipeClient implements Closeable {
    private final RandomAccessFile pipe;
    private final Consumer<String> lineConsumer;
    private final Consumer<IOException> disconnectConsumer;
    private final Object writeLock = new Object();
    private volatile boolean running = true;
    private Thread readerThread;

    private PipeClient(RandomAccessFile pipe,
                       Consumer<String> lineConsumer,
                       Consumer<IOException> disconnectConsumer) {
        this.pipe = pipe;
        this.lineConsumer = lineConsumer;
        this.disconnectConsumer = disconnectConsumer;
    }

    /**
     * Opens a client connection to an existing Windows named pipe.
     *
     * @param pipeName full named-pipe path such as {@code \\.\pipe\kraken}
     * @param lineConsumer callback for complete newline-delimited messages
     * @param disconnectConsumer callback for unexpected pipe read failures
     * @return connected pipe client
     * @throws IOException when the pipe cannot be opened
     */
    public static PipeClient connect(String pipeName,
                                     Consumer<String> lineConsumer,
                                     Consumer<IOException> disconnectConsumer)
        throws IOException {
        Objects.requireNonNull(pipeName, "pipeName");
        return new PipeClient(
            new RandomAccessFile(pipeName, "rw"),
            lineConsumer,
            disconnectConsumer);
    }

    /**
     * Starts the background reader thread for incoming IPC messages.
     */
    public void start() {
        readerThread = new Thread(this::readLoop, "kraken-pipe-reader");
        readerThread.setDaemon(true);
        readerThread.start();
    }

    /**
     * Writes one newline-delimited IPC message.
     *
     * @param line serialized message without the trailing newline
     * @throws IOException when the pipe write fails
     */
    public void writeLine(String line) throws IOException {
        byte[] bytes = (line + "\n").getBytes(StandardCharsets.UTF_8);
        synchronized (writeLock) {
            pipe.write(bytes);
        }
    }

    /**
     * Closes the named-pipe client.
     *
     * @throws IOException when the underlying pipe close fails
     */
    @Override
    public void close() throws IOException {
        running = false;
        pipe.close();
    }

    private void readLoop() {
        byte[] buffer = new byte[4096];
        StringBuilder pending = new StringBuilder();

        try {
            while (running) {
                int read = pipe.read(buffer);
                if (read < 0) {
                    break;
                }

                String chunk = new String(buffer, 0, read, StandardCharsets.UTF_8);
                pending.append(chunk);

                int newline = pending.indexOf("\n");
                while (newline >= 0) {
                    String line = pending.substring(0, newline);
                    if (line.endsWith("\r")) {
                        line = line.substring(0, line.length() - 1);
                    }

                    if (!line.isBlank()) {
                        lineConsumer.accept(line);
                    }

                    pending.delete(0, newline + 1);
                    newline = pending.indexOf("\n");
                }
            }
        } catch (IOException ex) {
            if (running) {
                disconnectConsumer.accept(ex);
            }
        }
    }
}
