package com.kraken.nativewin;

import java.awt.Canvas;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

public final class Win32Window {
    private static final boolean AVAILABLE = loadNativeLibrary();

    private Win32Window() {
    }

    /**
     * Returns whether the optional Win32 JNI helper was loaded.
     *
     * @return true when native HWND helpers are available
     */
    public static boolean isAvailable() {
        return AVAILABLE;
    }

    /**
     * Returns the native HWND owned by an AWT Canvas.
     *
     * @param canvas realized AWT canvas
     * @return HWND as an unsigned pointer value, or zero if unavailable
     */
    public static native long getCanvasWindowHandle(Canvas canvas);

    /**
     * Finds the first visible top-level window for a process.
     *
     * @param processId target process id
     * @return HWND as an unsigned pointer value, or zero if no window is found
     */
    public static native long findMainWindowByProcessId(long processId);

    /**
     * Reparents a child window into a new parent window.
     *
     * @param childHwnd child HWND
     * @param parentHwnd parent HWND
     * @return true when SetParent and style updates succeed
     */
    public static native boolean embedWindow(long childHwnd, long parentHwnd);

    /**
     * Moves and resizes a native window.
     *
     * @param hwnd target HWND
     * @param x left coordinate inside the parent
     * @param y top coordinate inside the parent
     * @param width target width
     * @param height target height
     * @return true when MoveWindow succeeds
     */
    public static native boolean moveWindow(long hwnd,
                                            int x,
                                            int y,
                                            int width,
                                            int height);

    private static boolean loadNativeLibrary() {
        String explicitPath = System.getProperty("kraken.ui.native");
        try {
            if (explicitPath != null && !explicitPath.isBlank()) {
                System.load(Path.of(explicitPath).toAbsolutePath().toString());
                return true;
            }

            for (Path candidate : nativeLibraryCandidates()) {
                if (Files.exists(candidate)) {
                    System.load(candidate.toAbsolutePath().toString());
                    return true;
                }
            }

            System.loadLibrary("kraken-ui-native");
            return true;
        } catch (UnsatisfiedLinkError ex) {
            return false;
        }
    }

    private static List<Path> nativeLibraryCandidates() {
        List<Path> candidates = new ArrayList<>();
        candidates.add(Path.of("cmake-build-debug", "ui-native", "Debug", "kraken-ui-native.dll"));
        candidates.add(Path.of("cmake-build-debug", "ui-native", "RelWithDebInfo", "kraken-ui-native.dll"));
        candidates.add(Path.of("cmake-build-debug", "ui-native", "Release", "kraken-ui-native.dll"));
        candidates.add(Path.of("kraken-ui-native.dll"));
        return candidates;
    }
}
