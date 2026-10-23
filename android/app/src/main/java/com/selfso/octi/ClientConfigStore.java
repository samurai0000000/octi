/*
 * ClientConfigStore.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.AtomicMoveNotSupportedException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.nio.file.StandardOpenOption;
import java.nio.file.attribute.PosixFilePermission;
import java.nio.file.attribute.PosixFilePermissions;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;

/**
 * Persists local session credentials and tabletop UI preferences with strict
 * 0700 directory / 0600 file permissions.
 *
 * <p>Hard-coded endpoint invariant: the server address is permanently compiled
 * as {@link OctiDefs#OCTI_SERVER_HOST} ({@code "trumpet.selfso.com"}) and
 * {@link OctiDefs#OCTI_SERVER_PORT} ({@code 6874}). Any {@code server_url},
 * {@code server_host}, {@code server_port}, {@code host}, or {@code port}
 * fields injected into the JSON file are explicitly ignored on load and never
 * written on save.
 */
public final class ClientConfigStore {

    public static final int MAX_CONFIG_FILE_BYTES = 65536;

    public static final class ClientPreferences {
        public String sessionToken = "";
        public String playerUuid = "";
        public String handle = "";
        public String displayName = "";
        public String avatarUrl = "";
        public String preferredProvider = "";
        public OctiThemeId theme = OctiThemeId.OBSIDIAN_BRASS;
        public boolean confirmAllMoves = false;
        public boolean sfxMuted = false;
        public boolean volumeKeyPtt = true;
        public int pttKey = 'V';

        public ClientPreferences() {
        }

        public String getSessionToken() {
            return sessionToken;
        }

        public void setSessionToken(String sessionToken) {
            this.sessionToken = sessionToken != null ? sessionToken : "";
        }

        public String getPlayerUuid() {
            return playerUuid;
        }

        public void setPlayerUuid(String playerUuid) {
            this.playerUuid = playerUuid != null ? playerUuid : "";
        }

        public String getHandle() {
            return handle;
        }

        public void setHandle(String handle) {
            this.handle = handle != null ? handle : "";
        }

        public String getDisplayName() {
            return displayName;
        }

        public void setDisplayName(String displayName) {
            this.displayName = displayName != null ? displayName : "";
        }

        public String getAvatarUrl() {
            return avatarUrl;
        }

        public void setAvatarUrl(String avatarUrl) {
            this.avatarUrl = avatarUrl != null ? avatarUrl : "";
        }

        public String getPreferredProvider() {
            return preferredProvider;
        }

        public void setPreferredProvider(String preferredProvider) {
            this.preferredProvider = preferredProvider != null ? preferredProvider : "";
        }

        public OctiThemeId getTheme() {
            return theme != null ? theme : OctiThemeId.OBSIDIAN_BRASS;
        }

        public void setTheme(OctiThemeId theme) {
            this.theme = theme != null ? theme : OctiThemeId.OBSIDIAN_BRASS;
        }

        public boolean isConfirmAllMoves() {
            return confirmAllMoves;
        }

        public void setConfirmAllMoves(boolean confirmAllMoves) {
            this.confirmAllMoves = confirmAllMoves;
        }

        public boolean isSfxMuted() {
            return sfxMuted;
        }

        public void setSfxMuted(boolean sfxMuted) {
            this.sfxMuted = sfxMuted;
        }

        public boolean isVolumeKeyPtt() {
            return volumeKeyPtt;
        }

        public void setVolumeKeyPtt(boolean volumeKeyPtt) {
            this.volumeKeyPtt = volumeKeyPtt;
        }

        public int getPttKey() {
            return pttKey;
        }

        public void setPttKey(int pttKey) {
            if (pttKey >= 32 && pttKey <= 126) {
                this.pttKey = pttKey;
            }
        }

        public ClientPreferences copy() {
            ClientPreferences c = new ClientPreferences();
            c.sessionToken = this.sessionToken;
            c.playerUuid = this.playerUuid;
            c.handle = this.handle;
            c.displayName = this.displayName;
            c.avatarUrl = this.avatarUrl;
            c.preferredProvider = this.preferredProvider;
            c.theme = this.theme;
            c.confirmAllMoves = this.confirmAllMoves;
            c.sfxMuted = this.sfxMuted;
            c.volumeKeyPtt = this.volumeKeyPtt;
            c.pttKey = this.pttKey;
            return c;
        }
    }

