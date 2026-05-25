package com.kraken.ipc;

import java.util.Map;
import java.util.Optional;

public final class Protocol {
    private Protocol() {
    }

    /**
     * Builds a JSON command line for the native IPC protocol.
     *
     * @param id command correlation id
     * @param name native command name
     * @param fields string command fields to include
     * @return serialized command without the trailing newline
     */
    public static String command(String id, String name, Map<String, String> fields) {
        StringBuilder builder = new StringBuilder();
        builder.append("{\"v\":1,\"type\":\"command\",\"id\":")
            .append(quote(id))
            .append(",\"name\":")
            .append(quote(name));

        for (Map.Entry<String, String> entry : fields.entrySet()) {
            builder.append(',')
                .append(quote(entry.getKey()))
                .append(':')
                .append(quote(entry.getValue()));
        }

        builder.append('}');
        return builder.toString();
    }

    /**
     * Extracts a string field from the small IPC JSON dialect.
     *
     * @param json serialized IPC message
     * @param key field name to read
     * @return decoded field value when present and valid
     */
    public static Optional<String> stringField(String json, String key) {
        String quotedKey = "\"" + key + "\"";
        int position = json.indexOf(quotedKey);
        if (position < 0) {
            return Optional.empty();
        }

        position = json.indexOf(':', position + quotedKey.length());
        if (position < 0) {
            return Optional.empty();
        }

        ++position;
        while (position < json.length() && Character.isWhitespace(json.charAt(position))) {
            ++position;
        }

        if (position >= json.length() || json.charAt(position) != '"') {
            return Optional.empty();
        }

        ++position;
        StringBuilder value = new StringBuilder();
        while (position < json.length()) {
            char character = json.charAt(position++);
            if (character == '"') {
                return Optional.of(value.toString());
            }

            if (character != '\\') {
                value.append(character);
                continue;
            }

            if (position >= json.length()) {
                return Optional.empty();
            }

            char escaped = json.charAt(position++);
            switch (escaped) {
                case '"':
                case '\\':
                case '/':
                    value.append(escaped);
                    break;
                case 'b':
                    value.append('\b');
                    break;
                case 'f':
                    value.append('\f');
                    break;
                case 'n':
                    value.append('\n');
                    break;
                case 'r':
                    value.append('\r');
                    break;
                case 't':
                    value.append('\t');
                    break;
                default:
                    return Optional.empty();
            }
        }

        return Optional.empty();
    }

    /**
     * Extracts a boolean field from the small IPC JSON dialect.
     *
     * @param json serialized IPC message
     * @param key field name to read
     * @return boolean field value when present and valid
     */
    public static Optional<Boolean> booleanField(String json, String key) {
        String quotedKey = "\"" + key + "\"";
        int position = json.indexOf(quotedKey);
        if (position < 0) {
            return Optional.empty();
        }

        position = json.indexOf(':', position + quotedKey.length());
        if (position < 0) {
            return Optional.empty();
        }

        ++position;
        while (position < json.length() && Character.isWhitespace(json.charAt(position))) {
            ++position;
        }

        if (json.startsWith("true", position)) {
            return Optional.of(true);
        }

        if (json.startsWith("false", position)) {
            return Optional.of(false);
        }

        return Optional.empty();
    }

    private static String quote(String value) {
        StringBuilder builder = new StringBuilder("\"");
        for (int index = 0; index < value.length(); ++index) {
            char character = value.charAt(index);
            switch (character) {
                case '\\':
                    builder.append("\\\\");
                    break;
                case '"':
                    builder.append("\\\"");
                    break;
                case '\n':
                    builder.append("\\n");
                    break;
                case '\r':
                    builder.append("\\r");
                    break;
                case '\t':
                    builder.append("\\t");
                    break;
                default:
                    if (character < 0x20) {
                        builder.append(String.format("\\u%04x", (int) character));
                    } else {
                        builder.append(character);
                    }
                    break;
            }
        }

        builder.append('"');
        return builder.toString();
    }
}
