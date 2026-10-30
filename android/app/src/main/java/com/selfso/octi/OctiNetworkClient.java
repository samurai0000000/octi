/*
 * OctiNetworkClient.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import java.io.ByteArrayOutputStream;
import java.io.EOFException;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.URLDecoder;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;

/**
 * Hard-coded REST + Server-Sent Events (SSE) client for
 * {@code trumpet.selfso.com:6874}.
 *
 * <p>Production instances created via {@link #OctiNetworkClient()} always
 * connect strictly to {@link OctiDefs#OCTI_SERVER_HOST}
 * ({@code "trumpet.selfso.com"}) and {@link OctiDefs#OCTI_SERVER_PORT}
 * ({@code 6874}), with zero public mutators to alter the server host or port.
 */
public final class OctiNetworkClient implements AutoCloseable {

    public static final class ApiResult<T> {
        public boolean ok = false;
        public int httpStatus = 0;
        public String errorCode = "";
        public String errorMessage = "";
        public int retryAfterSec = 0;
        public T data;

        public ApiResult() {
        }

        public ApiResult(T initialData) {
            this.data = initialData;
        }
    }

    public static final class HealthInfo {
        public String status = "";
        public String version = "";
        public long uptimeSec = 0;
        public long activeTables = 0;
        public long onlinePlayers = 0;
        public int protocolVersion = OctiDefs.OCTI_PROTOCOL_VERSION;
        public int minProtocolVersion = OctiDefs.OCTI_MIN_PROTOCOL_VERSION;
    }

    public static final class AuthProviderInfo {
        public String name = "";
        public String displayName = "";
        public List<String> clientIds = new ArrayList<>();
    }

    public static final class LinkedIdentityInfo {
        public String provider = "";
        public String providerEmail = "";
        public String displayHint = "";
        public long linkedAt = 0;
        public long lastLoginAt = 0;
    }

    public static final class ArchivedGamePlayer {
        public int seat = 0;
        public String handle = "";
        public String playerUuid = "";
    }

    public static final class ArchivedGameSummary {
        public String gameId = "";
        public String mode = "2P";
        public int bases = 3;
        public boolean superProngs = false;
        public boolean edgeless = false;
        public String timeControl = "untimed";
        public List<ArchivedGamePlayer> players = new ArrayList<>();
        public String result = "";
        public int winnerSeat = 0;
        public String termination = "";
        public int plyCount = 0;
        public long startedAt = 0;
        public long endedAt = 0;
        public String finalFen = "";
        public String octiTranscript = "";
    }

    public static final class ArchivedGamesPage {
        public long total = 0;
        public int limit = 20;
        public int offset = 0;
        public List<ArchivedGameSummary> games = new ArrayList<>();
    }

    public static final class UploadTranscriptRequest {
        public String transcriptJson = "";
        public String clientId = "";

        public UploadTranscriptRequest() {
        }

        public UploadTranscriptRequest(String transcriptJson, String clientId) {
            this.transcriptJson = transcriptJson != null ? transcriptJson : "";
            this.clientId = clientId != null ? clientId : "";
        }
    }

    public static final class UploadTranscriptResponse {
        public String gameId = "";
        public int plyCount = 0;
    }

    public static final class PlayerProfileInfo {
        public String uuid = "";
        public String handle = "";
        public String displayName = "";
        public String avatarUrl = "";
        public int elo2p = 1200;
        public int elo4p = 1200;
        public int eloOfk = 1200;
        public int gamesPlayed = 0;
        public int wins = 0;
        public int losses = 0;
        public int draws = 0;
        public long createdAt = 0;
        public long lastSeenAt = 0;
        public List<LinkedIdentityInfo> identities = new ArrayList<>();
        public List<ArchivedGameSummary> recentGames = new ArrayList<>();
    }

    public static final class SsoLoginData {
        public String sessionToken = "";
        public long expiresAt = 0;
        public boolean isNewAccount = false;
        public boolean onboardingRequired = false;
        public PlayerProfileInfo profile = new PlayerProfileInfo();
        public List<LinkedIdentityInfo> identities = new ArrayList<>();
    }

    public static final class LinkIdentityData {
        public String status = "";
        public List<LinkedIdentityInfo> identities = new ArrayList<>();
        public boolean conflictAlreadyLinked = false;
        public PlayerProfileInfo existingProfile = new PlayerProfileInfo();
    }

    public static final class UnlinkIdentityData {
        public String status = "";
        public List<LinkedIdentityInfo> identities = new ArrayList<>();
    }

    public static final class LeaderboardRow {
        public int rank = 0;
        public String uuid = "";
        public String handle = "";
        public String displayName = "";
        public int elo = 1200;
        public int gamesPlayed = 0;
        public int wins = 0;
        public int losses = 0;
        public int draws = 0;
    }

    public static final class LobbyPlayerInfo {
        public String playerUuid = "";
        public String handle = "";
        public String status = "IN_LOBBY";
        public String roomId = "";
    }

    public static final class LobbyChatEntry {
        public long seq = 0;
        public long atMs = 0;
        public String playerUuid = "";
        public String handle = "";
        public String text = "";
    }

    public static final class TableSummaryInfo {
        public String roomId = "";
        public String mode = "2P";
        public int bases = 3;
        public boolean superProngs = false;
        public boolean edgeless = false;
        public boolean timeControlEnabled = false;
        public long initialMs = 0;
        public long incrementMs = 0;
        public long disconnectGraceMs = 90000;
        public boolean rated = true;
        public boolean allowTakeback = false;
        public String observerPolicy = "OPEN";
        public String voicePolicy = "PLAYERS_ONLY";
        public String phase = "WAITING_FOR_PLAYERS";
        public boolean hasPasscode = false;
        public String hostHandle = "";
        public int seatsTotal = 2;
        public int seatsFilled = 0;
        public List<String> players = new ArrayList<>();
        public int observerCount = 0;
    }

    public static final class LobbyStateData {
        public List<LobbyPlayerInfo> players = new ArrayList<>();
        public List<TableSummaryInfo> tables = new ArrayList<>();
        public List<LobbyChatEntry> recentChat = new ArrayList<>();
        public long latestSeq = 0;
    }

    public static final class CreateTableRequest {
        public String mode = "2P";
        public int bases = 3;
        public boolean superProngs = false;
        public boolean edgeless = false;
        public boolean timeControlEnabled = false;
        public long initialMs = 600000;
        public long incrementMs = 5000;
        public long disconnectGraceMs = 90000;
        public boolean rated = true;
        public boolean allowTakeback = false;
        public String observerPolicy = "OPEN";
        public String voicePolicy = "PLAYERS_ONLY";
        public String passcode = "";
    }

    public static final class CreateTableResponse {
        public String roomId = "";
        public int seat = 1;
    }

    public static final class JoinTableResponse {
        public String roomId = "";
        public int seat = 0;
        public boolean gameStarted = false;
    }

    public static final class MatchmakingRequest {
        public String mode = "2P";
        public int bases = 3;
        public boolean superProngs = false;
        public boolean edgeless = false;
        public boolean timeControlEnabled = false;
        public long initialMs = 0;
        public long incrementMs = 0;
    }

    public static final class MatchmakingStatusData {
        public boolean queued = false;
        public long waitedMs = 0;
        public int window = 0;
        public int rating = 1200;
        public String matchedRoomId = "";
    }

    public static final class GameSeatInfo {
        public int seat = 0;
        public String playerUuid = "";
        public String handle = "";
        public int rating = 1200;
        public boolean connected = true;
        public long disconnectedForMs = 0;
        public boolean forfeitClaimable = false;
        public long clockMs = -1;
        public boolean eliminated = false;
    }

    public static final class GameProposalInfo {
        public String type = "";
        public String proposerUuid = "";
        public List<String> acceptedBy = new ArrayList<>();
        public List<String> awaiting = new ArrayList<>();
        public long expiresAtMs = 0;
    }

    public static final class VoicePeerInfo {
        public String playerUuid = "";
        public boolean micMuted = false;
    }

    public static final class GameViewData {
        public boolean live = true;
        public String roomId = "";
        public String mode = "2P";
        public int bases = 3;
        public boolean superProngs = false;
        public boolean edgeless = false;
        public boolean timeControlEnabled = false;
        public long initialMs = 0;
        public long incrementMs = 0;
        public long disconnectGraceMs = 90000;
        public boolean rated = true;
        public boolean allowTakeback = false;
        public String observerPolicy = "OPEN";
        public String voicePolicy = "PLAYERS_ONLY";
        public String phase = "WAITING_FOR_PLAYERS";
        public boolean paused = false;
        public boolean hasPasscode = false;
        public String hostUuid = "";
        public int viewerSeat = 0;
        public List<GameSeatInfo> seats = new ArrayList<>();
        public List<String> observers = new ArrayList<>();
        public int observerCount = 0;
        public String fen = "";
        public int turnSeat = 0;
        public int plyCount = 0;
        public List<String> moves = new ArrayList<>();
        public String result = "";
        public String termination = "";
        public int winnerSeat = 0;
        public List<GameProposalInfo> proposals = new ArrayList<>();
        public List<VoicePeerInfo> voicePeers = new ArrayList<>();
        public long seq = 0;
        public boolean archived = false;
        public String rematchRoomId = "";
        public long createdAtMs = 0;
    }

    public static final class MoveSubmitData {
        public String move = "";
        public long clockMs = -1;
        public long elapsedMs = 0;
        public boolean gameOver = false;
        public long seq = 0;
    }

    public static final class GameActionData {
        public String action = "";
        public String roomId = "";
        public boolean gameOver = false;
        public long seq = 0;
    }

    public static final class RoomSettingsData {
        public String observerPolicy = "";
        public String voicePolicy = "";
        public long seq = 0;
    }

    public static final class SseEvent {
        public long id = 0;
        public String event = "";
        public String dataJson = "";

        public SseEvent() {
        }

        public SseEvent(long id, String event, String dataJson) {
            this.id = id;
            this.event = event != null ? event : "";
            this.dataJson = dataJson != null ? dataJson : "";
        }
    }

    @FunctionalInterface
    public interface SseEventListener {
        void onEvent(SseEvent event);
    }

    /**
     * Ephemeral local HTTP callback listener bound strictly to
     * {@code 127.0.0.1:0} for system browser / Custom Tab OAuth SSO sign-in
     * redirects.
     */
    public static final class OAuthLoopbackListener implements AutoCloseable {

        @FunctionalInterface
        public interface Callback {
            void onOAuthCallback(String provider, String token);
        }

        private final Object _lock = new Object();
        private ServerSocket _serverSocket;
        private Thread _acceptThread;
        private final AtomicBoolean _running = new AtomicBoolean(false);
        private final AtomicInteger _boundPort = new AtomicInteger(0);
        private boolean _received = false;
        private String _receivedProvider = "";
        private String _receivedToken = "";
        private String _receivedIdToken = "";
        private String _receivedAccessToken = "";
        private Callback _callback;

        public OAuthLoopbackListener() {
        }

        public boolean start() {
            return start(null, null);
        }

        public boolean start(Callback onCallback) {
            return start(onCallback, null);
        }

        public boolean start(Callback onCallback, StringBuilder errorOut) {
            stop();
            synchronized (_lock) {
                _received = false;
                _receivedProvider = "";
                _receivedToken = "";
                _receivedIdToken = "";
                _receivedAccessToken = "";
                _callback = onCallback;
            }
            try {
                ServerSocket ss = new ServerSocket();
                ss.setReuseAddress(true);
                ss.bind(new InetSocketAddress(InetAddress.getByName("127.0.0.1"), 0));
                int port = ss.getLocalPort();
                if (port <= 0) {
                    ss.close();
                    if (errorOut != null) {
                        errorOut.setLength(0);
                        errorOut.append("failed to bind OAuth loopback listener on 127.0.0.1:0");
                    }
                    return false;
                }
                _serverSocket = ss;
                _boundPort.set(port);
                _running.set(true);
                _acceptThread = new Thread(this::acceptLoop, "octi-oauth-loopback");
                _acceptThread.setDaemon(true);
                _acceptThread.start();
                return true;
            } catch (IOException e) {
                if (errorOut != null) {
                    errorOut.setLength(0);
                    errorOut.append("failed to bind OAuth loopback listener: ").append(e.getMessage());
                }
                return false;
            }
        }

