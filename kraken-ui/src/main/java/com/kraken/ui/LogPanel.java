package com.kraken.ui;

import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextPane;
import javax.swing.SwingUtilities;
import javax.swing.text.BadLocationException;
import javax.swing.text.Style;
import javax.swing.text.StyleConstants;
import javax.swing.text.StyledDocument;
import java.awt.BorderLayout;
import java.awt.Color;
import java.time.LocalTime;
import java.time.format.DateTimeFormatter;

final class LogPanel extends JPanel {
    private static final DateTimeFormatter TIME_FORMAT =
        DateTimeFormatter.ofPattern("HH:mm:ss");

    private final JTextPane textPane = new JTextPane();
    private final StyledDocument document = textPane.getStyledDocument();

    LogPanel() {
        super(new BorderLayout());
        setBackground(Colors.PANEL);

        textPane.setEditable(false);
        textPane.setBackground(Colors.CONSOLE);
        textPane.setForeground(Colors.TEXT);
        textPane.setCaretColor(Colors.TEXT);
        textPane.setFont(Fonts.MONO);

        addStyles();
        add(new JScrollPane(textPane), BorderLayout.CENTER);
    }

    void append(String level, String message) {
        if (!SwingUtilities.isEventDispatchThread()) {
            SwingUtilities.invokeLater(() -> append(level, message));
            return;
        }

        Style style = document.getStyle(levelStyle(level));
        String timestamp = LocalTime.now().format(TIME_FORMAT);
        String line = "[" + timestamp + "] [" + level + "] " + message + "\n";

        try {
            document.insertString(document.getLength(), line, style);
            textPane.setCaretPosition(document.getLength());
        } catch (BadLocationException ignored) {
            // JTextPane document positions are internal; ignore impossible append failures.
        }
    }

    private void addStyles() {
        addStyle("INFO", Colors.TEXT);
        addStyle("WARN", Colors.WARN);
        addStyle("ERROR", Colors.ERROR);
        addStyle("LAUNCHER", Colors.ACCENT);
        addStyle("UI", Colors.MUTED);
    }

    private void addStyle(String name, Color color) {
        Style style = textPane.addStyle(name, null);
        StyleConstants.setForeground(style, color);
    }

    private static String levelStyle(String level) {
        switch (level) {
            case "WARN":
                return "WARN";
            case "ERROR":
                return "ERROR";
            case "LAUNCHER":
                return "LAUNCHER";
            case "UI":
                return "UI";
            default:
                return "INFO";
        }
    }
}
