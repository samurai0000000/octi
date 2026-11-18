/*
 * OctiHttpServer.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef OCTI_SERVER_OCTI_HTTP_SERVER_HXX
#define OCTI_SERVER_OCTI_HTTP_SERVER_HXX

#include "AccountStore.hxx"
#include "GameRoomManager.hxx"
#include "OctiDefs.hxx"
#include "OidcVerifier.hxx"

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

struct sockaddr;

namespace octi {

/* Tunable HTTP layer bounds. Defaults are production values. */
struct HttpServerConfig {
    std::string bindAddress = "127.0.0.1";
    int port = OCTI_SERVER_DAEMON_PORT;        // 0 = pick an ephemeral port

    /* Requests with a larger body are rejected with 413. */
    size_t maxBodyBytes = 524288;

    /* Period of the maintenance tick (rooms, matchmaking, deadlines). */
    int tickIntervalMs = 1000;
    /* Period of the expired 24-hour ban purge. */
    int64_t tempBanPurgeIntervalSec = 60;
    /* Period of the expired / revoked session purge. */
    int64_t sessionPurgeIntervalSec = 300;
    /* Period of scheduled DB retention pruning (crash reports, chat logs, sessions). */
    int64_t retentionPurgeIntervalSec = 3600;
    int64_t crashRetentionAgeSec = 90LL * 86400;
    int64_t lobbyChatRetentionAgeMs = 90LL * 86400 * 1000;
    int retentionMaxRows = 5000;
    /* Period of anonymous game reconciliation across player installs. */
    int64_t reconcileIntervalSec = 6 * 3600;

    /*
     * Credential verification (POST /auth/sso and POST /auth/link) is
     * throttled per client address with a fixed window: more than
     * ssoRateLimitMax attempts inside ssoRateLimitWindowMs get 429 with
     * Retry-After. 0 disables the limit. Behind a reverse proxy every
     * client shares the proxy's address, so the proxy should rate limit as
     * well.
     */
    int ssoRateLimitMax = 30;
    int64_t ssoRateLimitWindowMs = 60 * 1000;

    /* Archive upload rate limiting per IP. */
    int uploadRateLimitMax = 6;
    int64_t uploadRateLimitWindowMs = 60 * 1000;

    /* Archive bulk export rate limiting per actor and IP. */
    int exportRateLimitMax = 12;
    int64_t exportRateLimitWindowMs = 60 * 1000;
    int robotExportConfRateLimitMax = 6;
    int64_t robotExportConfRateLimitWindowMs = 60 * 1000;

    /* Trusted reverse-proxy & public origin settings. */
    std::vector<std::string> trustedProxies = {
        "127.0.0.1", "::1", "::ffff:127.0.0.1"
    };
    std::string publicBaseUrl;
    std::vector<std::string> allowedHosts = {
        "octi.selfso.com", "trumpet.selfso.com",
        "127.0.0.1", "localhost", "[::1]"
    };
    std::string oauthStateSecret;
    size_t maxPendingOauthStates = 16384;
    std::string banConfigPath;
    bool behindTls = false;
    bool allowSyntheticOauthCallbackForTesting = false;
    bool anonymizeObserverHandles = true;
    bool disallowRobotsTxt = true;

    /* HTTP keep-alive connection bounds. */
    int keepAliveMaxCount = 100;
    int keepAliveTimeoutSec = 10;

    /* Worker pool: base threads, dynamic ceiling, queued connections. */
    size_t workerThreads = 48;
    size_t maxWorkerThreads = 160;
    size_t maxQueuedRequests = 256;
    int readTimeoutSec = 5;
    int writeTimeoutSec = 5;

    /*
     * Server-Sent Events. Every stream holds one worker thread, so the
     * number of concurrent streams is capped (and kept below half of the
     * dynamic worker ceiling). A stream never buffers: each pass fetches at
     * most sseBatchLimit events and writes them out. Streams are bounded in
     * time; clients resume with Last-Event-ID.
     */
    int maxSseStreams = 96;
    int maxSsePerIp = 6;
    int maxSsePerPlayer = 8;
    int ssePollMs = 200;
    int sseHeartbeatMs = 15000;
    int64_t sseMaxDurationMs = 10LL * 60 * 1000;
    int64_t sseAnonMaxDurationMs = 120000;
    int64_t maxEventPollWaitMs = 15000;
    int sseSessionRecheckMs = 5000;
    size_t sseBatchLimit = 100;