        public void stop() {
            _running.set(false);
            ServerSocket ss;
            synchronized (_lock) {
                ss = _serverSocket;
                _serverSocket = null;
                _lock.notifyAll();
            }
            if (ss != null) {
                try {
                    ss.close();
                } catch (IOException ignored) {
                }
            }
            Thread t = _acceptThread;
            _acceptThread = null;
            if (t != null && t != Thread.currentThread()) {
                try {
                    t.join(500);
                } catch (InterruptedException ignored) {
                    Thread.currentThread().interrupt();
                }
            }
            _boundPort.set(0);
        }

        @Override
        public void close() {
            stop();
        }

        public boolean isRunning() {
            return _running.get();
        }

        public int boundPort() {
            return _boundPort.get();
        }

        public int port() {
            return _boundPort.get();
        }

        public String callbackUrl() {
            int p = _boundPort.get();
            if (p <= 0) {
                return "";
            }
            return "http://127.0.0.1:" + p + "/callback";
        }

        public String redirectUri() {
            return callbackUrl();
        }

        public boolean waitForCallback(long timeoutMs, String[] outProviderAndToken) {
            long deadline = System.currentTimeMillis() + Math.max(0L, timeoutMs);
            synchronized (_lock) {
                while (!_received && _running.get()) {
                    long remaining = deadline - System.currentTimeMillis();
                    if (remaining <= 0) {
                        break;
                    }
                    try {
                        _lock.wait(remaining);
                    } catch (InterruptedException e) {
                        Thread.currentThread().interrupt();
                        break;
                    }
                }
                if (_received) {
                    if (outProviderAndToken != null && outProviderAndToken.length >= 2) {
                        outProviderAndToken[0] = _receivedProvider;
                        outProviderAndToken[1] = _receivedToken;
                    }
                    return true;
                }
                return false;
            }
        }

        public String getReceivedProvider() {
            synchronized (_lock) {
                return _receivedProvider;
            }
        }

        public String getReceivedToken() {
            synchronized (_lock) {
                return _receivedToken;
            }
        }

        public String getReceivedIdToken() {
            synchronized (_lock) {
                return _receivedIdToken;
            }
        }

        public String getReceivedAccessToken() {
            synchronized (_lock) {
                return _receivedAccessToken;
            }
        }

        private void acceptLoop() {
            while (_running.get()) {
                ServerSocket ss;
                synchronized (_lock) {
                    ss = _serverSocket;
                }
                if (ss == null || ss.isClosed()) {
                    break;
                }
                try (Socket client = ss.accept()) {
                    client.setSoTimeout(5000);
                    handleClient(client);
                } catch (IOException e) {
                    if (!_running.get()) {
                        break;
                    }
                }
            }
            _running.set(false);
            synchronized (_lock) {
                _lock.notifyAll();
            }
        }

        private void handleClient(Socket client) throws IOException {
            InputStream in = client.getInputStream();
            OutputStream out = client.getOutputStream();

            String requestLine = readHttpLine(in);
            if (requestLine == null || requestLine.isEmpty()) {
                return;
            }
            String[] parts = requestLine.split(" ");
            if (parts.length < 2) {
                writeHttpResponse(out, 400, "Bad Request", "text/plain; charset=utf-8", "Bad Request");
                return;
            }
            String method = parts[0].toUpperCase(Locale.ROOT);
            String fullTarget = parts[1];

            int contentLength = 0;
            String headerLine;
            while ((headerLine = readHttpLine(in)) != null && !headerLine.isEmpty()) {
                int colon = headerLine.indexOf(':');
                if (colon > 0) {
                    String hName = headerLine.substring(0, colon).trim().toLowerCase(Locale.ROOT);
                    String hVal = headerLine.substring(colon + 1).trim();
                    if ("content-length".equals(hName)) {
                        try {
                            contentLength = Math.max(0, Math.min(65536, Integer.parseInt(hVal)));
                        } catch (NumberFormatException ignored) {
                        }
                    }
                }
            }

            String bodyStr = "";
            if (contentLength > 0 && ("POST".equals(method) || "PUT".equals(method))) {
                byte[] bodyBytes = readExactBytes(in, contentLength);
                bodyStr = new String(bodyBytes, StandardCharsets.UTF_8);
            }

            String path = fullTarget;
            String query = "";
            int qIdx = fullTarget.indexOf('?');
            if (qIdx >= 0) {
                path = fullTarget.substring(0, qIdx);
                query = fullTarget.substring(qIdx + 1);
            }

            if (!"/callback".equals(path) && !"/oauth/callback".equals(path)) {
                writeHttpResponse(out, 404, "Not Found", "text/plain; charset=utf-8", "Not Found");
                return;
            }

            Map<String, String> params = new LinkedHashMap<>();
            parseUrlEncodedParams(query, params);
            if (!bodyStr.isEmpty()) {
                parseUrlEncodedParams(bodyStr, params);
            }

            String prov = params.getOrDefault("provider", "google");
            if (prov.isEmpty()) {
                prov = "google";
            }
            String idTok = params.getOrDefault("id_token", "");
            String accTok = params.getOrDefault("access_token", "");
            String tok = params.getOrDefault("token", "");
            if (tok.isEmpty()) {
                tok = !idTok.isEmpty() ? idTok : accTok;
            }

            if (tok.isEmpty()) {
                String errHtml = "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
                        + "<title>OCTI Sign-In Incomplete</title></head>"
                        + "<body><h2>OCTI Sign-In Incomplete: missing token</h2></body></html>";
                writeHttpResponse(out, 400, "Bad Request", "text/html; charset=utf-8", errHtml);
                return;
            }

            Callback cbCopy;
            synchronized (_lock) {
                _received = true;
                _receivedProvider = prov;
                _receivedToken = tok;
                _receivedIdToken = !idTok.isEmpty() ? idTok : tok;
                _receivedAccessToken = accTok;
                cbCopy = _callback;
                _lock.notifyAll();
            }
            if (cbCopy != null) {
                cbCopy.onOAuthCallback(prov, tok);
            }

            String okHtml = "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
                    + "<title>Sign-in complete</title></head>"
                    + "<body style=\"background:#0B0F17;color:#F8FAFC;"
                    + "font-family:sans-serif;text-align:center;padding:48px;\">"
                    + "<h2 style=\"color:#C8A253;\">Sign-in complete — you may return to OCTI</h2>"
                    + "<p>Your mobile session has been authenticated.</p>"
                    + "</body></html>";
            writeHttpResponse(out, 200, "OK", "text/html; charset=utf-8", okHtml);
        }

        private static void parseUrlEncodedParams(String encoded, Map<String, String> out) {
            if (encoded == null || encoded.isEmpty()) {
                return;
            }
            String[] pairs = encoded.split("&");
            for (String pair : pairs) {
                if (pair.isEmpty()) {
                    continue;
                }
                int eq = pair.indexOf('=');
                if (eq >= 0) {
                    String k = URLDecoder.decode(pair.substring(0, eq), StandardCharsets.UTF_8);
                    String v = URLDecoder.decode(pair.substring(eq + 1), StandardCharsets.UTF_8);
                    out.put(k, v);
                } else {
                    String k = URLDecoder.decode(pair, StandardCharsets.UTF_8);
                    out.put(k, "");
                }
            }
        }

        private static void writeHttpResponse(
                OutputStream out, int status, String reason, String contentType, String body) throws IOException {
            byte[] bodyBytes = body.getBytes(StandardCharsets.UTF_8);
            String headers = "HTTP/1.1 " + status + " " + reason + "\r\n"
                    + "Content-Type: " + contentType + "\r\n"
                    + "Content-Length: " + bodyBytes.length + "\r\n"
                    + "Cache-Control: no-store\r\n"
                    + "Connection: close\r\n\r\n";
            out.write(headers.getBytes(StandardCharsets.US_ASCII));
            out.write(bodyBytes);
            out.flush();
        }
    }

    private static final class RawResponse {
        boolean transportOk = false;
        int status = 0;
        String body = "";
        String contentType = "";
        String transportError = "";
        int retryAfterSec = 0;
        Map<String, Object> json;
    }

    private static final class SseRunner {
        final Object mutex = new Object();
        Socket activeSocket;
        Thread worker;
        final AtomicBoolean stopRequested = new AtomicBoolean(false);
        final AtomicBoolean active = new AtomicBoolean(false);
        final AtomicLong lastId = new AtomicLong(0);

        void stop() {
            stopRequested.set(true);
            Socket sock;
            synchronized (mutex) {
                sock = activeSocket;
                mutex.notifyAll();
            }
            if (sock != null) {
                try {
                    sock.close();
                } catch (IOException ignored) {
                }
            }
            Thread w = worker;
            worker = null;
            if (w != null && w != Thread.currentThread()) {
                try {
                    w.join(500);
                } catch (InterruptedException ignored) {
                    Thread.currentThread().interrupt();
                }
            }
            synchronized (mutex) {
                activeSocket = null;
            }
            active.set(false);
        }
    }

    private final String _host;
    private final int _port;
    private final Object _tokenLock = new Object();
    private String _sessionToken = "";
    private final SseRunner _lobbyStream = new SseRunner();
    private final SseRunner _gameStream = new SseRunner();

    public static String canonicalHost() {
        return OctiDefs.OCTI_SERVER_HOST;
    }

    public static int canonicalPort() {
        return OctiDefs.OCTI_SERVER_PORT;
    }

    public OctiNetworkClient() {
        this._host = OctiDefs.OCTI_SERVER_HOST;
        this._port = OctiDefs.OCTI_SERVER_PORT;
    }

    private OctiNetworkClient(String host, int port) {
        this._host = host;
        this._port = port;
    }

    static OctiNetworkClient forHermeticTest(int loopbackPort) {
        return forHermeticTest("127.0.0.1", loopbackPort);
    }

    static OctiNetworkClient forHermeticTest(String loopbackHost, int loopbackPort) {
        if (!"127.0.0.1".equals(loopbackHost) && !"localhost".equals(loopbackHost)) {
            throw new IllegalArgumentException("Hermetic test client must bind to 127.0.0.1 or localhost");
        }
        if (loopbackPort <= 0 || loopbackPort > 65535) {
            throw new IllegalArgumentException("Invalid loopbackPort: " + loopbackPort);
        }
        return new OctiNetworkClient(loopbackHost, loopbackPort);
    }

    static OctiNetworkClient createForLoopbackTest(int loopbackPort) {
        return forHermeticTest(loopbackPort);
    }

    public String host() {
        return _host;
    }

    public String serverHost() {
        return _host;
    }

    public int port() {
        return _port;
    }

    public int serverPort() {
        return _port;
    }

    public String baseUrl() {
        return "http://" + _host + ":" + _port;
    }

    @Override
    public void close() {
        stopEventStreams();
    }

    public void setSessionToken(String token) {
        synchronized (_tokenLock) {
            _sessionToken = token != null ? token : "";
        }
    }

    public String sessionToken() {
        synchronized (_tokenLock) {
            return _sessionToken;
        }
    }

    public void clearSessionToken() {
        synchronized (_tokenLock) {
            _sessionToken = "";
        }
    }

    /* ================================================================== */
    /* 1..10: Health, Auth, Profile & Leaderboard                         */
    /* ================================================================== */

