package com.kraken.ui;

import com.kraken.nativewin.Win32Window;

import javax.swing.BorderFactory;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.SwingConstants;
import javax.swing.Timer;
import java.awt.BorderLayout;
import java.awt.Canvas;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.event.ComponentAdapter;
import java.awt.event.ComponentEvent;
import java.util.function.BiConsumer;

final class GamePanel extends JPanel {
    private final Canvas canvas = new Canvas();
    private final JLabel statusLabel = new JLabel("Client not launched", SwingConstants.CENTER);
    private final BiConsumer<String, String> logSink;
    private long attachedProcessId;
    private long embeddedWindow;

    GamePanel(BiConsumer<String, String> logSink) {
        super(new BorderLayout());
        this.logSink = logSink;

        setBackground(Color.BLACK);
        setBorder(BorderFactory.createMatteBorder(1, 0, 1, 0, Colors.BORDER));
        statusLabel.setForeground(Colors.MUTED);
        statusLabel.setFont(Fonts.UI);
        add(statusLabel, BorderLayout.CENTER);

        addComponentListener(new ComponentAdapter() {
            @Override
            public void componentResized(ComponentEvent event) {
                resizeEmbeddedWindow();
            }
        });
    }

    void attachToProcess(long processId) {
        if (attachedProcessId == processId) {
            return;
        }

        attachedProcessId = processId;
        statusLabel.setText("Client launched; waiting for window");

        if (!Win32Window.isAvailable()) {
            statusLabel.setText("Game launched outside embedded view");
            logSink.accept("WARN", "kraken-ui-native.dll was not found; window embedding is disabled.");
            return;
        }

        if (statusLabel.getParent() == this) {
            remove(statusLabel);
        }

        canvas.setBackground(Color.BLACK);
        canvas.setPreferredSize(new Dimension(800, 600));
        if (canvas.getParent() != this) {
            add(canvas, BorderLayout.CENTER);
        }

        revalidate();
        repaint();

        Timer timer = new Timer(500, null);
        final int[] attempts = {0};
        timer.addActionListener(event -> {
            attempts[0]++;
            long parent = Win32Window.getCanvasWindowHandle(canvas);
            long child = Win32Window.findMainWindowByProcessId(processId);

            if (parent != 0 && child != 0 && Win32Window.embedWindow(child, parent)) {
                embeddedWindow = child;
                resizeEmbeddedWindow();
                logSink.accept("INFO", "Embedded OSRS window into the Kraken UI.");
                timer.stop();
                return;
            }

            if (attempts[0] >= 60) {
                if (canvas.getParent() == this) {
                    remove(canvas);
                    add(statusLabel, BorderLayout.CENTER);
                    statusLabel.setText("Game launched outside embedded view");
                    revalidate();
                    repaint();
                }

                logSink.accept("WARN", "Timed out waiting for the OSRS window to become embeddable.");
                timer.stop();
            }
        });

        timer.start();
    }

    private void resizeEmbeddedWindow() {
        if (embeddedWindow == 0) {
            return;
        }

        Win32Window.moveWindow(
            embeddedWindow,
            0,
            0,
            Math.max(1, canvas.getWidth()),
            Math.max(1, canvas.getHeight()));
    }
}