    private final String _configFilePath;
    private ClientPreferences _prefs;
    private String _lastError = "";

    public ClientConfigStore() {
        this(defaultConfigFilePath());
    }

    public ClientConfigStore(String configFilePath) {
        this._configFilePath = (configFilePath == null || configFilePath.isEmpty())
                ? defaultConfigFilePath()
                : configFilePath;
        this._prefs = new ClientPreferences();
    }

    public ClientConfigStore(File configFile) {
        this(configFile != null ? configFile.getAbsolutePath() : defaultConfigFilePath());
    }

    public ClientConfigStore(Path configPath) {
        this(configPath != null ? configPath.toAbsolutePath().toString() : defaultConfigFilePath());
    }

    public static File configFileForDirectory(File baseDir) {
        if (baseDir == null) {
            return defaultConfigFile();
        }
        return new File(new File(baseDir, "octi"), "client_config.json");
    }

    public static String defaultConfigFilePath() {
        String xdg = System.getenv("XDG_CONFIG_HOME");
        if (xdg != null && !xdg.isEmpty()) {
            return new File(new File(xdg, "octi"), "client_config.json").getPath();
        }
        String home = System.getProperty("user.home");
        if (home == null || home.isEmpty()) {
            home = System.getenv("HOME");
        }
        if (home != null && !home.isEmpty()) {
            return new File(new File(new File(home, ".config"), "octi"), "client_config.json").getPath();
        }
        return "octi_client_config.json";
    }

    public static File defaultConfigFile() {
        return new File(defaultConfigFilePath());
    }

    public static String serverHost() {
        return OctiDefs.OCTI_SERVER_HOST;
    }

    public static int serverPort() {
        return OctiDefs.OCTI_SERVER_PORT;
    }

    public static boolean containsForbiddenServerOverride(String rawJson) {
        if (rawJson == null || rawJson.isEmpty()) {
            return false;
        }
        try {
            Map<String, Object> doc = OctiMiniJson.parseObject(rawJson);
            if (doc == null) {
                return false;
            }
            return doc.containsKey("server_url") ||
                    doc.containsKey("server_host") ||
                    doc.containsKey("server_port") ||
                    doc.containsKey("host") ||
                    doc.containsKey("port");
        } catch (RuntimeException e) {
            return false;
        }
    }

    public String configFilePath() {
        return _configFilePath;
    }

    public File configFile() {
        return new File(_configFilePath);
    }

    public ClientPreferences preferences() {
        return _prefs;
    }

    public ClientPreferences getPreferences() {
        return _prefs;
    }

    public String lastError() {
        return _lastError;
    }

    public String getLastError() {
        return _lastError;
    }

    public void clearSession() {
        _prefs.sessionToken = "";
        _prefs.playerUuid = "";
    }

    public boolean load() {
        StringBuilder err = new StringBuilder();
        boolean ok = load(err);
        _lastError = err.toString();
        return ok;
    }