    public ApiResult<HealthInfo> fetchHealth() {
        ApiResult<HealthInfo> res = new ApiResult<>(new HealthInfo());
        RawResponse raw = doGet("/api/v1/health");
        if (!raw.transportOk || raw.status != 200 || raw.json == null) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        res.data.status = OctiMiniJson.getString(raw.json, "status", "");
        res.data.version = OctiMiniJson.getString(raw.json, "version", "");
        res.data.uptimeSec = OctiMiniJson.getLong(raw.json, "uptime_sec", 0);
        res.data.activeTables = OctiMiniJson.getLong(raw.json, "active_tables", 0);
        res.data.onlinePlayers = OctiMiniJson.getLong(raw.json, "online_players", 0);
        res.data.protocolVersion = OctiMiniJson.getInt(
                raw.json, "protocol_version", OctiDefs.OCTI_PROTOCOL_VERSION);
        res.data.minProtocolVersion = OctiMiniJson.getInt(
                raw.json, "min_protocol_version", OctiDefs.OCTI_MIN_PROTOCOL_VERSION);
        return res;
    }

    public ApiResult<List<AuthProviderInfo>> fetchAuthProviders() {
        ApiResult<List<AuthProviderInfo>> res = new ApiResult<>(new ArrayList<>());
        RawResponse raw = doGet("/api/v1/auth/providers");
        if (!raw.transportOk || raw.status != 200 || raw.json == null) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        List<Object> arr = OctiMiniJson.getArray(raw.json, "providers");
        if (arr != null) {
            for (Object item : arr) {
                if (!(item instanceof Map<?, ?>)) {
                    continue;
                }
                @SuppressWarnings("unchecked")
                Map<String, Object> m = (Map<String, Object>) item;
                AuthProviderInfo info = new AuthProviderInfo();
                info.name = OctiMiniJson.getString(m, "name", "");
                info.displayName = OctiMiniJson.getString(m, "display_name", "");
                List<Object> cids = OctiMiniJson.getArray(m, "client_ids");
                if (cids != null) {
                    for (Object cid : cids) {
                        if (cid instanceof String s) {
                            info.clientIds.add(s);
                        }
                    }
                }
                res.data.add(info);
            }
        }
        return res;
    }

    public ApiResult<List<AuthProviderInfo>> fetchProviders() {
        return fetchAuthProviders();
    }

    public ApiResult<SsoLoginData> loginWithSso(String provider, String token) {
        return loginWithSso(provider, token, "", false);
    }

    public ApiResult<SsoLoginData> loginWithSso(String provider, String token, String desiredHandle) {
        return loginWithSso(provider, token, desiredHandle, false);
    }

    public ApiResult<SsoLoginData> loginWithSso(
            String provider, String idToken, String accessToken, String displayName) {
        boolean useAccess = (idToken == null || idToken.isEmpty())
                && (accessToken != null && !accessToken.isEmpty());
        String tok = useAccess ? accessToken : (idToken != null ? idToken : "");
        return loginWithSso(provider, tok, displayName != null ? displayName : "", useAccess);
    }

    public ApiResult<SsoLoginData> loginWithSso(
            String provider, String token, String desiredHandle, boolean isAccessToken) {
        ApiResult<SsoLoginData> res = new ApiResult<>(new SsoLoginData());
        Map<String, Object> body = new LinkedHashMap<>();
        body.put("provider", provider != null ? provider : "");
        body.put("client_agent", "octi-android");
        if (isAccessToken) {
            body.put("access_token", token != null ? token : "");
        } else {
            body.put("id_token", token != null ? token : "");
        }

        RawResponse raw = doPost("/api/v1/auth/sso", body);
        if (!raw.transportOk || (raw.status != 200 && raw.status != 201) || raw.json == null) {
            populateError(raw, res);
            return res;
        }

        res.ok = true;
        res.httpStatus = raw.status;
        res.data.sessionToken = OctiMiniJson.getString(raw.json, "session_token", "");
        res.data.expiresAt = OctiMiniJson.getLong(raw.json, "expires_at", 0);
        res.data.isNewAccount = (raw.status == 201)
                || OctiMiniJson.getBoolean(raw.json, "created", false)
                || OctiMiniJson.getBoolean(raw.json, "onboarding_required", false);
        res.data.onboardingRequired = res.data.isNewAccount;
        Map<String, Object> profObj = OctiMiniJson.getObject(raw.json, "profile");
        if (profObj != null) {
            res.data.profile = parsePlayerProfile(profObj);
        }
        List<Object> idArr = OctiMiniJson.getArray(raw.json, "linked_identities");
        if (idArr != null) {
            res.data.identities = parseLinkedIdentities(idArr);
            res.data.profile.identities = new ArrayList<>(res.data.identities);
        }

        if (!res.data.sessionToken.isEmpty()) {
            setSessionToken(res.data.sessionToken);
        }

        if (desiredHandle != null && !desiredHandle.isEmpty() && !res.data.sessionToken.isEmpty()) {
            ApiResult<PlayerProfileInfo> patched = updateMyProfile(desiredHandle);
            if (patched.ok && patched.data != null) {
                res.data.profile = patched.data;
                res.data.profile.identities = new ArrayList<>(res.data.identities);
            }
        }

        return res;
    }

    public ApiResult<LinkIdentityData> linkIdentity(String provider, String token) {
        return linkIdentity(provider, token, false);
    }

    public ApiResult<LinkIdentityData> linkIdentity(
            String provider, String idToken, String accessToken) {
        boolean useAccess = (idToken == null || idToken.isEmpty())
                && (accessToken != null && !accessToken.isEmpty());
        String tok = useAccess ? accessToken : (idToken != null ? idToken : "");
        return linkIdentity(provider, tok, useAccess);
    }

    public ApiResult<LinkIdentityData> linkIdentity(
            String provider, String token, boolean isAccessToken) {
        ApiResult<LinkIdentityData> res = new ApiResult<>(new LinkIdentityData());
        Map<String, Object> body = new LinkedHashMap<>();
        body.put("provider", provider != null ? provider : "");
        if (isAccessToken) {
            body.put("access_token", token != null ? token : "");
        } else {
            body.put("id_token", token != null ? token : "");
        }

        RawResponse raw = doPost("/api/v1/auth/link", body);
        if (!raw.transportOk) {
            populateError(raw, res);
            return res;
        }

        if (raw.status == 409 && raw.json != null) {
            populateError(raw, res);
            res.data.conflictAlreadyLinked = true;
            Map<String, Object> existing = OctiMiniJson.getObject(raw.json, "existing_profile");
            if (existing != null) {
                res.data.existingProfile = parsePlayerProfile(existing);
            }
            return res;
        }

        if (raw.status != 200 || raw.json == null) {
            populateError(raw, res);
            return res;
        }

        res.ok = true;
        res.httpStatus = raw.status;
        res.data.status = OctiMiniJson.getString(raw.json, "status", "linked");
        List<Object> idArr = OctiMiniJson.getArray(raw.json, "linked_identities");
        if (idArr != null) {
            res.data.identities = parseLinkedIdentities(idArr);
        }
        return res;
    }

    public ApiResult<UnlinkIdentityData> unlinkIdentity(String provider) {
        ApiResult<UnlinkIdentityData> res = new ApiResult<>(new UnlinkIdentityData());
        RawResponse raw = doDelete("/api/v1/auth/link/" + urlEncode(provider));
        if (!raw.transportOk || raw.status != 200 || raw.json == null) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        res.data.status = OctiMiniJson.getString(raw.json, "status", "unlinked");
        List<Object> idArr = OctiMiniJson.getArray(raw.json, "linked_identities");
        if (idArr != null) {
            res.data.identities = parseLinkedIdentities(idArr);
        }
        return res;
    }

    public ApiResult<Boolean> logout() {
        ApiResult<Boolean> res = new ApiResult<>(Boolean.FALSE);
        RawResponse raw = doPost("/api/v1/auth/logout", Collections.emptyMap());
        if (!raw.transportOk || raw.status != 200) {
            populateError(raw, res);
            return res;
        }
        clearSessionToken();
        res.ok = true;
        res.httpStatus = raw.status;
        res.data = Boolean.TRUE;
        return res;
    }

    public ApiResult<PlayerProfileInfo> fetchMyProfile() {
        ApiResult<PlayerProfileInfo> res = new ApiResult<>(new PlayerProfileInfo());
        RawResponse raw = doGet("/api/v1/profile/me");
        if (!raw.transportOk || raw.status != 200 || raw.json == null) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        res.data = parsePlayerProfile(raw.json);
        return res;
    }

    public ApiResult<PlayerProfileInfo> updateMyProfile(String handle) {
        return updateMyProfile(handle, "", "");
    }

    public ApiResult<PlayerProfileInfo> updateMyProfile(String handle, String avatarUrl) {
        return updateMyProfile(handle, avatarUrl, "");
    }

    public ApiResult<PlayerProfileInfo> updateMyProfile(
            String handle, String avatarUrl, String displayName) {
        ApiResult<PlayerProfileInfo> res = new ApiResult<>(new PlayerProfileInfo());
        Map<String, Object> body = new LinkedHashMap<>();
        if (handle != null && !handle.isEmpty()) {
            body.put("handle", handle);
        }
        if (avatarUrl != null && !avatarUrl.isEmpty()) {
            body.put("avatar_url", avatarUrl);
        }
        if (displayName != null && !displayName.isEmpty()) {
            body.put("display_name", displayName);
        }
        RawResponse raw = doPatch("/api/v1/profile/me", body);
        if (!raw.transportOk || raw.status != 200 || raw.json == null) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        res.data = parsePlayerProfile(raw.json);
        return res;
    }

    public ApiResult<PlayerProfileInfo> fetchPlayerProfile(String playerUuid) {
        ApiResult<PlayerProfileInfo> res = new ApiResult<>(new PlayerProfileInfo());
        RawResponse raw = doGet("/api/v1/players/" + urlEncode(playerUuid));
        if (!raw.transportOk || raw.status != 200 || raw.json == null) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        res.data = parsePlayerProfile(raw.json);
        return res;
    }

    public ApiResult<List<LeaderboardRow>> fetchLeaderboard() {
        return fetchLeaderboard("2P", 50);
    }

    public ApiResult<List<LeaderboardRow>> fetchLeaderboard(String mode, int limit) {
        ApiResult<List<LeaderboardRow>> res = new ApiResult<>(new ArrayList<>());
        String m = (mode == null || mode.isEmpty()) ? "2P" : mode;
        String path = "/api/v1/leaderboard?mode=" + urlEncode(m) + "&limit=" + limit;
        RawResponse raw = doGet(path);
        if (!raw.transportOk || raw.status != 200 || raw.json == null) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        List<Object> arr = OctiMiniJson.getArray(raw.json, "entries");
        if (arr != null) {
            for (int i = 0; i < arr.size(); ++i) {
                Object item = arr.get(i);
                if (!(item instanceof Map<?, ?>)) {
                    continue;
                }
                @SuppressWarnings("unchecked")
                Map<String, Object> e = (Map<String, Object>) item;
                LeaderboardRow row = new LeaderboardRow();
                row.rank = OctiMiniJson.getInt(e, "rank", i + 1);
                row.uuid = OctiMiniJson.getString(e, "uuid", "");
                row.handle = OctiMiniJson.getString(e, "handle", "");
                row.displayName = OctiMiniJson.getString(e, "display_name", "");
                row.elo = OctiMiniJson.getInt(e, "elo", 1200);
                row.gamesPlayed = OctiMiniJson.getInt(e, "games_played", 0);
                row.wins = OctiMiniJson.getInt(e, "wins", 0);
                row.losses = OctiMiniJson.getInt(e, "losses", 0);
                row.draws = OctiMiniJson.getInt(e, "draws", 0);
                res.data.add(row);
            }
        }
        return res;
    }

    /* ================================================================== */
    /* 11..22: Lobby, Tables & Matchmaking                                */
    /* ================================================================== */

