/*
 * OctiHttpServer.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef OCTI_SERVER_OCTI_HTTP_SERVER_HXX
#define OCTI_SERVER_OCTI_HTTP_SERVER_HXX

#include "AccountStore.hxx"
#include "GameRoomManager.hxx"
#include "OidcVerifier.hxx"

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace octi {

/* Tunable HTTP layer bounds. Defaults are production values. */
struct HttpServerConfig {
    std::string bindAddress = "127.0.0.1";
    int port = 8888;                    // 0 = pick an ephemeral port

    /* Requests with a larger body are rejected with 413. */
    size_t maxBodyBytes = 65536;

    /* Period of the maintenance tick (rooms, matchmaking, deadlines). */
    int tickIntervalMs = 1000;
    /* Period of the expired / revoked session purge. */
    int64_t sessionPurgeIntervalSec = 300;

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

    /* Worker pool: base threads, dynamic ceiling, queued connections. */
    size_t workerThreads = 24;
    size_t maxWorkerThreads = 64;
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
    int maxSseStreams = 16;
    int ssePollMs = 200;
    int sseHeartbeatMs = 15000;
    int64_t sseMaxDurationMs = 10LL * 60 * 1000;
    int sseSessionRecheckMs = 5000;
    size_t sseBatchLimit = 100;

    /*
     * Public OAuth client ids advertised by GET /api/v1/auth/providers for
     * every enabled provider. Client ids are not secrets.
     */
    std::map<std::string, std::vector<std::string> > providerClientIds;
};

/*
 * JSON REST + SSE server (cpp-httplib) in front of AccountStore,
 * OidcVerifier and GameRoomManager, plus the embedded Web Archive,
 * Leaderboard and Live Spectator UI served at GET /.
 *
 * Voice is strictly peer-to-peer: the server only relays small signaling
 * blobs and records voice meta events. Request bodies that are not
 * application/json are refused (415), so audio can never be uploaded.
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