    public boolean load(StringBuilder errorOut) {
        if (errorOut != null) {
            errorOut.setLength(0);
        }
        _lastError = "";
        if (_configFilePath == null || _configFilePath.isEmpty()) {
            setError(errorOut, "empty config file path");
            return false;
        }

        File file = new File(_configFilePath);
        Path path = file.toPath();
        if (!Files.exists(path)) {
            setError(errorOut, "config file does not exist");
            return false;
        }
        if (!Files.isRegularFile(path)) {
            setError(errorOut, "config path is not a regular file");
            return false;
        }

        long fileSize;
        try {
            fileSize = Files.size(path);
        } catch (IOException e) {
            setError(errorOut, "cannot stat config file: " + e.getMessage());
            return false;
        }
        if (fileSize <= 0) {
            setError(errorOut, "config file is empty");
            return false;
        }
        if (fileSize > MAX_CONFIG_FILE_BYTES) {
            setError(errorOut, "config file exceeds maximum allowed size (64 KiB)");
            return false;
        }

        enforceFile0600(file, path);

        byte[] rawBytes;
        try {
            rawBytes = Files.readAllBytes(path);
        } catch (IOException e) {
            setError(errorOut, "unable to read config file: " + e.getMessage());
            return false;
        }
        if (rawBytes.length == 0) {
            setError(errorOut, "config file is empty");
            return false;
        }
        if (rawBytes.length > MAX_CONFIG_FILE_BYTES) {
            setError(errorOut, "config file exceeds maximum allowed size (64 KiB)");
            return false;
        }

        String raw = new String(rawBytes, StandardCharsets.UTF_8);
        Map<String, Object> doc;
        try {
            doc = OctiMiniJson.parseObject(raw);
        } catch (IllegalArgumentException e) {
            setError(errorOut, "config file does not contain a valid JSON object");
            return false;
        }
        if (doc == null) {
            setError(errorOut, "config file does not contain a valid JSON object");
            return false;
        }

        /*
         * Explicitly strip and ignore any server_url, server_host, server_port,
         * host, or port keys that may have been manually injected into the JSON
         * file.
         */
        doc.remove("server_url");
        doc.remove("server_host");
        doc.remove("server_port");
        doc.remove("host");
        doc.remove("port");

        ClientPreferences parsed = new ClientPreferences();
        if (doc.get("session_token") instanceof String s) {
            parsed.sessionToken = s;
        }
        if (doc.get("player_uuid") instanceof String s) {
            parsed.playerUuid = s;
        }
        if (doc.get("handle") instanceof String s) {
            parsed.handle = s;
        }
        if (doc.get("display_name") instanceof String s) {
            parsed.displayName = s;
        }
        if (doc.get("avatar_url") instanceof String s) {
            parsed.avatarUrl = s;
        }
        if (doc.get("preferred_provider") instanceof String s) {
            parsed.preferredProvider = s;
        }
        if (doc.containsKey("theme")) {
            parsed.theme = parseTheme(doc.get("theme"));
        }
        if (doc.get("confirm_all_moves") instanceof Boolean b) {
            parsed.confirmAllMoves = b;
        }
        if (doc.get("sfx_muted") instanceof Boolean b) {
            parsed.sfxMuted = b;
        }
        if (doc.get("volume_key_ptt") instanceof Boolean b) {
            parsed.volumeKeyPtt = b;
        }
        Object keyObj = doc.containsKey("ptt_keycode") ? doc.get("ptt_keycode") : doc.get("ptt_key");
        if (keyObj instanceof Number n) {
            int key = n.intValue();
            if (key >= 32 && key <= 126) {
                parsed.pttKey = key;
            }
        }

        _prefs = parsed;
        return true;
    }

    public boolean save() {
        StringBuilder err = new StringBuilder();
        boolean ok = save(err);
        _lastError = err.toString();
        return ok;
    }

    public boolean save(StringBuilder errorOut) {
        if (errorOut != null) {
            errorOut.setLength(0);
        }
        _lastError = "";
        if (_configFilePath == null || _configFilePath.isEmpty()) {
            setError(errorOut, "empty config file path");
            return false;
        }

        File targetFile = new File(_configFilePath);
        Path targetPath = targetFile.toPath();
        File parentDir = targetFile.getParentFile();
        if (parentDir != null) {
            if (!ensureDirectory0700(parentDir, errorOut)) {
                return false;
            }
        }

        /*
         * Never write server_url, server_host, or server_port to disk.
         */
        Map<String, Object> doc = new LinkedHashMap<>();
        doc.put("session_token", _prefs.sessionToken != null ? _prefs.sessionToken : "");
        doc.put("player_uuid", _prefs.playerUuid != null ? _prefs.playerUuid : "");
        doc.put("handle", _prefs.handle != null ? _prefs.handle : "");
        doc.put("display_name", _prefs.displayName != null ? _prefs.displayName : "");
        doc.put("avatar_url", _prefs.avatarUrl != null ? _prefs.avatarUrl : "");
        doc.put("preferred_provider", _prefs.preferredProvider != null ? _prefs.preferredProvider : "");
        doc.put("theme", themeToString(_prefs.theme));
        doc.put("confirm_all_moves", _prefs.confirmAllMoves);
        doc.put("sfx_muted", _prefs.sfxMuted);
        doc.put("volume_key_ptt", _prefs.volumeKeyPtt);
        doc.put("ptt_keycode", _prefs.pttKey);

        String serialized = OctiMiniJson.stringifyPretty(doc) + "\n";
        byte[] bytes = serialized.getBytes(StandardCharsets.UTF_8);

        File tmpFile = new File(_configFilePath + ".tmp");
        Path tmpPath = tmpFile.toPath();
        try {
            Files.deleteIfExists(tmpPath);
            try {
                Set<PosixFilePermission> perms0600 = PosixFilePermissions.fromString("rw-------");
                Files.createFile(tmpPath, PosixFilePermissions.asFileAttribute(perms0600));
            } catch (UnsupportedOperationException e) {
                Files.createFile(tmpPath);
            }
            enforceFile0600(tmpFile, tmpPath);
            Files.write(tmpPath, bytes, StandardOpenOption.WRITE, StandardOpenOption.TRUNCATE_EXISTING);
            enforceFile0600(tmpFile, tmpPath);

            try {
                Files.move(tmpPath, targetPath,
                        StandardCopyOption.REPLACE_EXISTING,
                        StandardCopyOption.ATOMIC_MOVE);
            } catch (AtomicMoveNotSupportedException e) {
                Files.move(tmpPath, targetPath, StandardCopyOption.REPLACE_EXISTING);
            }
            enforceFile0600(targetFile, targetPath);
            return true;
        } catch (IOException e) {
            try {
                Files.deleteIfExists(tmpPath);
            } catch (IOException ignored) {
            }
            setError(errorOut, "failed to save config file: " + e.getMessage());
            return false;
        }
    }