    public ApiResult<LobbyStateData> fetchLobbyState() {
        ApiResult<LobbyStateData> res = new ApiResult<>(new LobbyStateData());
        RawResponse raw = doGet("/api/v1/lobby/state");
        if (!raw.transportOk || raw.status != 200 || raw.json == null) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        List<Object> pl = OctiMiniJson.getArray(raw.json, "players");
        if (pl != null) {
            for (Object item : pl) {
                if (!(item instanceof Map<?, ?>)) {
                    continue;
                }
                @SuppressWarnings("unchecked")
                Map<String, Object> m = (Map<String, Object>) item;
                LobbyPlayerInfo p = new LobbyPlayerInfo();
                p.playerUuid = OctiMiniJson.getString(m, "player_uuid", "");
                p.handle = OctiMiniJson.getString(m, "handle", "");
                p.status = OctiMiniJson.getString(m, "status", "IN_LOBBY");
                p.roomId = OctiMiniJson.getString(m, "room_id", "");
                res.data.players.add(p);
            }
        }
        List<Object> tb = OctiMiniJson.getArray(raw.json, "tables");
        if (tb != null) {
            for (Object item : tb) {
                if (item instanceof Map<?, ?>) {
                    @SuppressWarnings("unchecked")
                    Map<String, Object> m = (Map<String, Object>) item;
                    res.data.tables.add(parseTableSummary(m));
                }
            }
        }
        List<Object> ch = OctiMiniJson.getArray(raw.json, "recent_chat");
        if (ch != null) {
            for (Object item : ch) {
                if (!(item instanceof Map<?, ?>)) {
                    continue;
                }
                @SuppressWarnings("unchecked")
                Map<String, Object> m = (Map<String, Object>) item;
                LobbyChatEntry c = new LobbyChatEntry();
                c.seq = OctiMiniJson.getLong(m, "seq", 0);
                c.atMs = OctiMiniJson.getLong(m, "at_ms", 0);
                c.playerUuid = OctiMiniJson.getString(m, "player_uuid", "");
                c.handle = OctiMiniJson.getString(m, "handle", "");
                c.text = OctiMiniJson.getString(m, "text", "");
                res.data.recentChat.add(c);
            }
        }
        res.data.latestSeq = OctiMiniJson.getLong(raw.json, "latest_seq", 0);
        return res;
    }

    public ApiResult<Long> postLobbyChat(String message) {
        ApiResult<Long> res = new ApiResult<>(0L);
        Map<String, Object> body = new LinkedHashMap<>();
        body.put("text", message != null ? message : "");
        RawResponse raw = doPost("/api/v1/lobby/chat", body);
        if (!raw.transportOk || raw.status != 200 || raw.json == null) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        res.data = OctiMiniJson.getLong(raw.json, "seq", 0);
        return res;
    }

    public ApiResult<List<TableSummaryInfo>> listTables() {
        return listTables(false);
    }

    public ApiResult<List<TableSummaryInfo>> listTables(boolean includeFinished) {
        ApiResult<List<TableSummaryInfo>> res = new ApiResult<>(new ArrayList<>());
        String path = includeFinished ? "/api/v1/tables?include_finished=1" : "/api/v1/tables";
        RawResponse raw = doGet(path);
        if (!raw.transportOk || raw.status != 200 || raw.json == null) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        List<Object> arr = OctiMiniJson.getArray(raw.json, "tables");
        if (arr != null) {
            for (Object item : arr) {
                if (item instanceof Map<?, ?>) {
                    @SuppressWarnings("unchecked")
                    Map<String, Object> m = (Map<String, Object>) item;
                    res.data.add(parseTableSummary(m));
                }
            }
        }
        return res;
    }

    public ApiResult<CreateTableResponse> createTable() {
        return createTable(new CreateTableRequest());
    }

    public ApiResult<CreateTableResponse> createTable(
            String mode,
            int bases,
            boolean superProngs,
            boolean edgeless,
            boolean rated,
            boolean allowTakeback,
            long initialMs,
            long incrementMs,
            String observerPolicy,
            String voicePolicy,
            String passcode) {
        CreateTableRequest req = new CreateTableRequest();
        req.mode = mode != null && !mode.isEmpty() ? mode : "2P";
        req.bases = bases;
        req.superProngs = superProngs;
        req.edgeless = edgeless;
        req.rated = rated;
        req.allowTakeback = allowTakeback;
        req.timeControlEnabled = initialMs > 0;
        req.initialMs = initialMs;
        req.incrementMs = incrementMs;
        req.observerPolicy = observerPolicy != null && !observerPolicy.isEmpty() ? observerPolicy : "OPEN";
        req.voicePolicy = voicePolicy != null && !voicePolicy.isEmpty() ? voicePolicy : "PLAYERS_ONLY";
        req.passcode = passcode != null ? passcode : "";
        return createTable(req);
    }

    public ApiResult<CreateTableResponse> createTable(CreateTableRequest req) {
        ApiResult<CreateTableResponse> res = new ApiResult<>(new CreateTableResponse());
        CreateTableRequest r = req != null ? req : new CreateTableRequest();
        Map<String, Object> body = new LinkedHashMap<>();
        body.put("mode", r.mode != null ? r.mode : "2P");
        body.put("bases", r.bases);
        body.put("super_prongs", r.superProngs);
        body.put("edgeless", r.edgeless);
        body.put("rated", r.rated);
        body.put("allow_takeback", r.allowTakeback);
        body.put("observer_policy", normalizeObserverPolicy(r.observerPolicy));
        body.put("voice_policy", normalizeVoicePolicy(r.voicePolicy));
        if (r.passcode != null && !r.passcode.isEmpty()) {
            body.put("passcode", r.passcode);
        }
        if (r.timeControlEnabled && r.initialMs > 0) {
            Map<String, Object> tc = new LinkedHashMap<>();
            tc.put("initial_ms", r.initialMs);
            tc.put("increment_ms", r.incrementMs);
            tc.put("disconnect_grace_ms", r.disconnectGraceMs);
            body.put("time_control", tc);
        } else {
            body.put("time_control", "untimed");
        }

        RawResponse raw = doPost("/api/v1/tables", body);
        if (!raw.transportOk || (raw.status != 201 && raw.status != 200) || raw.json == null) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        res.data.roomId = OctiMiniJson.getString(raw.json, "room_id", "");
        res.data.seat = OctiMiniJson.getInt(raw.json, "seat", 1);
        return res;
    }

    public ApiResult<JoinTableResponse> joinTable(String roomId, int seat) {
        return joinTable(roomId, seat, "");
    }

    public ApiResult<JoinTableResponse> joinTable(String roomId, int seat, String passcode) {
        ApiResult<JoinTableResponse> res = new ApiResult<>(new JoinTableResponse());
        Map<String, Object> body = new LinkedHashMap<>();
        body.put("seat", seat);
        if (passcode != null && !passcode.isEmpty()) {
            body.put("passcode", passcode);
        }
        RawResponse raw = doPost("/api/v1/tables/" + urlEncode(roomId) + "/join", body);
        if (!raw.transportOk || raw.status != 200 || raw.json == null) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        res.data.roomId = OctiMiniJson.getString(raw.json, "room_id", roomId != null ? roomId : "");
        res.data.seat = OctiMiniJson.getInt(raw.json, "seat", seat);
        res.data.gameStarted = OctiMiniJson.getBoolean(raw.json, "game_started", false);
        return res;
    }

    public ApiResult<Boolean> leaveTable(String roomId) {
        ApiResult<Boolean> res = new ApiResult<>(Boolean.FALSE);
        RawResponse raw = doPost("/api/v1/tables/" + urlEncode(roomId) + "/leave", Collections.emptyMap());
        if (!raw.transportOk || raw.status != 200) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        res.data = Boolean.TRUE;
        return res;
    }

    public ApiResult<Boolean> kickSeat(String roomId, String targetPlayerUuid) {
        ApiResult<Boolean> res = new ApiResult<>(Boolean.FALSE);
        Map<String, Object> body = new LinkedHashMap<>();
        body.put("target", targetPlayerUuid != null ? targetPlayerUuid : "");
        RawResponse raw = doPost("/api/v1/tables/" + urlEncode(roomId) + "/kick", body);
        if (!raw.transportOk || raw.status != 200) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        res.data = Boolean.TRUE;
        return res;
    }

    public ApiResult<Boolean> kickSeat(String roomId, int seat) {
        ApiResult<GameViewData> gv = fetchGameView(roomId);
        if (!gv.ok || gv.data == null) {
            ApiResult<Boolean> res = new ApiResult<>(Boolean.FALSE);
            res.ok = false;
            res.httpStatus = gv.httpStatus;
            res.errorCode = gv.errorCode;
            res.errorMessage = gv.errorMessage;
            return res;
        }
        for (GameSeatInfo s : gv.data.seats) {
            if (s.seat == seat && s.playerUuid != null && !s.playerUuid.isEmpty()) {
                return kickSeat(roomId, s.playerUuid);
            }
        }
        return kickSeat(roomId, "");
    }

    public ApiResult<Boolean> observeTable(String roomId) {
        return observeTable(roomId, "");
    }

    public ApiResult<Boolean> observeTable(String roomId, String passcode) {
        ApiResult<Boolean> res = new ApiResult<>(Boolean.FALSE);
        Map<String, Object> body = new LinkedHashMap<>();
        if (passcode != null && !passcode.isEmpty()) {
            body.put("passcode", passcode);
        }
        RawResponse raw = doPost("/api/v1/tables/" + urlEncode(roomId) + "/observe", body);
        if (!raw.transportOk || raw.status != 200) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        res.data = Boolean.TRUE;
        return res;
    }

    public ApiResult<Boolean> unobserveTable(String roomId) {
        ApiResult<Boolean> res = new ApiResult<>(Boolean.FALSE);
        RawResponse raw = doPost("/api/v1/tables/" + urlEncode(roomId) + "/unobserve", Collections.emptyMap());
        if (!raw.transportOk || raw.status != 200) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        res.data = Boolean.TRUE;
        return res;
    }

    public ApiResult<Boolean> enqueueMatchmaking() {
        return enqueueMatchmaking(new MatchmakingRequest());
    }

    public ApiResult<Boolean> enqueueMatchmaking(
            String mode, int bases, boolean superProngs, boolean edgeless, long initialMs, long incrementMs) {
        MatchmakingRequest req = new MatchmakingRequest();
        req.mode = mode != null && !mode.isEmpty() ? mode : "2P";
        req.bases = bases;
        req.superProngs = superProngs;
        req.edgeless = edgeless;
        req.timeControlEnabled = initialMs > 0;
        req.initialMs = initialMs;
        req.incrementMs = incrementMs;
        return enqueueMatchmaking(req);
    }

    public ApiResult<Boolean> enqueueMatchmaking(MatchmakingRequest req) {
        ApiResult<Boolean> res = new ApiResult<>(Boolean.FALSE);
        MatchmakingRequest r = req != null ? req : new MatchmakingRequest();
        Map<String, Object> body = new LinkedHashMap<>();
        body.put("mode", r.mode != null ? r.mode : "2P");
        body.put("bases", r.bases);
        body.put("super_prongs", r.superProngs);
        body.put("edgeless", r.edgeless);
        if (r.timeControlEnabled && r.initialMs > 0) {
            Map<String, Object> tc = new LinkedHashMap<>();
            tc.put("initial_ms", r.initialMs);
            tc.put("increment_ms", r.incrementMs);
            body.put("time_control", tc);
        } else {
            body.put("time_control", "untimed");
        }
        RawResponse raw = doPost("/api/v1/matchmaking/enqueue", body);
        if (!raw.transportOk || raw.status != 200) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        res.data = Boolean.TRUE;
        return res;
    }

    public ApiResult<Boolean> dequeueMatchmaking() {
        ApiResult<Boolean> res = new ApiResult<>(Boolean.FALSE);
        RawResponse raw = doPost("/api/v1/matchmaking/dequeue", Collections.emptyMap());
        if (!raw.transportOk || raw.status != 200) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        res.data = Boolean.TRUE;
        return res;
    }

