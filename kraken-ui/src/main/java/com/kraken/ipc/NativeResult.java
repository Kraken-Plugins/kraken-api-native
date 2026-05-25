package com.kraken.ipc;

public final class NativeResult {
    private final boolean ok;
    private final String message;

    /**
     * Creates a native command result.
     *
     * @param ok true when the native command succeeded
     * @param message native result detail for logs or UI feedback
     */
    public NativeResult(boolean ok, String message) {
        this.ok = ok;
        this.message = message;
    }

    /**
     * Returns whether the native command succeeded.
     *
     * @return true when the native command succeeded
     */
    public boolean ok() {
        return ok;
    }

    /**
     * Returns the native command result message.
     *
     * @return result detail for logs or UI feedback
     */
    public String message() {
        return message;
    }
}