    private static String themeToString(OctiThemeId theme) {
        if (theme == OctiThemeId.ROSEWOOD_IVORY) {
            return "rosewood_ivory";
        }
        return "obsidian_brass";
    }

    private static OctiThemeId parseTheme(Object val) {
        if (val instanceof String s) {
            String trimmed = s.trim();
            if ("rosewood_ivory".equalsIgnoreCase(trimmed) || "1".equals(trimmed)) {
                return OctiThemeId.ROSEWOOD_IVORY;
            }
            return OctiThemeId.OBSIDIAN_BRASS;
        }
        if (val instanceof Number n) {
            if (n.intValue() == 1) {
                return OctiThemeId.ROSEWOOD_IVORY;
            }
        }
        return OctiThemeId.OBSIDIAN_BRASS;
    }

    private void setError(StringBuilder errorOut, String msg) {
        _lastError = msg != null ? msg : "";
        if (errorOut != null) {
            errorOut.setLength(0);
            errorOut.append(_lastError);
        }
    }

    private boolean ensureDirectory0700(File dir, StringBuilder errorOut) {
        Path dirPath = dir.toPath();
        try {
            if (!Files.exists(dirPath)) {
                try {
                    Set<PosixFilePermission> perms0700 = PosixFilePermissions.fromString("rwx------");
                    Files.createDirectories(dirPath, PosixFilePermissions.asFileAttribute(perms0700));
                } catch (UnsupportedOperationException e) {
                    Files.createDirectories(dirPath);
                }
            }
            if (!Files.isDirectory(dirPath)) {
                setError(errorOut, "config parent path is not a directory");
                return false;
            }
            try {
                Files.setPosixFilePermissions(dirPath, PosixFilePermissions.fromString("rwx------"));
            } catch (UnsupportedOperationException | IOException ignored) {
            }
            dir.setReadable(false, false);
            dir.setReadable(true, true);
            dir.setWritable(false, false);
            dir.setWritable(true, true);
            dir.setExecutable(false, false);
            dir.setExecutable(true, true);
            return true;
        } catch (IOException e) {
            setError(errorOut, "failed to create config directory: " + e.getMessage());
            return false;
        }
    }

    private static void enforceFile0600(File file, Path path) {
        try {
            Files.setPosixFilePermissions(path, PosixFilePermissions.fromString("rw-------"));
        } catch (UnsupportedOperationException | IOException ignored) {
        }
        file.setReadable(false, false);
        file.setReadable(true, true);
        file.setWritable(false, false);
        file.setWritable(true, true);
        file.setExecutable(false, false);
    }
}

/**
 * Self-contained RFC 8259 JSON parser and serializer for package
 * {@code com.selfso.octi}, avoiding Android unit-test stub exceptions from
 * {@code org.json.JSONObject}.
 */
final class OctiMiniJson {

    private final String src;
    private int pos;

    private OctiMiniJson(String src) {
        this.src = src != null ? src : "";
        this.pos = 0;
    }

