package com.kraken.ui;

import javax.swing.SwingUtilities;
import javax.swing.UIManager;

public final class KrakenLauncherApp {
    private KrakenLauncherApp() {
    }

    /**
     * Starts the Kraken Swing launcher UI.
     *
     * @param args optional launcher, client, and plugin path arguments
     */
    public static void main(String[] args) {
        AppConfig config;
        try {
            config = AppConfig.fromArgs(args);
        } catch (RuntimeException ex) {
            System.err.println(ex.getMessage());
            return;
        }

        SwingUtilities.invokeLater(() -> {
            UIManager.put("Button.arc", 6);
            UIManager.put("Component.arc", 6);

            ClientFrame frame = new ClientFrame(config);
            frame.setVisible(true);
        });
    }
}