    /* Fixed-window / event-rate limits for auth, write, and heavy routes. */
    int authFailureRateLimitMax = 30;
    int64_t authFailureRateLimitWindowMs = 60000;
    int passcodeFailureRateLimitMax = 10;
    int64_t passcodeFailureRateLimitWindowMs = 60000;
    int probeRateLimitMax = 5;
    int64_t probeRateLimitWindowMs = 60000;
    int maxConcurrentUrlProbes = 4;
    int maxConcurrentExports = 2;
    int exportBatchDelayMsForTesting = 0;
    int pliesRateLimitMax = 30;
    int64_t pliesRateLimitWindowMs = 60000;
    int mutationRateLimitMax = 120;
    int64_t mutationRateLimitWindowMs = 60000;

    /* SEC-19: Token-bucket burst + sustained polling & read guardrails. */
    double livePollActorRefillPerSec = 8.0;     // 480 req/min per player_id or robot_id
    double livePollActorBurst = 40.0;
    double livePollAuthedIpRefillPerSec = 40.0; // shared-NAT headroom for 8+ concurrent bots/tabs
    double livePollAuthedIpBurst = 120.0;
    double livePollAnonIpRefillPerSec = 4.0;    // 240 req/min anonymous spectator
    double livePollAnonIpBurst = 25.0;

    double lobbySnapActorRefillPerSec = 3.0;    // /lobby/state, /tables
    double lobbySnapActorBurst = 20.0;
    double lobbySnapAuthedIpRefillPerSec = 15.0;
    double lobbySnapAuthedIpBurst = 60.0;
    double lobbySnapAnonIpRefillPerSec = 1.5;
    double lobbySnapAnonIpBurst = 15.0;

    double browseActorRefillPerSec = 2.0;       // /leaderboard, /robots/directory, /archive/games, /users
    double browseActorBurst = 30.0;
    double browseAuthedIpRefillPerSec = 8.0;
    double browseAuthedIpBurst = 60.0;
    double browseAnonIpRefillPerSec = 0.5;      // 30 req/min anon (blocks high-speed sequential scraping)
    double browseAnonIpBurst = 15.0;

    /*
     * Public OAuth client ids advertised by GET /api/v1/auth/providers for
     * every enabled provider. Client ids are not secrets.
     */
    std::map<std::string, std::vector<std::string> > providerClientIds;
    std::map<std::string, std::string> providerClientSecrets;

    /* Wire protocol versioning and compatibility gate. */
    int protocolVersion = OCTI_PROTOCOL_VERSION;
    int minProtocolVersion = OCTI_MIN_PROTOCOL_VERSION;
};

/*
 * JSON REST + SSE server (cpp-httplib) in front of AccountStore,
 * OidcVerifier and GameRoomManager, plus the embedded Web Archive,
 * Leaderboard and Live Spectator UI served at GET /.
 *
 * Request bodies that are not application/json are refused (415).
 *
 * start() binds and launches a listener thread and a maintenance thread
 * (GameRoomManager::tick, session purge, matchmaking pump). stop() ends
 * every stream, joins both threads and is idempotent. A stopped server
 * cannot be restarted; create a new instance.
 *
 * The three collaborators must outlive the server.
 */
class OctiHttpServer {
public:
    OctiHttpServer(AccountStore &store, OidcVerifier &verifier,
                   GameRoomManager &rooms,
                   const HttpServerConfig &config = HttpServerConfig());
    ~OctiHttpServer();

    OctiHttpServer(const OctiHttpServer &) = delete;
    OctiHttpServer &operator=(const OctiHttpServer &) = delete;

    /* Bind and serve. Returns false with a reason when binding fails. */
    bool start(std::string &error);
    void stop();

    bool isRunning() const;
    /* Bound port (resolves port 0); 0 before a successful start(). */
    int port() const;
    const std::string &bindAddress() const;

    /* Completed maintenance ticks (diagnostics / tests). */
    uint64_t tickCount() const;
    /* Event streams currently open. */
    int activeStreams() const;
    /* Archive exports currently streaming. */
    int activeExports() const;

    /* Canonicalize an IPv4, IPv6, or IPv4-mapped IPv6 literal, or "" if invalid. */
    static std::string normalizeIpLiteral(const std::string &raw);
    /* Right-to-left trusted-proxy client IP extraction. */
    static std::string resolveClientIp(
        const std::string &peerAddr,
        const std::string &xffHeader,
        const std::string &xRealIpHeader,
        const std::vector<std::string> &trustedProxies);
    /* Outbound SSRF bogon/private/loopback/embedded-IPv4 address check. */
    static bool isDisallowedProbeAddress(const struct sockaddr *sa,
                                         size_t saLen,
                                         std::string &errReason,
                                         bool allowLoopbackForTest = false);
    static bool isDisallowedProbeAddress(const std::string &ipLiteral,
                                         std::string &errReason,
                                         bool allowLoopbackForTest = false);

private:
    struct Impl;
    std::unique_ptr<Impl> _impl;
};

} // namespace octi

#endif // OCTI_SERVER_OCTI_HTTP_SERVER_HXX

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