    static Object parse(String json) {
        if (json == null) {
            throw new IllegalArgumentException("null JSON input");
        }
        OctiMiniJson p = new OctiMiniJson(json);
        p.skipWhitespace();
        if (p.pos >= p.src.length()) {
            throw new IllegalArgumentException("empty JSON input");
        }
        Object value = p.readValue();
        p.skipWhitespace();
        if (p.pos != p.src.length()) {
            throw new IllegalArgumentException("trailing characters after JSON value at index " + p.pos);
        }
        return value;
    }

    @SuppressWarnings("unchecked")
    static Map<String, Object> parseObject(String json) {
        Object v = parse(json);
        if (!(v instanceof Map<?, ?>)) {
            throw new IllegalArgumentException("JSON root is not an object");
        }
        return (Map<String, Object>) v;
    }

    @SuppressWarnings("unchecked")
    static Map<String, Object> parseObjectOrNull(String json) {
        if (json == null || json.isEmpty()) {
            return null;
        }
        try {
            Object v = parse(json);
            if (v instanceof Map<?, ?>) {
                return (Map<String, Object>) v;
            }
        } catch (RuntimeException ignored) {
        }
        return null;
    }

    static String getString(Map<String, Object> obj, String key, String defaultValue) {
        if (obj == null || key == null) {
            return defaultValue;
        }
        Object v = obj.get(key);
        if (v instanceof String s) {
            return s;
        }
        return defaultValue;
    }

    static long getLong(Map<String, Object> obj, String key, long defaultValue) {
        if (obj == null || key == null) {
            return defaultValue;
        }
        Object v = obj.get(key);
        if (v instanceof Number n) {
            return n.longValue();
        }
        return defaultValue;
    }

    static int getInt(Map<String, Object> obj, String key, int defaultValue) {
        if (obj == null || key == null) {
            return defaultValue;
        }
        Object v = obj.get(key);
        if (v instanceof Number n) {
            return n.intValue();
        }
        return defaultValue;
    }

    static boolean getBoolean(Map<String, Object> obj, String key, boolean defaultValue) {
        if (obj == null || key == null) {
            return defaultValue;
        }
        Object v = obj.get(key);
        if (v instanceof Boolean b) {
            return b;
        }
        return defaultValue;
    }

    @SuppressWarnings("unchecked")
    static Map<String, Object> getObject(Map<String, Object> obj, String key) {
        if (obj == null || key == null) {
            return null;
        }
        Object v = obj.get(key);
        if (v instanceof Map<?, ?>) {
            return (Map<String, Object>) v;
        }
        return null;
    }

    @SuppressWarnings("unchecked")
    static List<Object> getArray(Map<String, Object> obj, String key) {
        if (obj == null || key == null) {
            return null;
        }
        Object v = obj.get(key);
        if (v instanceof List<?>) {
            return (List<Object>) v;
        }
        return null;
    }

    static String stringify(Object value) {
        StringBuilder sb = new StringBuilder(128);
        writeJson(sb, value, -1, 0);
        return sb.toString();
    }

    static String stringifyPretty(Object value) {
        StringBuilder sb = new StringBuilder(256);
        writeJson(sb, value, 2, 0);
        return sb.toString();
    }

    @SuppressWarnings("unchecked")
    private static void writeJson(StringBuilder sb, Object value, int indentStep, int depth) {
        if (value == null) {
            sb.append("null");
        } else if (value instanceof Boolean b) {
            sb.append(b.booleanValue() ? "true" : "false");
        } else if (value instanceof Number n) {
            if (n instanceof Double || n instanceof Float) {
                double d = n.doubleValue();
                if (Double.isNaN(d) || Double.isInfinite(d)) {
                    sb.append("0");
                } else if (d == Math.rint(d) && Math.abs(d) < 9e15) {
                    sb.append((long) d);
                } else {
                    sb.append(Double.toString(d));
                }
            } else {
                sb.append(n.longValue());
            }
        } else if (value instanceof String s) {
            writeEscapedString(sb, s);
        } else if (value instanceof Map<?, ?> map) {
            if (map.isEmpty()) {
                sb.append("{}");
                return;
            }
            sb.append('{');
            boolean first = true;
            for (Map.Entry<?, ?> entry : map.entrySet()) {
                if (entry.getKey() == null) {
                    continue;
                }
                if (!first) {
                    sb.append(',');
                }
                first = false;
                if (indentStep > 0) {
                    sb.append('\n');
                    appendSpaces(sb, (depth + 1) * indentStep);
                }
                writeEscapedString(sb, entry.getKey().toString());
                sb.append(indentStep > 0 ? ": " : ":");
                writeJson(sb, entry.getValue(), indentStep, depth + 1);
            }
            if (indentStep > 0) {
                sb.append('\n');
                appendSpaces(sb, depth * indentStep);
            }
            sb.append('}');
        } else if (value instanceof Iterable<?> list) {
            List<Object> items = new ArrayList<>();
            for (Object item : list) {
                items.add(item);
            }
            if (items.isEmpty()) {
                sb.append("[]");
                return;
            }
            sb.append('[');
            for (int i = 0; i < items.size(); ++i) {
                if (i > 0) {
                    sb.append(',');
                }
                if (indentStep > 0) {
                    sb.append('\n');
                    appendSpaces(sb, (depth + 1) * indentStep);
                }
                writeJson(sb, items.get(i), indentStep, depth + 1);
            }
            if (indentStep > 0) {
                sb.append('\n');
                appendSpaces(sb, depth * indentStep);
            }
            sb.append(']');
        } else {
            writeEscapedString(sb, value.toString());
        }
    }