    public ApiResult<MatchmakingStatusData> fetchMatchmakingStatus() {
        ApiResult<MatchmakingStatusData> res = new ApiResult<>(new MatchmakingStatusData());
        RawResponse raw = doGet("/api/v1/matchmaking/status");
        if (!raw.transportOk || raw.status != 200 || raw.json == null) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        res.data.queued = OctiMiniJson.getBoolean(raw.json, "queued", false);
        res.data.waitedMs = OctiMiniJson.getLong(raw.json, "waited_ms", 0);
        res.data.window = OctiMiniJson.getInt(raw.json, "window", 0);
        res.data.rating = OctiMiniJson.getInt(raw.json, "rating", 1200);
        res.data.matchedRoomId = OctiMiniJson.getString(raw.json, "matched_room_id", "");
        return res;
    }

    /* ================================================================== */
    /* 23..32: Live Game, Chat, Voice Signaling & Moderation              */
    /* ================================================================== */

    public ApiResult<GameViewData> fetchGameView(String roomId) {
        ApiResult<GameViewData> res = new ApiResult<>(new GameViewData());
        RawResponse raw = doGet("/api/v1/games/" + urlEncode(roomId));
        if (!raw.transportOk || raw.status != 200 || raw.json == null) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        res.data = parseGameView(raw.json);
        return res;
    }

    public ApiResult<MoveSubmitData> submitMove(String roomId, String moveNotation) {
        ApiResult<MoveSubmitData> res = new ApiResult<>(new MoveSubmitData());
        Map<String, Object> body = new LinkedHashMap<>();
        body.put("move", moveNotation != null ? moveNotation : "");
        RawResponse raw = doPost("/api/v1/games/" + urlEncode(roomId) + "/move", body);
        if (!raw.transportOk || raw.status != 200 || raw.json == null) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        res.data.move = OctiMiniJson.getString(raw.json, "move", moveNotation != null ? moveNotation : "");
        res.data.clockMs = OctiMiniJson.getLong(raw.json, "clock_ms", -1);
        res.data.elapsedMs = OctiMiniJson.getLong(raw.json, "elapsed_ms", 0);
        res.data.gameOver = OctiMiniJson.getBoolean(raw.json, "game_over", false);
        res.data.seq = OctiMiniJson.getLong(raw.json, "seq", 0);
        return res;
    }

    public ApiResult<Boolean> resignGame(String roomId) {
        ApiResult<Boolean> res = new ApiResult<>(Boolean.FALSE);
        RawResponse raw = doPost("/api/v1/games/" + urlEncode(roomId) + "/resign", Collections.emptyMap());
        if (!raw.transportOk || raw.status != 200 || raw.json == null) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        res.data = OctiMiniJson.getBoolean(raw.json, "game_over", true);
        return res;
    }

    public ApiResult<GameActionData> submitGameAction(String roomId, String action) {
        ApiResult<GameActionData> res = new ApiResult<>(new GameActionData());
        Map<String, Object> body = new LinkedHashMap<>();
        body.put("action", action != null ? action : "");
        RawResponse raw = doPost("/api/v1/games/" + urlEncode(roomId) + "/action", body);
        if (!raw.transportOk || raw.status != 200 || raw.json == null) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        res.data.action = OctiMiniJson.getString(raw.json, "action", action != null ? action : "");
        res.data.roomId = OctiMiniJson.getString(raw.json, "room_id", roomId != null ? roomId : "");
        res.data.gameOver = OctiMiniJson.getBoolean(raw.json, "game_over", false);
        res.data.seq = OctiMiniJson.getLong(raw.json, "seq", 0);
        return res;
    }

    public ApiResult<GameActionData> submitAction(String roomId, String action) {
        return submitGameAction(roomId, action);
    }

    public ApiResult<Long> postGameChat(String roomId, String scope, String text) {
        ApiResult<Long> res = new ApiResult<>(0L);
        String sc = (scope == null || scope.isEmpty()) ? "all" : scope.toLowerCase(Locale.ROOT);
        Map<String, Object> body = new LinkedHashMap<>();
        body.put("scope", sc);
        body.put("text", text != null ? text : "");
        RawResponse raw = doPost("/api/v1/games/" + urlEncode(roomId) + "/chat", body);
        if (!raw.transportOk || raw.status != 200 || raw.json == null) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        res.data = OctiMiniJson.getLong(raw.json, "seq", 0);
        return res;
    }

    public ApiResult<Long> submitVoiceSignal(
            String roomId, String toPlayerUuid, String signalType, String payloadJson) {
        ApiResult<Long> res = new ApiResult<>(0L);
        Map<String, Object> body = new LinkedHashMap<>();
        body.put("target", toPlayerUuid != null ? toPlayerUuid : "");
        body.put("type", signalType != null ? signalType : "");
        Object parsedPayload = null;
        if (payloadJson != null && !payloadJson.isEmpty()) {
            try {
                parsedPayload = OctiMiniJson.parse(payloadJson);
            } catch (RuntimeException ignored) {
            }
        }
        body.put("payload", parsedPayload != null ? parsedPayload : (payloadJson != null ? payloadJson : ""));
        RawResponse raw = doPost("/api/v1/games/" + urlEncode(roomId) + "/voice-signal", body);
        if (!raw.transportOk || raw.status != 200 || raw.json == null) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        res.data = OctiMiniJson.getLong(raw.json, "seq", 0);
        return res;
    }

    public ApiResult<Long> submitVoiceMeta(String roomId, String eventType) {
        return submitVoiceMeta(roomId, eventType, "");
    }

    public ApiResult<Long> submitVoiceMeta(String roomId, String eventType, String targetPlayerUuid) {
        ApiResult<Long> res = new ApiResult<>(0L);
        String action = eventType != null ? eventType.toUpperCase(Locale.ROOT) : "";
        if ("SPEAKING_START".equals(action)) {
            action = "MIC_UNMUTED";
        } else if ("SPEAKING_STOP".equals(action)) {
            action = "MIC_MUTED";
        }
        Map<String, Object> body = new LinkedHashMap<>();
        body.put("action", action);
        RawResponse raw = doPost("/api/v1/games/" + urlEncode(roomId) + "/voice-meta", body);
        if (!raw.transportOk || raw.status != 200 || raw.json == null) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        res.data = OctiMiniJson.getLong(raw.json, "seq", 0);
        return res;
    }

    public ApiResult<Boolean> banObserver(String roomId, String targetPlayerUuid) {
        return banObserver(roomId, targetPlayerUuid, "");
    }

    public ApiResult<Boolean> banObserver(String roomId, String targetPlayerUuid, String reason) {
        ApiResult<Boolean> res = new ApiResult<>(Boolean.FALSE);
        Map<String, Object> body = new LinkedHashMap<>();
        body.put("target", targetPlayerUuid != null ? targetPlayerUuid : "");
        if (reason != null && !reason.isEmpty()) {
            body.put("reason", reason);
        }
        RawResponse raw = doPost("/api/v1/games/" + urlEncode(roomId) + "/ban", body);
        if (!raw.transportOk || raw.status != 200) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        res.data = Boolean.TRUE;
        return res;
    }

    public ApiResult<Boolean> muteParticipant(
            String roomId, String targetPlayerUuid, boolean muteChat, boolean muteVoice) {
        ApiResult<Boolean> res = new ApiResult<>(Boolean.FALSE);
        Map<String, Object> body = new LinkedHashMap<>();
        body.put("target", targetPlayerUuid != null ? targetPlayerUuid : "");
        body.put("mute_chat", muteChat);
        body.put("mute_voice", muteVoice);
        RawResponse raw = doPost("/api/v1/games/" + urlEncode(roomId) + "/mute", body);
        if (!raw.transportOk || raw.status != 200) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        res.data = Boolean.TRUE;
        return res;
    }

    public ApiResult<RoomSettingsData> updateRoomSettings(
            String roomId, String observerPolicy, String voicePolicy) {
        ApiResult<RoomSettingsData> res = new ApiResult<>(new RoomSettingsData());
        Map<String, Object> body = new LinkedHashMap<>();
        if (observerPolicy != null && !observerPolicy.isEmpty()) {
            body.put("observer_policy", normalizeObserverPolicy(observerPolicy));
        }
        if (voicePolicy != null && !voicePolicy.isEmpty()) {
            body.put("voice_policy", normalizeVoicePolicy(voicePolicy));
        }
        RawResponse raw = doPost("/api/v1/games/" + urlEncode(roomId) + "/settings", body);
        if (!raw.transportOk || raw.status != 200 || raw.json == null) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        res.data.observerPolicy = OctiMiniJson.getString(
                raw.json, "observer_policy", observerPolicy != null ? observerPolicy : "");
        res.data.voicePolicy = OctiMiniJson.getString(
                raw.json, "voice_policy", voicePolicy != null ? voicePolicy : "");
        res.data.seq = OctiMiniJson.getLong(raw.json, "seq", 0);
        return res;
    }

    private static String normalizeObserverPolicy(String policy) {
        if (policy == null || policy.isEmpty()) {
            return "OPEN";
        }
        String u = policy.trim().toUpperCase(Locale.ROOT);
        if ("PUBLIC".equals(u) || "ALL".equals(u)) {
            return "OPEN";
        }
        if ("SILENT".equals(u) || "MUTED".equals(u)) {
            return "SILENT_OBSERVERS";
        }
        if ("PRIVATE".equals(u) || "NONE".equals(u) || "DISABLED".equals(u)) {
            return "NO_OBSERVERS";
        }
        return u;
    }

    private static String normalizeVoicePolicy(String policy) {
        if (policy == null || policy.isEmpty()) {
            return "PLAYERS_ONLY";
        }
        String u = policy.trim().toUpperCase(Locale.ROOT);
        if ("PLAYERS_AND_OBSERVERS".equals(u) || "ALL".equals(u) || "OPEN".equals(u)) {
            return "ALL_ALLOWED";
        }
        if ("OFF".equals(u) || "NONE".equals(u) || "MUTED".equals(u)) {
            return "DISABLED";
        }
        return u;
    }

    /* ================================================================== */
    /* 33..35: Game Archive & .octi Transcript                            */
    /* ================================================================== */

    public ApiResult<ArchivedGamesPage> listArchivedGames() {
        return listArchivedGames("", "", 20, 0);
    }

    public ApiResult<ArchivedGamesPage> listArchivedGames(int limit, int offset) {
        return listArchivedGames("", "", limit, offset);
    }

    public ApiResult<ArchivedGamesPage> listArchivedGames(
            String mode, String playerUuid, int limit, int offset) {
        ApiResult<ArchivedGamesPage> res = new ApiResult<>(new ArchivedGamesPage());
        StringBuilder path = new StringBuilder("/api/v1/archive/games?limit=")
                .append(limit)
                .append("&offset=")
                .append(offset);
        if (mode != null && !mode.isEmpty()) {
            path.append("&mode=").append(urlEncode(mode));
        }
        if (playerUuid != null && !playerUuid.isEmpty()) {
            path.append("&player=").append(urlEncode(playerUuid));
        }
        RawResponse raw = doGet(path.toString());
        if (!raw.transportOk || raw.status != 200 || raw.json == null) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        res.data.total = OctiMiniJson.getLong(raw.json, "total", 0);
        res.data.limit = OctiMiniJson.getInt(raw.json, "limit", limit);
        res.data.offset = OctiMiniJson.getInt(raw.json, "offset", offset);
        List<Object> arr = OctiMiniJson.getArray(raw.json, "games");
        if (arr != null) {
            for (Object item : arr) {
                if (item instanceof Map<?, ?>) {
                    @SuppressWarnings("unchecked")
                    Map<String, Object> m = (Map<String, Object>) item;
                    res.data.games.add(parseArchivedGameSummary(m));
                }
            }
        }
        return res;
    }

    public ApiResult<ArchivedGameSummary> fetchArchivedGame(String gameId) {
        ApiResult<ArchivedGameSummary> res = new ApiResult<>(new ArchivedGameSummary());
        RawResponse raw = doGet("/api/v1/archive/games/" + urlEncode(gameId));
        if (!raw.transportOk || raw.status != 200 || raw.json == null) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        res.data = parseArchivedGameSummary(raw.json);
        return res;
    }

