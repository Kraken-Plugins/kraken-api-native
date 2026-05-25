package com.kraken.ui;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

final class AppConfig {
    private final Path launcherPath;
    private final Optional<Path> clientPath;
    private final Optional<Path> pluginPath;

    private AppConfig(Path launcherPath,
                      Optional<Path> clientPath,
                      Optional<Path> pluginPath) {
        this.launcherPath = launcherPath;
        this.clientPath = clientPath;
        this.pluginPath = pluginPath;
    }

    Path launcherPath() {
        return launcherPath;
    }

    Optional<Path> clientPath() {
        return clientPath;
    }

    Optional<Path> pluginPath() {
        return pluginPath;
    }

    static AppConfig fromArgs(String[] args) {
        Path launcherPath = resolveDefaultLauncherPath();
        Optional<Path> clientPath = Optional.empty();
        Optional<Path> pluginPath = Optional.empty();

        for (int index = 0; index < args.length; ++index) {
            switch (args[index]) {
                case "--launcher":
                    launcherPath = requirePathValue(args, ++index, "--launcher");
                    break;
                case "--client":
                    clientPath = Optional.of(requirePathValue(args, ++index, "--client"));
                    break;
                case "--plugin":
                    pluginPath = Optional.of(requirePathValue(args, ++index, "--plugin"));
                    break;
                default:
                    throw new IllegalArgumentException("Unknown argument: " + args[index]);
            }
        }

        return new AppConfig(launcherPath, clientPath, pluginPath);
    }

    private static Path requirePathValue(String[] args, int index, String option) {
        if (index >= args.length) {
            throw new IllegalArgumentException(option + " requires a path.");
        }

        return Path.of(args[index]).toAbsolutePath().normalize();
    }

    private static Path resolveDefaultLauncherPath() {
        List<Path> candidates = new ArrayList<>();
        candidates.add(Path.of("cmake-build-debug", "launcher", "Debug", "launcher.exe"));
        candidates.add(Path.of("cmake-build-debug", "launcher", "RelWithDebInfo", "launcher.exe"));
        candidates.add(Path.of("cmake-build-debug", "launcher", "Release", "launcher.exe"));
        candidates.add(Path.of("launcher.exe"));

        for (Path candidate : candidates) {
            Path absolute = candidate.toAbsolutePath().normalize();
            if (Files.exists(absolute)) {
                return absolute;
            }
        }

        return candidates.get(0).toAbsolutePath().normalize();
    }
}