    private static void appendSpaces(StringBuilder sb, int count) {
        for (int i = 0; i < count; ++i) {
            sb.append(' ');
        }
    }

    private static void writeEscapedString(StringBuilder sb, String s) {
        sb.append('"');
        for (int i = 0; i < s.length(); ++i) {
            char c = s.charAt(i);
            switch (c) {
                case '"':
                    sb.append("\\\"");
                    break;
                case '\\':
                    sb.append("\\\\");
                    break;
                case '\b':
                    sb.append("\\b");
                    break;
                case '\f':
                    sb.append("\\f");
                    break;
                case '\n':
                    sb.append("\\n");
                    break;
                case '\r':
                    sb.append("\\r");
                    break;
                case '\t':
                    sb.append("\\t");
                    break;
                default:
                    if (c < 0x20) {
                        sb.append(String.format(Locale.ROOT, "\\u%04x", (int) c));
                    } else {
                        sb.append(c);
                    }
                    break;
            }
        }
        sb.append('"');
    }

    private void skipWhitespace() {
        while (pos < src.length()) {
            char c = src.charAt(pos);
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                ++pos;
            } else {
                break;
            }
        }
    }

    private Object readValue() {
        skipWhitespace();
        if (pos >= src.length()) {
            throw new IllegalArgumentException("unexpected end of JSON");
        }
        char c = src.charAt(pos);
        return switch (c) {
            case '{' -> readObject();
            case '[' -> readArray();
            case '"' -> readString();
            case 't', 'f' -> readBoolean();
            case 'n' -> readNull();
            default -> {
                if (c == '-' || (c >= '0' && c <= '9')) {
                    yield readNumber();
                }
                throw new IllegalArgumentException("unexpected character '" + c + "' at index " + pos);
            }
        };
    }

    private Map<String, Object> readObject() {
        expect('{');
        Map<String, Object> map = new LinkedHashMap<>();
        skipWhitespace();
        if (pos < src.length() && src.charAt(pos) == '}') {
            ++pos;
            return map;
        }
        while (true) {
            skipWhitespace();
            if (pos >= src.length() || src.charAt(pos) != '"') {
                throw new IllegalArgumentException("expected string key in object at index " + pos);
            }
            String key = readString();
            skipWhitespace();
            expect(':');
            Object val = readValue();
            map.put(key, val);
            skipWhitespace();
            if (pos >= src.length()) {
                throw new IllegalArgumentException("unterminated JSON object");
            }
            char next = src.charAt(pos);
            if (next == '}') {
                ++pos;
                return map;
            }
            if (next != ',') {
                throw new IllegalArgumentException("expected ',' or '}' in object at index " + pos);
            }
            ++pos;
        }
    }

    private List<Object> readArray() {
        expect('[');
        List<Object> list = new ArrayList<>();
        skipWhitespace();
        if (pos < src.length() && src.charAt(pos) == ']') {
            ++pos;
            return list;
        }
        while (true) {
            Object val = readValue();
            list.add(val);
            skipWhitespace();
            if (pos >= src.length()) {
                throw new IllegalArgumentException("unterminated JSON array");
            }
            char next = src.charAt(pos);
            if (next == ']') {
                ++pos;
                return list;
            }
            if (next != ',') {
                throw new IllegalArgumentException("expected ',' or ']' in array at index " + pos);
            }
            ++pos;
        }
    }

    private String readString() {
        expect('"');
        StringBuilder sb = new StringBuilder();
        while (pos < src.length()) {
            char c = src.charAt(pos++);
            if (c == '"') {
                return sb.toString();
            }
            if (c < 0x20) {
                throw new IllegalArgumentException("unescaped control character in JSON string");
            }
            if (c == '\\') {
                if (pos >= src.length()) {
                    throw new IllegalArgumentException("unterminated escape sequence in JSON string");
                }
                char esc = src.charAt(pos++);
                switch (esc) {
                    case '"':
                        sb.append('"');
                        break;
                    case '\\':
                        sb.append('\\');
                        break;
                    case '/':
                        sb.append('/');
                        break;
                    case 'b':
                        sb.append('\b');
                        break;
                    case 'f':
                        sb.append('\f');
                        break;
                    case 'n':
                        sb.append('\n');
                        break;
                    case 'r':
                        sb.append('\r');
                        break;
                    case 't':
                        sb.append('\t');
                        break;
                    case 'u':
                        if (pos + 4 > src.length()) {
                            throw new IllegalArgumentException("incomplete \\uXXXX escape in JSON string");
                        }
                        String hex = src.substring(pos, pos + 4);
                        pos += 4;
                        try {
                            int codeUnit = Integer.parseInt(hex, 16);
                            sb.append((char) codeUnit);
                        } catch (NumberFormatException e) {
                            throw new IllegalArgumentException("invalid \\u" + hex + " escape");
                        }
                        break;
                    default:
                        throw new IllegalArgumentException("invalid escape '\\" + esc + "'");
                }
            } else {
                sb.append(c);
            }
        }
        throw new IllegalArgumentException("unterminated JSON string");
    }

    private Boolean readBoolean() {
        if (src.startsWith("true", pos)) {
            pos += 4;
            return Boolean.TRUE;
        }
        if (src.startsWith("false", pos)) {
            pos += 5;
            return Boolean.FALSE;
        }
        throw new IllegalArgumentException("invalid boolean literal at index " + pos);
    }

    private Object readNull() {
        if (src.startsWith("null", pos)) {
            pos += 4;
            return null;
        }
        throw new IllegalArgumentException("invalid null literal at index " + pos);
    }

    private Number readNumber() {
        int start = pos;
        if (pos < src.length() && src.charAt(pos) == '-') {
            ++pos;
        }
        if (pos >= src.length() || src.charAt(pos) < '0' || src.charAt(pos) > '9') {
            throw new IllegalArgumentException("invalid number at index " + start);
        }
        if (src.charAt(pos) == '0') {
            ++pos;
        } else {
            while (pos < src.length() && src.charAt(pos) >= '0' && src.charAt(pos) <= '9') {
                ++pos;
            }
        }
        boolean isFloat = false;
        if (pos < src.length() && src.charAt(pos) == '.') {
            isFloat = true;
            ++pos;
            if (pos >= src.length() || src.charAt(pos) < '0' || src.charAt(pos) > '9') {
                throw new IllegalArgumentException("invalid fractional number at index " + start);
            }
            while (pos < src.length() && src.charAt(pos) >= '0' && src.charAt(pos) <= '9') {
                ++pos;
            }
        }
        if (pos < src.length() && (src.charAt(pos) == 'e' || src.charAt(pos) == 'E')) {
            isFloat = true;
            ++pos;
            if (pos < src.length() && (src.charAt(pos) == '+' || src.charAt(pos) == '-')) {
                ++pos;
            }
            if (pos >= src.length() || src.charAt(pos) < '0' || src.charAt(pos) > '9') {
                throw new IllegalArgumentException("invalid exponent in number at index " + start);
            }
            while (pos < src.length() && src.charAt(pos) >= '0' && src.charAt(pos) <= '9') {
                ++pos;
            }
        }
        String token = src.substring(start, pos);
        try {
            if (isFloat) {
                return Double.parseDouble(token);
            }
            return Long.parseLong(token);
        } catch (NumberFormatException e) {
            return Double.parseDouble(token);
        }
    }

    private void expect(char expected) {
        if (pos >= src.length() || src.charAt(pos) != expected) {
            throw new IllegalArgumentException("expected '" + expected + "' at index " + pos);
        }
        ++pos;
    }
}