    public ApiResult<String> downloadOctiTranscript(String gameId) {
        ApiResult<String> res = new ApiResult<>("");
        RawResponse raw = doGet("/api/v1/archive/games/" + urlEncode(gameId) + "/transcript");
        if (!raw.transportOk || raw.status != 200) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        res.data = raw.body != null ? raw.body : "";
        return res;
    }

    public ApiResult<UploadTranscriptResponse> uploadTrainingTranscript(UploadTranscriptRequest req) {
        ApiResult<UploadTranscriptResponse> res = new ApiResult<>(new UploadTranscriptResponse());
        if (req == null) {
            res.errorCode = "INVALID_REQUEST";
            res.errorMessage = "null request";
            return res;
        }

        Map<String, Object> body = new LinkedHashMap<>();
        if (req.transcriptJson != null && req.transcriptJson.trim().startsWith("{")) {
            try {
                Object parsed = OctiMiniJson.parse(req.transcriptJson);
                body.put("transcript", parsed != null ? parsed : req.transcriptJson);
            } catch (RuntimeException ignored) {
                body.put("transcript", req.transcriptJson);
            }
        } else {
            body.put("transcript", req.transcriptJson != null ? req.transcriptJson : "");
        }
        if (req.clientId != null && !req.clientId.trim().isEmpty()) {
            body.put("clientId", req.clientId);
        }

        RawResponse raw = doPost("/api/v1/archive/upload", body);
        if (!raw.transportOk || raw.status != 200) {
            populateError(raw, res);
            return res;
        }

        res.ok = true;
        res.httpStatus = raw.status;
        if (raw.json != null) {
            Object gid = raw.json.get("game_id");
            if (gid == null) gid = raw.json.get("gameId");
            if (gid != null) res.data.gameId = gid.toString();

            Object pc = raw.json.get("ply_count");
            if (pc == null) pc = raw.json.get("plyCount");
            if (pc instanceof Number n) res.data.plyCount = n.intValue();
        }
        return res;
    }

    public ApiResult<UploadTranscriptResponse> uploadTrainingTranscript(String transcriptJson, String clientId) {
        return uploadTrainingTranscript(new UploadTranscriptRequest(transcriptJson, clientId));
    }

    public ApiResult<Integer> deleteInstallArchiveData(String installId) {
        ApiResult<Integer> res = new ApiResult<>(0);
        RawResponse raw = doDelete("/api/v1/archive/install/" + urlEncode(installId != null ? installId : ""));
        if (!raw.transportOk || raw.status != 200) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        if (raw.json != null) {
            Object del = raw.json.get("deleted");
            if (del instanceof Number n) {
                res.data = n.intValue();
            }
        }
        return res;
    }

    /* ================================================================== */
    /* Poll & Server-Sent Events (SSE) Live Streams                       */
    /* ================================================================== */

    public ApiResult<List<SseEvent>> pollLobbyEvents(long sinceSeq) {
        return pollLobbyEvents(sinceSeq, 100);
    }

    public ApiResult<List<SseEvent>> pollLobbyEvents(long sinceSeq, int limit) {
        ApiResult<List<SseEvent>> res = new ApiResult<>(new ArrayList<>());
        String path = "/api/v1/lobby/events?since_seq=" + Math.max(0L, sinceSeq)
                + "&limit=" + Math.max(1, limit);
        RawResponse raw = doGet(path);
        if (!raw.transportOk || raw.status != 200 || raw.json == null) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        List<Object> arr = OctiMiniJson.getArray(raw.json, "events");
        if (arr != null) {
            for (Object item : arr) {
                if (!(item instanceof Map<?, ?>)) {
                    continue;
                }
                @SuppressWarnings("unchecked")
                Map<String, Object> m = (Map<String, Object>) item;
                SseEvent ev = new SseEvent();
                ev.id = OctiMiniJson.getLong(m, "seq", 0);
                ev.event = OctiMiniJson.getString(m, "type", "");
                ev.dataJson = OctiMiniJson.stringify(m);
                res.data.add(ev);
            }
        }
        return res;
    }

    public ApiResult<List<SseEvent>> pollGameEvents(String roomId, long sinceSeq) {
        return pollGameEvents(roomId, sinceSeq, 100);
    }

    public ApiResult<List<SseEvent>> pollGameEvents(String roomId, long sinceSeq, int limit) {
        ApiResult<List<SseEvent>> res = new ApiResult<>(new ArrayList<>());
        String path = "/api/v1/games/" + urlEncode(roomId)
                + "/events?since_seq=" + Math.max(0L, sinceSeq)
                + "&limit=" + Math.max(1, limit);
        RawResponse raw = doGet(path);
        if (!raw.transportOk || raw.status != 200 || raw.json == null) {
            populateError(raw, res);
            return res;
        }
        res.ok = true;
        res.httpStatus = raw.status;
        List<Object> arr = OctiMiniJson.getArray(raw.json, "events");
        if (arr != null) {
            for (Object item : arr) {
                if (!(item instanceof Map<?, ?>)) {
                    continue;
                }
                @SuppressWarnings("unchecked")
                Map<String, Object> m = (Map<String, Object>) item;
                SseEvent ev = new SseEvent();
                ev.id = OctiMiniJson.getLong(m, "seq", 0);
                ev.event = OctiMiniJson.getString(m, "type", "");
                ev.dataJson = OctiMiniJson.stringify(m);
                res.data.add(ev);
            }
        }
        return res;
    }

    public void startLobbyEventStream(SseEventListener onEvent) {
        startLobbyEventStream(0L, onEvent);
    }

    public void startLobbyEventStream(long sinceSeq, SseEventListener onEvent) {
        launchSseStream(_lobbyStream, "/api/v1/lobby/events", sinceSeq, onEvent);
    }

    public void stopLobbyEventStream() {
        _lobbyStream.stop();
    }

    public boolean isLobbyEventStreamActive() {
        return _lobbyStream.active.get();
    }

    public long lastLobbyEventId() {
        return _lobbyStream.lastId.get();
    }

    public void startGameEventStream(String roomId, SseEventListener onEvent) {
        startGameEventStream(roomId, 0L, onEvent);
    }

    public void startGameEventStream(String roomId, long sinceSeq, SseEventListener onEvent) {
        launchSseStream(
                _gameStream,
                "/api/v1/games/" + urlEncode(roomId) + "/events",
                sinceSeq,
                onEvent);
    }

    public void stopGameEventStream() {
        _gameStream.stop();
    }

    public boolean isGameEventStreamActive() {
        return _gameStream.active.get();
    }

    public long lastGameEventId() {
        return _gameStream.lastId.get();
    }

    public void stopEventStreams() {
        stopLobbyEventStream();
        stopGameEventStream();
    }

    public static List<SseEvent> parseSseBuffer(StringBuilder buffer) {
        List<SseEvent> events = new ArrayList<>();
        if (buffer == null || buffer.isEmpty()) {
            return events;
        }

        while (true) {
            int sepPos = -1;
            int sepLen = 0;
            int n = buffer.length();
            for (int i = 0; i + 1 < n; ++i) {
                char c0 = buffer.charAt(i);
                char c1 = buffer.charAt(i + 1);
                if (c0 == '\n' && c1 == '\n') {
                    sepPos = i;
                    sepLen = 2;
                    break;
                }
                if (i + 3 < n && c0 == '\r' && c1 == '\n'
                        && buffer.charAt(i + 2) == '\r' && buffer.charAt(i + 3) == '\n') {
                    sepPos = i;
                    sepLen = 4;
                    break;
                }
            }
            if (sepPos < 0) {
                break;
            }

            String rawBlock = buffer.substring(0, sepPos);
            buffer.delete(0, sepPos + sepLen);

            SseEvent ev = new SseEvent();
            boolean hasEventOrData = false;
            StringBuilder dataBuf = new StringBuilder();
            String[] lines = rawBlock.split("\n", -1);
            for (String rawLine : lines) {
                String line = rawLine.endsWith("\r")
                        ? rawLine.substring(0, rawLine.length() - 1)
                        : rawLine;
                if (line.isEmpty() || line.charAt(0) == ':') {
                    continue;
                }
                if (line.startsWith("id:")) {
                    String val = line.substring(3).trim();
                    try {
                        ev.id = Long.parseUnsignedLong(val);
                    } catch (NumberFormatException ignored) {
                    }
                } else if (line.startsWith("event:")) {
                    ev.event = line.substring(6).trim();
                    hasEventOrData = true;
                } else if (line.startsWith("data:")) {
                    String val = line.substring(5);
                    if (!val.isEmpty() && val.charAt(0) == ' ') {
                        val = val.substring(1);
                    }
                    if (dataBuf.length() > 0) {
                        dataBuf.append('\n');
                    }
                    dataBuf.append(val);
                    hasEventOrData = true;
                }
            }
            ev.dataJson = dataBuf.toString();
            if (hasEventOrData && !ev.event.isEmpty()) {
                events.add(ev);
            }
        }

        return events;
    }

    /* ================================================================== */
    /* Internal HTTP/1.1 & SSE Transport                                  */
    /* ================================================================== */

    private RawResponse doGet(String path) {
        return doHttpRequest("GET", path, null);
    }

    private RawResponse doPost(String path, Map<String, Object> body) {
        String jsonBody = OctiMiniJson.stringify(body != null ? body : Collections.emptyMap());
        return doHttpRequest("POST", path, jsonBody);
    }

    private RawResponse doPatch(String path, Map<String, Object> body) {
        String jsonBody = OctiMiniJson.stringify(body != null ? body : Collections.emptyMap());
        return doHttpRequest("PATCH", path, jsonBody);
    }

    private RawResponse doDelete(String path) {
        return doHttpRequest("DELETE", path, null);
    }

    private RawResponse doHttpRequest(String method, String path, String jsonBody) {
        RawResponse r = new RawResponse();
        byte[] bodyBytes = jsonBody != null ? jsonBody.getBytes(StandardCharsets.UTF_8) : null;
        String token = sessionToken();

        for (int attempt = 1; attempt <= 5; attempt++) {
            r = new RawResponse();
            try (Socket sock = new Socket()) {
                sock.connect(new InetSocketAddress(_host, _port), 5000);
                sock.setSoTimeout(10000);

                OutputStream out = sock.getOutputStream();
                InputStream in = sock.getInputStream();

                StringBuilder req = new StringBuilder(256);
                req.append(method).append(' ').append(path).append(" HTTP/1.1\r\n");
                req.append("Host: ").append(_host).append(':').append(_port).append("\r\n");
                req.append("User-Agent: octi-android/1.0\r\n");
                req.append("Accept: application/json, text/plain, */*\r\n");
                req.append("Connection: close\r\n");
                req.append("X-Octi-Protocol-Version: ").append(OctiDefs.OCTI_PROTOCOL_VERSION).append("\r\n");
                if (!token.isEmpty()) {
                    req.append("Authorization: Bearer ").append(token).append("\r\n");
                }
                if (bodyBytes != null) {
                    req.append("Content-Type: application/json\r\n");
                    req.append("Content-Length: ").append(bodyBytes.length).append("\r\n");
                }
                req.append("\r\n");

                out.write(req.toString().getBytes(StandardCharsets.US_ASCII));
                if (bodyBytes != null && bodyBytes.length > 0) {
                    out.write(bodyBytes);
                }
                out.flush();

                String statusLine = readHttpLine(in);
                if (statusLine == null || statusLine.isEmpty()) {
                    r.transportOk = false;
                    r.transportError = "empty HTTP response";
                    if (attempt < 5) {
                        try {
                            Thread.sleep(100L * attempt);
                        } catch (InterruptedException ie) {
                            Thread.currentThread().interrupt();
                            return r;
                        }
                        continue;
                    }
                    return r;
                }
                String[] stParts = statusLine.split(" ", 3);
                if (stParts.length < 2) {
                    r.transportOk = false;
                    r.transportError = "malformed status line: " + statusLine;
                    return r;
                }
                r.status = Integer.parseInt(stParts[1].trim());
                r.transportOk = true;

                int contentLength = -1;
                boolean chunked = false;
                String line;
                while ((line = readHttpLine(in)) != null && !line.isEmpty()) {
                    int colon = line.indexOf(':');
                    if (colon <= 0) {
                        continue;
                    }
                    String hName = line.substring(0, colon).trim().toLowerCase(Locale.ROOT);
                    String hVal = line.substring(colon + 1).trim();
                    switch (hName) {
                        case "content-type":
                            r.contentType = hVal;
                            break;
                        case "content-length":
                            try {
                                contentLength = Integer.parseInt(hVal);
                            } catch (NumberFormatException ignored) {
                            }
                            break;
                        case "transfer-encoding":
                            if (hVal.toLowerCase(Locale.ROOT).contains("chunked")) {
                                chunked = true;
                            }
                            break;
                        case "retry-after":
                            try {
                                r.retryAfterSec = Math.max(0, Integer.parseInt(hVal));
                            } catch (NumberFormatException ignored) {
                            }
                            break;
                        default:
                            break;
                    }
                }

                byte[] respBytes;
                if (chunked) {
                    respBytes = readChunkedBody(in);
                } else if (contentLength >= 0) {
                    respBytes = readExactBytes(in, contentLength);
                } else {
                    respBytes = readUntilEof(in);
                }
                r.body = new String(respBytes, StandardCharsets.UTF_8);
                String trimmed = r.body.trim();
                if (!trimmed.isEmpty() && (trimmed.charAt(0) == '{' || trimmed.charAt(0) == '[')) {
                    r.json = OctiMiniJson.parseObjectOrNull(trimmed);
                }
                return r;
            } catch (Exception e) {
                r.transportOk = false;
                r.transportError = e.getMessage() != null ? e.getMessage() : e.getClass().getSimpleName();
                if (attempt < 5) {
                    try {
                        Thread.sleep(100L * attempt);
                    } catch (InterruptedException ie) {
                        Thread.currentThread().interrupt();
                        return r;
                    }
                    continue;
                }
                return r;
            }
        }
        return r;
    }

