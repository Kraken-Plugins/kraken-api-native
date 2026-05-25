package com.kraken.ui;

import com.kraken.api.NativeClient;

import javax.swing.BorderFactory;
import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JList;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JSplitPane;
import javax.swing.ListSelectionModel;
import javax.swing.SwingUtilities;
import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;

final class ClientFrame extends JFrame {
    private final LogPanel logPanel = new LogPanel();
    private final GamePanel gamePanel = new GamePanel(this::appendLog);
    private final JLabel statusLabel = new JLabel("Idle");
    private final JButton launchButton = new JButton("Launch");
    private final JButton setTitleButton = new JButton("Set Title");
    private final LauncherController controller;
    private NativeClient nativeClient;

    ClientFrame(AppConfig config) {
        super("Kraken");
        controller = new LauncherController(
            this::appendLog,
            this::onNativeConnected,
            gamePanel::attachToProcess);

        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setMinimumSize(new Dimension(1100, 720));
        setSize(1320, 820);
        setLocationRelativeTo(null);

        JPanel root = new JPanel(new BorderLayout());
        root.setBackground(Colors.BACKGROUND);
        root.add(buildToolbar(config), BorderLayout.NORTH);
        root.add(buildMainContent(), BorderLayout.CENTER);
        setContentPane(root);

        setTitleButton.setEnabled(false);
        appendLog("UI", "Launcher path: " + config.launcherPath());
    }

    private JPanel buildToolbar(AppConfig config) {
        JPanel toolbar = new JPanel(new BorderLayout());
        toolbar.setBackground(Colors.PANEL);
        toolbar.setBorder(BorderFactory.createMatteBorder(0, 0, 1, 0, Colors.BORDER));

        JLabel brand = new JLabel("Kraken");
        brand.setForeground(Colors.TEXT);
        brand.setFont(Fonts.UI_BOLD);
        brand.setBorder(BorderFactory.createEmptyBorder(0, 14, 0, 12));
        toolbar.add(brand, BorderLayout.WEST);

        JPanel actions = new JPanel(new FlowLayout(FlowLayout.LEFT, 8, 8));
        actions.setOpaque(false);

        launchButton.addActionListener(event -> {
            launchButton.setEnabled(false);
            statusLabel.setText("Launching");
            controller.launch(config);
        });

        setTitleButton.addActionListener(event -> {
            if (nativeClient != null) {
                controller.sendSetWindowTitle(nativeClient);
            }
        });

        JButton screenshotButton = new JButton("Screenshot");
        screenshotButton.addActionListener(event ->
            appendLog("UI", "Screenshot button clicked; capture implementation is deferred."));

        JButton discordButton = new JButton("Discord");
        discordButton.addActionListener(event ->
            appendLog("UI", "Discord auth placeholder clicked."));

        actions.add(launchButton);
        actions.add(setTitleButton);
        actions.add(screenshotButton);
        actions.add(discordButton);
        toolbar.add(actions, BorderLayout.CENTER);

        statusLabel.setForeground(Colors.MUTED);
        statusLabel.setFont(Fonts.UI);
        statusLabel.setBorder(BorderFactory.createEmptyBorder(0, 12, 0, 14));
        toolbar.add(statusLabel, BorderLayout.EAST);

        return toolbar;
    }

    private JSplitPane buildMainContent() {
        JPanel center = new JPanel(new BorderLayout());
        center.setBackground(Colors.BACKGROUND);
        center.add(gamePanel, BorderLayout.CENTER);

        JSplitPane verticalSplit = new JSplitPane(
            JSplitPane.VERTICAL_SPLIT,
            center,
            logPanel);
        verticalSplit.setResizeWeight(0.76);
        verticalSplit.setDividerSize(6);
        verticalSplit.setBorder(null);
        verticalSplit.setBackground(Colors.BACKGROUND);

        JSplitPane horizontalSplit = new JSplitPane(
            JSplitPane.HORIZONTAL_SPLIT,
            verticalSplit,
            buildPluginSidebar());
        horizontalSplit.setResizeWeight(1.0);
        horizontalSplit.setDividerSize(6);
        horizontalSplit.setBorder(null);
        horizontalSplit.setBackground(Colors.BACKGROUND);

        return horizontalSplit;
    }

    private JPanel buildPluginSidebar() {
        JPanel sidebar = new JPanel(new BorderLayout());
        sidebar.setPreferredSize(new Dimension(286, 600));
        sidebar.setMinimumSize(new Dimension(250, 400));
        sidebar.setBackground(Colors.PANEL);
        sidebar.setBorder(BorderFactory.createMatteBorder(0, 1, 0, 0, Colors.BORDER));

        JList<String> pluginList = new JList<>(new String[]{"Native Probe"});
        pluginList.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        pluginList.setSelectedIndex(0);
        pluginList.setBackground(Colors.PANEL_ALT);
        pluginList.setForeground(Colors.TEXT);
        pluginList.setFont(Fonts.UI);
        pluginList.setFixedCellHeight(32);
        sidebar.add(new JScrollPane(pluginList), BorderLayout.NORTH);

        JPanel content = new JPanel(new GridBagLayout());
        content.setBackground(Colors.PANEL);
        content.setBorder(BorderFactory.createEmptyBorder(14, 14, 14, 14));

        GridBagConstraints constraints = new GridBagConstraints();
        constraints.gridx = 0;
        constraints.gridy = 0;
        constraints.weightx = 1;
        constraints.fill = GridBagConstraints.HORIZONTAL;
        constraints.insets = new Insets(0, 0, 10, 0);

        JLabel title = new JLabel("Native Probe");
        title.setForeground(Colors.TEXT);
        title.setFont(Fonts.UI_BOLD);
        content.add(title, constraints);

        constraints.gridy++;
        JButton panelSetTitleButton = new JButton("Set Client Title");
        panelSetTitleButton.addActionListener(event -> {
            if (nativeClient != null) {
                controller.sendSetWindowTitle(nativeClient);
            }
        });
        content.add(panelSetTitleButton, constraints);

        constraints.gridy++;
        constraints.weighty = 1;
        content.add(new JPanel(), constraints);

        sidebar.add(content, BorderLayout.CENTER);
        return sidebar;
    }

    private void onNativeConnected(NativeClient nativeClient) {
        this.nativeClient = nativeClient;
        setTitleButton.setEnabled(true);
        statusLabel.setText("Connected");
    }

    private void appendLog(String level, String message) {
        if (!SwingUtilities.isEventDispatchThread()) {
            SwingUtilities.invokeLater(() -> appendLog(level, message));
            return;
        }

        logPanel.append(level, message);
    }
}