    private void launchSseStream(
            SseRunner runner, String basePath, long sinceSeq, SseEventListener onEvent) {
        runner.stop();
        runner.stopRequested.set(false);
        runner.lastId.set(Math.max(0L, sinceSeq));
        runner.active.set(true);

        Thread worker = new Thread(() -> {
            while (!runner.stopRequested.get()) {
                long curSeq = runner.lastId.get();
                String sep = basePath.indexOf('?') < 0 ? "?stream=1&since_seq=" : "&stream=1&since_seq=";
                String path = basePath + sep + curSeq;
                String token = sessionToken();

                int httpStatus = 0;
                boolean terminalEventSeen = false;
                Socket sock = new Socket();
                synchronized (runner.mutex) {
                    if (runner.stopRequested.get()) {
                        try {
                            sock.close();
                        } catch (IOException ignored) {
                        }
                        break;
                    }
                    runner.activeSocket = sock;
                }

                try {
                    sock.connect(new InetSocketAddress(_host, _port), 5000);
                    sock.setSoTimeout(30000);

                    OutputStream out = sock.getOutputStream();
                    InputStream in = sock.getInputStream();

                    StringBuilder req = new StringBuilder(256);
                    req.append("GET ").append(path).append(" HTTP/1.1\r\n");
                    req.append("Host: ").append(_host).append(':').append(_port).append("\r\n");
                    req.append("Accept: text/event-stream\r\n");
                    req.append("Cache-Control: no-cache\r\n");
                    req.append("Connection: close\r\n");
                    req.append("X-Octi-Protocol-Version: ").append(OctiDefs.OCTI_PROTOCOL_VERSION).append("\r\n");
                    if (!token.isEmpty()) {
                        req.append("Authorization: Bearer ").append(token).append("\r\n");
                    }
                    if (curSeq > 0) {
                        req.append("Last-Event-ID: ").append(curSeq).append("\r\n");
                    }
                    req.append("\r\n");
                    out.write(req.toString().getBytes(StandardCharsets.US_ASCII));
                    out.flush();

                    String statusLine = readHttpLine(in);
                    if (statusLine != null && !statusLine.isEmpty()) {
                        String[] parts = statusLine.split(" ", 3);
                        if (parts.length >= 2) {
                            httpStatus = Integer.parseInt(parts[1].trim());
                        }
                    }

                    boolean chunked = false;
                    String hLine;
                    while ((hLine = readHttpLine(in)) != null && !hLine.isEmpty()) {
                        int colon = hLine.indexOf(':');
                        if (colon > 0) {
                            String hName = hLine.substring(0, colon).trim().toLowerCase(Locale.ROOT);
                            String hVal = hLine.substring(colon + 1).trim().toLowerCase(Locale.ROOT);
                            if ("transfer-encoding".equals(hName) && hVal.contains("chunked")) {
                                chunked = true;
                            }
                        }
                    }

                    if (httpStatus == 200) {
                        StringBuilder chunkBuffer = new StringBuilder();
                        if (chunked) {
                            while (!runner.stopRequested.get()) {
                                String sizeLine = readHttpLine(in);
                                if (sizeLine == null) {
                                    break;
                                }
                                int semi = sizeLine.indexOf(';');
                                String hexStr = (semi >= 0 ? sizeLine.substring(0, semi) : sizeLine).trim();
                                if (hexStr.isEmpty()) {
                                    continue;
                                }
                                int chunkLen = Integer.parseInt(hexStr, 16);
                                if (chunkLen <= 0) {
                                    readHttpLine(in);
                                    break;
                                }
                                byte[] chunkData = readExactBytes(in, chunkLen);
                                readHttpLine(in);
                                chunkBuffer.append(new String(chunkData, StandardCharsets.UTF_8));
                                if (dispatchParsedSseEvents(runner, chunkBuffer, onEvent)) {
                                    terminalEventSeen = true;
                                    break;
                                }
                            }
                        } else {
                            byte[] buf = new byte[1024];
                            int n;
                            while (!runner.stopRequested.get() && (n = in.read(buf)) != -1) {
                                if (n > 0) {
                                    chunkBuffer.append(new String(buf, 0, n, StandardCharsets.UTF_8));
                                    if (dispatchParsedSseEvents(runner, chunkBuffer, onEvent)) {
                                        terminalEventSeen = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                } catch (Exception ignored) {
                } finally {
                    synchronized (runner.mutex) {
                        runner.activeSocket = null;
                    }
                    try {
                        sock.close();
                    } catch (IOException ignored) {
                    }
                }

                if (runner.stopRequested.get() || terminalEventSeen) {
                    break;
                }
                if (httpStatus >= 400 && httpStatus < 500 && httpStatus != 429) {
                    break;
                }

                synchronized (runner.mutex) {
                    if (!runner.stopRequested.get()) {
                        try {
                            runner.mutex.wait(150);
                        } catch (InterruptedException e) {
                            Thread.currentThread().interrupt();
                            break;
                        }
                    }
                }
            }
            runner.active.set(false);
        }, "octi-sse-worker");
        worker.setDaemon(true);
        runner.worker = worker;
        worker.start();
    }

    private static boolean dispatchParsedSseEvents(
            SseRunner runner, StringBuilder chunkBuffer, SseEventListener onEvent) {
        boolean terminalSeen = false;
        List<SseEvent> parsed = parseSseBuffer(chunkBuffer);
        for (SseEvent ev : parsed) {
            if (ev.id > runner.lastId.get()) {
                runner.lastId.set(ev.id);
            }
            if ("closed".equals(ev.event)) {
                terminalSeen = true;
            }
            if (onEvent != null && !runner.stopRequested.get()) {
                onEvent.onEvent(ev);
            }
        }
        return terminalSeen;
    }

    private static <T> void populateError(RawResponse raw, ApiResult<T> out) {
        out.ok = false;
        out.httpStatus = raw.status;
        out.retryAfterSec = raw.retryAfterSec;
        if (!raw.transportOk) {
            out.errorCode = "TRANSPORT_ERROR";
            out.errorMessage = raw.transportError != null ? raw.transportError : "transport failure";
            return;
        }
        if (raw.json != null) {
            out.errorCode = OctiMiniJson.getString(raw.json, "error", "HTTP_ERROR");
            out.errorMessage = OctiMiniJson.getString(raw.json, "message", "");
        } else {
            out.errorCode = "HTTP_" + raw.status;
            out.errorMessage = raw.body != null ? raw.body : "";
        }
    }

    private static String urlEncode(String val) {
        if (val == null || val.isEmpty()) {
            return "";
        }
        return URLEncoder.encode(val, StandardCharsets.UTF_8).replace("+", "%20");
    }

    private static String readHttpLine(InputStream in) throws IOException {
        ByteArrayOutputStream baos = new ByteArrayOutputStream(128);
        int b;
        boolean sawByte = false;
        while ((b = in.read()) != -1) {
            sawByte = true;
            if (b == '\n') {
                break;
            }
            if (b != '\r') {
                baos.write(b);
            }
        }
        if (!sawByte && baos.size() == 0) {
            return null;
        }
        return baos.toString(StandardCharsets.UTF_8);
    }

    private static byte[] readExactBytes(InputStream in, int length) throws IOException {
        byte[] buf = new byte[length];
        int offset = 0;
        while (offset < length) {
            int n = in.read(buf, offset, length - offset);
            if (n < 0) {
                throw new EOFException("unexpected EOF reading HTTP body");
            }
            offset += n;
        }
        return buf;
    }

    private static byte[] readChunkedBody(InputStream in) throws IOException {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        while (true) {
            String sizeLine = readHttpLine(in);
            if (sizeLine == null) {
                break;
            }
            int semi = sizeLine.indexOf(';');
            String hex = (semi >= 0 ? sizeLine.substring(0, semi) : sizeLine).trim();
            if (hex.isEmpty()) {
                continue;
            }
            int chunkSize = Integer.parseInt(hex, 16);
            if (chunkSize <= 0) {
                readHttpLine(in);
                break;
            }
            byte[] chunk = readExactBytes(in, chunkSize);
            baos.write(chunk);
            readHttpLine(in);
        }
        return baos.toByteArray();
    }

    private static byte[] readUntilEof(InputStream in) throws IOException {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        byte[] buf = new byte[4096];
        int n;
        while ((n = in.read(buf)) != -1) {
            baos.write(buf, 0, n);
        }
        return baos.toByteArray();
    }

    /* ================================================================== */
    /* JSON Model Parsers                                                 */
    /* ================================================================== */

    private static LinkedIdentityInfo parseLinkedIdentity(Map<String, Object> j) {
        LinkedIdentityInfo info = new LinkedIdentityInfo();
        if (j == null) {
            return info;
        }
        info.provider = OctiMiniJson.getString(j, "provider", "");
        info.providerEmail = OctiMiniJson.getString(j, "provider_email", "");
        info.displayHint = OctiMiniJson.getString(j, "display_hint", "");
        info.linkedAt = OctiMiniJson.getLong(j, "linked_at", 0);
        info.lastLoginAt = OctiMiniJson.getLong(j, "last_login_at", 0);
        return info;
    }

    private static List<LinkedIdentityInfo> parseLinkedIdentities(List<Object> arr) {
        List<LinkedIdentityInfo> out = new ArrayList<>();
        if (arr == null) {
            return out;
        }
        for (Object item : arr) {
            if (item instanceof Map<?, ?>) {
                @SuppressWarnings("unchecked")
                Map<String, Object> m = (Map<String, Object>) item;
                out.add(parseLinkedIdentity(m));
            }
        }
        return out;
    }

    private static ArchivedGameSummary parseArchivedGameSummary(Map<String, Object> j) {
        ArchivedGameSummary g = new ArchivedGameSummary();
        if (j == null) {
            return g;
        }
        g.gameId = OctiMiniJson.getString(
                j, "game_id", OctiMiniJson.getString(j, "room_id", ""));
        g.mode = OctiMiniJson.getString(j, "mode", "2P");
        g.bases = OctiMiniJson.getInt(j, "bases", 3);
        g.superProngs = OctiMiniJson.getBoolean(j, "super_prongs", false);
        g.edgeless = OctiMiniJson.getBoolean(j, "edgeless", false);
        g.timeControl = OctiMiniJson.getString(j, "time_control", "untimed");
        List<Object> pl = OctiMiniJson.getArray(j, "players");
        if (pl != null) {
            for (int i = 0; i < pl.size(); ++i) {
                Object item = pl.get(i);
                if (!(item instanceof Map<?, ?>)) {
                    continue;
                }
                @SuppressWarnings("unchecked")
                Map<String, Object> pm = (Map<String, Object>) item;
                ArchivedGamePlayer p = new ArchivedGamePlayer();
                p.seat = OctiMiniJson.getInt(pm, "seat", i + 1);
                p.handle = OctiMiniJson.getString(pm, "handle", "");
                p.playerUuid = OctiMiniJson.getString(pm, "player_uuid", "");
                g.players.add(p);
            }
        }
        g.result = OctiMiniJson.getString(j, "result", "");
        g.winnerSeat = OctiMiniJson.getInt(j, "winner_seat", 0);
        g.termination = OctiMiniJson.getString(j, "termination", "");
        g.plyCount = OctiMiniJson.getInt(j, "ply_count", 0);
        g.startedAt = OctiMiniJson.getLong(j, "started_at", 0);
        g.endedAt = OctiMiniJson.getLong(j, "ended_at", 0);
        g.finalFen = OctiMiniJson.getString(
                j, "final_fen", OctiMiniJson.getString(j, "fen", ""));
        g.octiTranscript = OctiMiniJson.getString(j, "octi_transcript", "");
        return g;
    }

    private static PlayerProfileInfo parsePlayerProfile(Map<String, Object> j) {
        PlayerProfileInfo p = new PlayerProfileInfo();
        if (j == null) {
            return p;
        }
        p.uuid = OctiMiniJson.getString(j, "uuid", "");
        p.handle = OctiMiniJson.getString(j, "handle", "");
        p.displayName = OctiMiniJson.getString(j, "display_name", "");
        p.avatarUrl = OctiMiniJson.getString(j, "avatar_url", "");
        p.elo2p = OctiMiniJson.getInt(j, "elo_2p", 1200);
        p.elo4p = OctiMiniJson.getInt(j, "elo_4p", 1200);
        p.eloOfk = OctiMiniJson.getInt(j, "elo_ofk", 1200);
        p.gamesPlayed = OctiMiniJson.getInt(j, "games_played", 0);
        p.wins = OctiMiniJson.getInt(j, "wins", 0);
        p.losses = OctiMiniJson.getInt(j, "losses", 0);
        p.draws = OctiMiniJson.getInt(j, "draws", 0);
        p.createdAt = OctiMiniJson.getLong(j, "created_at", 0);
        p.lastSeenAt = OctiMiniJson.getLong(j, "last_seen_at", 0);
        List<Object> ids = OctiMiniJson.getArray(j, "linked_identities");
        if (ids != null) {
            p.identities = parseLinkedIdentities(ids);
        }
        List<Object> rg = OctiMiniJson.getArray(j, "recent_games");
        if (rg != null) {
            for (Object item : rg) {
                if (item instanceof Map<?, ?>) {
                    @SuppressWarnings("unchecked")
                    Map<String, Object> m = (Map<String, Object>) item;
                    p.recentGames.add(parseArchivedGameSummary(m));
                }
            }
        }
        return p;
    }

    private static TableSummaryInfo parseTableSummary(Map<String, Object> j) {
        TableSummaryInfo t = new TableSummaryInfo();
        if (j == null) {
            return t;
        }
        t.roomId = OctiMiniJson.getString(j, "room_id", "");
        t.mode = OctiMiniJson.getString(j, "mode", "2P");
        t.bases = OctiMiniJson.getInt(j, "bases", 3);
        t.superProngs = OctiMiniJson.getBoolean(j, "super_prongs", false);
        t.edgeless = OctiMiniJson.getBoolean(j, "edgeless", false);
        Map<String, Object> tc = OctiMiniJson.getObject(j, "time_control");
        if (tc != null) {
            t.timeControlEnabled = OctiMiniJson.getBoolean(tc, "enabled", false);
            t.initialMs = OctiMiniJson.getLong(tc, "initial_ms", 0);
            t.incrementMs = OctiMiniJson.getLong(tc, "increment_ms", 0);
            t.disconnectGraceMs = OctiMiniJson.getLong(tc, "disconnect_grace_ms", 90000);
        }
        t.rated = OctiMiniJson.getBoolean(j, "rated", true);
        t.allowTakeback = OctiMiniJson.getBoolean(j, "allow_takeback", false);
        t.observerPolicy = OctiMiniJson.getString(j, "observer_policy", "OPEN");
        t.voicePolicy = OctiMiniJson.getString(j, "voice_policy", "PLAYERS_ONLY");
        t.phase = OctiMiniJson.getString(j, "phase", "WAITING_FOR_PLAYERS");
        t.hasPasscode = OctiMiniJson.getBoolean(j, "has_passcode", false);
        t.hostHandle = OctiMiniJson.getString(j, "host_handle", "");
        t.seatsTotal = OctiMiniJson.getInt(j, "seats_total", 2);
        t.seatsFilled = OctiMiniJson.getInt(j, "seats_filled", 0);
        List<Object> pl = OctiMiniJson.getArray(j, "players");
        if (pl != null) {
            for (Object item : pl) {
                if (item instanceof String s) {
                    t.players.add(s);
                }
            }
        }
        t.observerCount = OctiMiniJson.getInt(j, "observer_count", 0);
        return t;
    }

    private static GameViewData parseGameView(Map<String, Object> j) {
        GameViewData v = new GameViewData();
        if (j == null) {
            return v;
        }
        v.live = OctiMiniJson.getBoolean(j, "live", true);
        v.roomId = OctiMiniJson.getString(
                j, "room_id", OctiMiniJson.getString(j, "game_id", ""));
        v.mode = OctiMiniJson.getString(j, "mode", "2P");
        v.bases = OctiMiniJson.getInt(j, "bases", 3);
        v.superProngs = OctiMiniJson.getBoolean(j, "super_prongs", false);
        v.edgeless = OctiMiniJson.getBoolean(j, "edgeless", false);
        Map<String, Object> tc = OctiMiniJson.getObject(j, "time_control");
        if (tc != null) {
            v.timeControlEnabled = OctiMiniJson.getBoolean(tc, "enabled", false);
            v.initialMs = OctiMiniJson.getLong(tc, "initial_ms", 0);
            v.incrementMs = OctiMiniJson.getLong(tc, "increment_ms", 0);
            v.disconnectGraceMs = OctiMiniJson.getLong(tc, "disconnect_grace_ms", 90000);
        }
        v.rated = OctiMiniJson.getBoolean(j, "rated", true);
        v.allowTakeback = OctiMiniJson.getBoolean(j, "allow_takeback", false);
        v.observerPolicy = OctiMiniJson.getString(j, "observer_policy", "OPEN");
        v.voicePolicy = OctiMiniJson.getString(j, "voice_policy", "PLAYERS_ONLY");
        v.phase = OctiMiniJson.getString(j, "phase", "WAITING_FOR_PLAYERS");
        v.paused = OctiMiniJson.getBoolean(j, "paused", false);
        v.hasPasscode = OctiMiniJson.getBoolean(j, "has_passcode", false);
        v.hostUuid = OctiMiniJson.getString(j, "host_uuid", "");
        v.viewerSeat = OctiMiniJson.getInt(j, "viewer_seat", 0);
        List<Object> seatsArr = OctiMiniJson.getArray(j, "seats");
        if (seatsArr != null) {
            for (int i = 0; i < seatsArr.size(); ++i) {
                Object item = seatsArr.get(i);
                if (!(item instanceof Map<?, ?>)) {
                    continue;
                }
                @SuppressWarnings("unchecked")
                Map<String, Object> sm = (Map<String, Object>) item;
                GameSeatInfo s = new GameSeatInfo();
                s.seat = OctiMiniJson.getInt(sm, "seat", i + 1);
                s.playerUuid = OctiMiniJson.getString(sm, "player_uuid", "");
                s.handle = OctiMiniJson.getString(sm, "handle", "");
                s.rating = OctiMiniJson.getInt(sm, "rating", 1200);
                s.connected = OctiMiniJson.getBoolean(sm, "connected", true);
                s.disconnectedForMs = OctiMiniJson.getLong(sm, "disconnected_for_ms", 0);
                s.forfeitClaimable = OctiMiniJson.getBoolean(sm, "forfeit_claimable", false);
                s.clockMs = OctiMiniJson.getLong(sm, "clock_ms", -1);
                s.eliminated = OctiMiniJson.getBoolean(sm, "eliminated", false);
                v.seats.add(s);
            }
        }
        List<Object> obsArr = OctiMiniJson.getArray(j, "observers");
        if (obsArr != null) {
            for (Object item : obsArr) {
                if (item instanceof String s) {
                    v.observers.add(s);
                }
            }
        }
        v.observerCount = OctiMiniJson.getInt(j, "observer_count", v.observers.size());
        v.fen = OctiMiniJson.getString(j, "fen", "");
        v.turnSeat = OctiMiniJson.getInt(j, "turn_seat", 0);
        v.plyCount = OctiMiniJson.getInt(j, "ply_count", 0);
        List<Object> movesArr = OctiMiniJson.getArray(j, "moves");
        if (movesArr != null) {
            for (Object item : movesArr) {
                if (item instanceof String s) {
                    v.moves.add(s);
                }
            }
        }
        v.result = OctiMiniJson.getString(j, "result", "");
        v.termination = OctiMiniJson.getString(j, "termination", "");
        v.winnerSeat = OctiMiniJson.getInt(j, "winner_seat", 0);
        List<Object> propArr = OctiMiniJson.getArray(j, "proposals");
        if (propArr != null) {
            for (Object item : propArr) {
                if (!(item instanceof Map<?, ?>)) {
                    continue;
                }
                @SuppressWarnings("unchecked")
                Map<String, Object> pm = (Map<String, Object>) item;
                GameProposalInfo p = new GameProposalInfo();
                p.type = OctiMiniJson.getString(pm, "type", "");
                p.proposerUuid = OctiMiniJson.getString(pm, "proposer_uuid", "");
                List<Object> acc = OctiMiniJson.getArray(pm, "accepted_by");
                if (acc != null) {
                    for (Object a : acc) {
                        if (a instanceof String s) {
                            p.acceptedBy.add(s);
                        }
                    }
                }
                List<Object> aw = OctiMiniJson.getArray(pm, "awaiting");
                if (aw != null) {
                    for (Object a : aw) {
                        if (a instanceof String s) {
                            p.awaiting.add(s);
                        }
                    }
                }
                p.expiresAtMs = OctiMiniJson.getLong(pm, "expires_at_ms", 0);
                v.proposals.add(p);
            }
        }
        List<Object> vpArr = OctiMiniJson.getArray(j, "voice_peers");
        if (vpArr != null) {
            for (Object item : vpArr) {
                if (!(item instanceof Map<?, ?>)) {
                    continue;
                }
                @SuppressWarnings("unchecked")
                Map<String, Object> vm = (Map<String, Object>) item;
                VoicePeerInfo peer = new VoicePeerInfo();
                peer.playerUuid = OctiMiniJson.getString(vm, "player_uuid", "");
                peer.micMuted = OctiMiniJson.getBoolean(vm, "mic_muted", false);
                v.voicePeers.add(peer);
            }
        }
        v.seq = OctiMiniJson.getLong(j, "seq", 0);
        v.archived = OctiMiniJson.getBoolean(j, "archived", false);
        v.rematchRoomId = OctiMiniJson.getString(j, "rematch_room_id", "");
        v.createdAtMs = OctiMiniJson.getLong(j, "created_at_ms", 0);
        return v;
    }
}
