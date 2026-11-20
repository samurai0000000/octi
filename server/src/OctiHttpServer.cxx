/*
 * OctiHttpServer.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "OctiHttpServer.hxx"

#ifndef CPPHTTPLIB_OPENSSL_SUPPORT
#define CPPHTTPLIB_OPENSSL_SUPPORT
#endif
#include "BanManager.hxx"
#include "OctiCore.hxx"
#include "OctiBoard.hxx"
#include "OctiCrypto.hxx"
#include "OctiExceptions.hxx"
#include "OctiGame.hxx"
#include "OctiRulebook.hxx"
#include "OctiSoundCue.hxx"
#include "WebUi.hxx"

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <zlib.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <exception>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <optional>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <unordered_set>

namespace octi {

namespace {

typedef nlohmann::json Json;
typedef httplib::Request Request;
typedef httplib::Response Response;

const size_t kMaxBearerBytes = 256;
const size_t kMaxIdBytes = 64;
const size_t kMaxClientAgentBytes = 64;
const size_t kMaxCacheEntries = 20000;
const size_t kMaxQueryLimit = 500;

/* ------------------------------------------------------------------ */
/* Small helpers                                                       */
/* ------------------------------------------------------------------ */

int64_t steadyMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

int64_t epochSec()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::string dumpJson(const Json &j)
{
    return j.dump(-1, ' ', false, Json::error_handler_t::replace);
}

std::string lower(const std::string &s)
{
    std::string out = s;
    for (size_t i = 0; i < out.size(); ++i) {
        out[i] = static_cast<char>(
            std::tolower(static_cast<unsigned char>(out[i])));
    }
    return out;
}

std::string upper(const std::string &s)
{
    std::string out = s;
    for (size_t i = 0; i < out.size(); ++i) {
        out[i] = static_cast<char>(
            std::toupper(static_cast<unsigned char>(out[i])));
    }
    return out;
}

bool hasControlChars(const std::string &s)
{
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 0x20 || c == 0x7f) {
            return true;
        }
    }
    return false;
}

/* Digits only (no sign), at most 18 of them; values above max clamp. */
bool parseUnsignedClamped(const std::string &s, int64_t maxValue,
                          int64_t &out)
{
    if (s.empty() || s.size() > 18) {
        return false;
    }
    int64_t v = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] < '0' || s[i] > '9') {
            return false;
        }
        v = v * 10 + (s[i] - '0');
    }
    out = std::min(v, maxValue);
    return true;
}

/* Like parseUnsignedClamped but values above max are rejected. */
bool parseUnsignedStrict(const std::string &s, int64_t maxValue, int64_t &out)
{
    int64_t v = 0;
    if (s.empty() || s.size() > 18 || !parseUnsignedClamped(s, INT64_MAX, v) ||
        v > maxValue) {
        return false;
    }
    out = v;
    return true;
}

bool isSafeId(const std::string &s)
{
    if (s.empty() || s.size() > kMaxIdBytes) {
        return false;
    }
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (!(std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '@')) {
            return false;
        }
    }
    return true;
}

} // anonymous namespace

bool OctiHttpServer::isDisallowedProbeAddress(const struct sockaddr *sa,
                                              size_t saLen,
                                              std::string &errReason,
                                              bool allowLoopbackForTest)
{
    if (sa == nullptr) {
        errReason = "null socket address is not permitted";
        return true;
    }

    if (sa->sa_family == AF_INET) {
        if (saLen < sizeof(struct sockaddr_in)) {
            errReason = "truncated IPv4 socket address is not permitted";
            return true;
        }
        const auto *sin = reinterpret_cast<const struct sockaddr_in *>(sa);
        uint32_t ip = ntohl(sin->sin_addr.s_addr);

        /* Loopback: 127.0.0.0/8 */
        if ((ip >> 24) == 127u) {
            if (!allowLoopbackForTest) {
                errReason = "loopback IP addresses are not permitted";
                return true;
            }
            return false;
        }
        /* Unspecified ("this network"): 0.0.0.0/8 */
        if ((ip >> 24) == 0u) {
            errReason = "unspecified IPv4 address is not permitted";
            return true;
        }
        /* Private RFC 1918: 10.0.0.0/8, 172.16.0.0/12, 192.168.0.0/16 */
        if ((ip >> 24) == 10u ||
            (ip & 0xFFF00000u) == 0xAC100000u ||
            (ip & 0xFFFF0000u) == 0xC0A80000u) {
            errReason = "private network IP addresses are not permitted";
            return true;
        }
        /* CGNAT shared address space RFC 6598: 100.64.0.0/10 */
        if ((ip & 0xFFC00000u) == 0x64400000u) {
            errReason = "CGNAT shared address space (100.64.0.0/10) is not permitted";
            return true;
        }
        /* Link-local / cloud metadata RFC 3927: 169.254.0.0/16 */
        if ((ip & 0xFFFF0000u) == 0xA9FE0000u) {
            errReason = "link-local / cloud metadata IP addresses are not permitted";
            return true;
        }
        /* IETF protocol, documentation, 6to4 relay, benchmarking RFC 6890 */
        if ((ip & 0xFFFFFF00u) == 0xC0000000u ||
            (ip & 0xFFFFFF00u) == 0xC0000200u ||
            (ip & 0xFFFFFF00u) == 0xC0586300u ||
            (ip & 0xFFFE0000u) == 0xC6120000u ||
            (ip & 0xFFFFFF00u) == 0xC6336400u ||
            (ip & 0xFFFFFF00u) == 0xCB007100u) {
            errReason = "special-purpose / documentation IPv4 address is not permitted";
            return true;
        }
        /* Multicast (224.0.0.0/4), reserved Class E (240.0.0.0/4) & broadcast */
        if (ip >= 0xE0000000u) {
            errReason = "multicast, reserved, or broadcast IPv4 address is not permitted";
            return true;
        }
        return false;
    }

    if (sa->sa_family == AF_INET6) {
        if (saLen < sizeof(struct sockaddr_in6)) {
            errReason = "truncated IPv6 socket address is not permitted";
            return true;
        }
        const auto *sin6 = reinterpret_cast<const struct sockaddr_in6 *>(sa);
        const unsigned char *b = sin6->sin6_addr.s6_addr;

        bool first15Zero = true;
        for (int i = 0; i < 15; ++i) {
            if (b[i] != 0) {
                first15Zero = false;
                break;
            }
        }
        /* ::/128 unspecified */
        if (first15Zero && b[15] == 0) {
            errReason = "IPv6 unspecified address is not permitted";
            return true;
        }
        /* ::1/128 loopback */
        if (first15Zero && b[15] == 1) {
            if (!allowLoopbackForTest) {
                errReason = "IPv6 loopback address is not permitted";
                return true;
            }
            return false;
        }
        if (sin6->sin6_scope_id != 0) {
            errReason = "scoped IPv6 address is not permitted";
            return true;
        }

        /* ::ffff:0:0/96 IPv4-mapped IPv6 */
        bool first10Zero = true;
        for (int i = 0; i < 10; ++i) {
            if (b[i] != 0) {
                first10Zero = false;
                break;
            }
        }
        if (first10Zero && b[10] == 0xFF && b[11] == 0xFF) {
            errReason = "IPv4-mapped IPv6 address (::ffff:0:0/96) is not permitted";
            return true;
        }
        /* ::/96 deprecated IPv4-compatible IPv6 */
        if (first10Zero && b[10] == 0 && b[11] == 0) {
            errReason = "IPv4-compatible IPv6 address (::/96) is not permitted";
            return true;
        }
        /* ::ffff:0:0:0/96 SIIT IPv4-translated IPv6 */
        bool first8Zero = true;
        for (int i = 0; i < 8; ++i) {
            if (b[i] != 0) {
                first8Zero = false;
                break;
            }
        }
        if (first8Zero && b[8] == 0xFF && b[9] == 0xFF &&
            b[10] == 0x00 && b[11] == 0x00) {
            errReason = "IPv4-translated IPv6 address (::ffff:0:0:0/96) is not permitted";
            return true;
        }
        /* NAT64 well-known 64:ff9b::/96 & local-use 64:ff9b:1::/48 */
        if (b[0] == 0x00 && b[1] == 0x64 && b[2] == 0xFF && b[3] == 0x9B) {
            bool nat64WellKnown = true;
            for (int i = 4; i < 12; ++i) {
                if (b[i] != 0) {
                    nat64WellKnown = false;
                    break;
                }
            }
            if (nat64WellKnown || (b[4] == 0x00 && b[5] == 0x01)) {
                errReason = "NAT64 embedded-IPv4 address is not permitted";
                return true;
            }
        }
        /* Discard-only 100::/64 (RFC 6666) */
        if (b[0] == 0x01 && b[1] == 0x00 &&
            b[2] == 0x00 && b[3] == 0x00 && b[4] == 0x00 &&
            b[5] == 0x00 && b[6] == 0x00 && b[7] == 0x00) {
            errReason = "discard-only IPv6 prefix (100::/64) is not permitted";
            return true;
        }
        /* 2001:: special prefixes: Teredo 2001::/32, BMWG 2001:2::/48, ORCHID 2001:10::/28, Doc 2001:db8::/32 */
        if (b[0] == 0x20 && b[1] == 0x01) {
            if (b[2] == 0x00 && b[3] == 0x00) {
                errReason = "Teredo IPv6 address (2001::/32) is not permitted";
                return true;
            }
            if (b[2] == 0x00 && b[3] == 0x02 && b[4] == 0x00 && b[5] == 0x00) {
                errReason = "benchmarking IPv6 address (2001:2::/48) is not permitted";
                return true;
            }
            if (b[2] == 0x00 && (b[3] & 0xF0) == 0x10) {
                errReason = "ORCHID IPv6 address (2001:10::/28) is not permitted";
                return true;
            }
            if (b[2] == 0x0D && b[3] == 0xB8) {
                errReason = "documentation IPv6 address (2001:db8::/32) is not permitted";
                return true;
            }
        }
        /* 6to4 2002::/16 */
        if (b[0] == 0x20 && b[1] == 0x02) {
            errReason = "6to4 IPv6 address (2002::/16) is not permitted";
            return true;
        }
        /* Unique local fc00::/7 */
        if ((b[0] & 0xFE) == 0xFC) {
            errReason = "IPv6 unique local address (fc00::/7) is not permitted";
            return true;
        }
        /* Link-local fe80::/10 & deprecated site-local fec0::/10 */
        if (b[0] == 0xFE && ((b[1] & 0xC0) == 0x80 || (b[1] & 0xC0) == 0xC0)) {
            errReason = "IPv6 link-local or site-local address is not permitted";
            return true;
        }
        /* Multicast ff00::/8 */
        if (b[0] == 0xFF) {
            errReason = "IPv6 multicast address (ff00::/8) is not permitted";
            return true;
        }
        return false;
    }

    errReason = "unsupported address family is not permitted";
    return true;
}

bool OctiHttpServer::isDisallowedProbeAddress(const std::string &ipLiteral,
                                              std::string &errReason,
                                              bool allowLoopbackForTest)
{
    size_t start = 0;
    while (start < ipLiteral.size() &&
           std::isspace(static_cast<unsigned char>(ipLiteral[start]))) {
        ++start;
    }
    size_t end = ipLiteral.size();
    while (end > start &&
           std::isspace(static_cast<unsigned char>(ipLiteral[end - 1]))) {
        --end;
    }
    if (end <= start || (end - start) > 128) {
        errReason = "malformed IP literal is not permitted";
        return true;
    }
    std::string trimmed = ipLiteral.substr(start, end - start);
    if (trimmed.front() == '[' && trimmed.back() == ']' && trimmed.size() > 2) {
        trimmed = trimmed.substr(1, trimmed.size() - 2);
    }
    if (trimmed.find('%') != std::string::npos) {
        errReason = "scoped IP literal is not permitted";
        return true;
    }

    struct sockaddr_in sin;
    std::memset(&sin, 0, sizeof(sin));
    if (inet_pton(AF_INET, trimmed.c_str(), &sin.sin_addr) == 1) {
        sin.sin_family = AF_INET;
        return isDisallowedProbeAddress(
            reinterpret_cast<const struct sockaddr *>(&sin),
            sizeof(sin), errReason, allowLoopbackForTest);
    }

    struct sockaddr_in6 sin6;
    std::memset(&sin6, 0, sizeof(sin6));
    if (inet_pton(AF_INET6, trimmed.c_str(), &sin6.sin6_addr) == 1) {
        sin6.sin6_family = AF_INET6;
        return isDisallowedProbeAddress(
            reinterpret_cast<const struct sockaddr *>(&sin6),
            sizeof(sin6), errReason, allowLoopbackForTest);
    }

    errReason = "malformed IP literal is not permitted";
    return true;
}

namespace {

static constexpr size_t kMaxProbeHeaderBytes = 8192;

struct ResolvedProbeEndpoint {
    struct sockaddr_storage storage;
    socklen_t addrLen = 0;
    int family = AF_UNSPEC;
    int sockType = SOCK_STREAM;
    int protocol = 0;
};

bool parseProbeUrlParts(const std::string &url,
                        bool &isHttpsOut,
                        std::string &hostOut,
                        int &portOut,
                        std::string &pathOut,
                        std::string &errReason)
{
    std::string low = lower(url);
    bool isHttps = (low.rfind("https://", 0) == 0);
    bool isHttp = (low.rfind("http://", 0) == 0);
    if (!isHttp && !isHttps) {
        errReason = "scheme must be http:// or https://";
        return false;
    }
    size_t schemeEnd = isHttps ? 8 : 7;
    size_t pathStart = url.find('/', schemeEnd);
    std::string auth = (pathStart == std::string::npos)
        ? url.substr(schemeEnd)
        : url.substr(schemeEnd, pathStart - schemeEnd);
    std::string path = (pathStart == std::string::npos)
        ? std::string("/")
        : url.substr(pathStart);
    if (path.empty()) {
        path = "/";
    }
    if (auth.empty() || hasControlChars(auth) || hasControlChars(path)) {
        errReason = "malformed URL authority or path";
        return false;
    }

    std::string host;
    int port = isHttps ? 443 : 80;
    if (auth.front() == '[') {
        size_t closeBracket = auth.find(']');
        if (closeBracket == std::string::npos || closeBracket <= 1) {
            errReason = "malformed IPv6 host literal";
            return false;
        }
        host = auth.substr(1, closeBracket - 1);
        if (closeBracket + 1 < auth.size()) {
            if (auth[closeBracket + 1] != ':') {
                errReason = "malformed port after IPv6 literal";
                return false;
            }
            int64_t parsedPort = 0;
            if (!parseUnsignedStrict(auth.substr(closeBracket + 2), 65535, parsedPort) ||
                parsedPort <= 0) {
                errReason = "invalid URL port";
                return false;
            }
            port = static_cast<int>(parsedPort);
        }
    } else {
        size_t colon = auth.find(':');
        if (colon != std::string::npos &&
            auth.find(':', colon + 1) != std::string::npos) {
            errReason = "unbracketed IPv6 literal in URL";
            return false;
        }
        host = (colon == std::string::npos) ? auth : auth.substr(0, colon);
        if (colon != std::string::npos) {
            int64_t parsedPort = 0;
            if (!parseUnsignedStrict(auth.substr(colon + 1), 65535, parsedPort) ||
                parsedPort <= 0) {
                errReason = "invalid URL port";
                return false;
            }
            port = static_cast<int>(parsedPort);
        }
    }
    if (host.empty() || host.size() > 253) {
        errReason = "invalid URL hostname";
        return false;
    }

    isHttpsOut = isHttps;
    hostOut = std::move(host);
    portOut = port;
    pathOut = std::move(path);
    return true;
}

bool resolveAndValidateProbeEndpoints(const std::string &host,
                                      int port,
                                      bool allowLoopbackForTest,
                                      std::vector<ResolvedProbeEndpoint> &outEndpoints,
                                      bool &outDisallowedTarget,
                                      std::string &errReason)
{
    outEndpoints.clear();
    outDisallowedTarget = false;

    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *resList = nullptr;
    const std::string portStr = std::to_string(port);
    int gaErr = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &resList);
    if (gaErr != 0 || resList == nullptr) {
        errReason = "hostname could not be resolved: " + host;
        return false;
    }

    for (struct addrinfo *p = resList; p != nullptr; p = p->ai_next) {
        if (p->ai_addr == nullptr ||
            p->ai_addrlen == 0 ||
            static_cast<size_t>(p->ai_addrlen) > sizeof(struct sockaddr_storage)) {
            freeaddrinfo(resList);
            errReason = "invalid resolved socket address";
            return false;
        }
        if (OctiHttpServer::isDisallowedProbeAddress(
                p->ai_addr, static_cast<size_t>(p->ai_addrlen),
                errReason, allowLoopbackForTest)) {
            freeaddrinfo(resList);
            outDisallowedTarget = true;
            return false;
        }
        ResolvedProbeEndpoint ep;
        std::memset(&ep.storage, 0, sizeof(ep.storage));
        std::memcpy(&ep.storage, p->ai_addr, p->ai_addrlen);
        ep.addrLen = p->ai_addrlen;
        ep.family = p->ai_family;
        ep.sockType = p->ai_socktype;
        ep.protocol = p->ai_protocol;
        outEndpoints.push_back(ep);
    }
    freeaddrinfo(resList);

    if (outEndpoints.empty()) {
        errReason = "no usable network addresses resolved for " + host;
        return false;
    }
    return true;
}

bool probeResolvedHttpOrHttps(const std::vector<ResolvedProbeEndpoint> &endpoints,
                              bool isHttps,
                              const std::string &host,
                              int port,
                              const std::string &path,
                              const std::string &method,
                              int &statusCodeOut,
                              std::string &redirectLocationOut,
                              bool &fatalTlsFailureOut,
                              std::string &errReason)
{
    fatalTlsFailureOut = false;
    int sock = -1;
    for (size_t i = 0; i < endpoints.size(); ++i) {
        const ResolvedProbeEndpoint &ep = endpoints[i];
        sock = ::socket(ep.family, ep.sockType, ep.protocol);
        if (sock < 0) {
            continue;
        }
        int flags = ::fcntl(sock, F_GETFL, 0);
        if (flags >= 0) {
            (void)::fcntl(sock, F_SETFL, flags | O_NONBLOCK);
        }

        int rc = ::connect(
            sock, reinterpret_cast<const struct sockaddr *>(&ep.storage),
            ep.addrLen);
        if (rc < 0 && errno == EINPROGRESS) {
            struct pollfd pfd;
            pfd.fd = sock;
            pfd.events = POLLOUT;
            int prc = ::poll(&pfd, 1, 3000);
            if (prc > 0 && (pfd.revents & POLLOUT)) {
                int sockErr = 0;
                socklen_t errLen = sizeof(sockErr);
                if (::getsockopt(sock, SOL_SOCKET, SO_ERROR, &sockErr, &errLen) == 0 &&
                    sockErr == 0) {
                    break;
                }
            }
        } else if (rc == 0) {
            break;
        }
        ::close(sock);
        sock = -1;
    }

    if (sock < 0) {
        errReason = "could not connect to target within 3 seconds: " + host;
        return false;
    }

    int flags = ::fcntl(sock, F_GETFL, 0);
    if (flags >= 0) {
        (void)::fcntl(sock, F_SETFL, flags & ~O_NONBLOCK);
    }
    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    (void)::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    (void)::setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    std::string hostHeader = host;
    if (host.find(':') != std::string::npos) {
        hostHeader = "[" + host + "]";
    }
    if ((isHttps && port != 443) || (!isHttps && port != 80)) {
        hostHeader += ":" + std::to_string(port);
    }
    std::string req = method + " " + path + " HTTP/1.1\r\n" +
                      "Host: " + hostHeader + "\r\n" +
                      "User-Agent: OctiBot-UrlVerifier/1.0\r\n" +
                      "Connection: close\r\n" +
                      "Accept: */*\r\n\r\n";

    std::string respStr;
    respStr.reserve(1024);
    const int64_t readDeadlineMs = steadyMs() + 3000;

    if (isHttps) {
        SSL_CTX *sslCtx = SSL_CTX_new(TLS_client_method());
        if (!sslCtx) {
            ::close(sock);
            fatalTlsFailureOut = true;
            errReason = "failed to initialize TLS context";
            return false;
        }
        SSL_CTX_set_min_proto_version(sslCtx, TLS1_2_VERSION);
        if (SSL_CTX_set_default_verify_paths(sslCtx) != 1) {
            SSL_CTX_free(sslCtx);
            ::close(sock);
            fatalTlsFailureOut = true;
            errReason = "failed to load system CA bundle for TLS verification";
            return false;
        }
        SSL_CTX_set_verify(sslCtx, SSL_VERIFY_PEER, nullptr);

        SSL *ssl = SSL_new(sslCtx);
        if (!ssl) {
            SSL_CTX_free(sslCtx);
            ::close(sock);
            fatalTlsFailureOut = true;
            errReason = "failed to allocate TLS session";
            return false;
        }

        struct in_addr v4Dummy;
        struct in6_addr v6Dummy;
        const bool hostIsIp =
            (inet_pton(AF_INET, host.c_str(), &v4Dummy) == 1) ||
            (inet_pton(AF_INET6, host.c_str(), &v6Dummy) == 1);
        bool hostBindOk = false;
        if (hostIsIp) {
            X509_VERIFY_PARAM *vpm = SSL_get0_param(ssl);
            hostBindOk = (vpm != nullptr &&
                          X509_VERIFY_PARAM_set1_ip_asc(vpm, host.c_str()) == 1);
        } else {
            SSL_set_hostflags(ssl, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
            hostBindOk = (SSL_set_tlsext_host_name(ssl, host.c_str()) == 1 &&
                          SSL_set1_host(ssl, host.c_str()) == 1);
        }
        if (!hostBindOk) {
            SSL_free(ssl);
            SSL_CTX_free(sslCtx);
            ::close(sock);
            fatalTlsFailureOut = true;
            errReason = "failed to configure TLS hostname verification for " + host;
            return false;
        }

        SSL_set_fd(ssl, sock);
        if (SSL_connect(ssl) != 1 || SSL_get_verify_result(ssl) != X509_V_OK) {
            SSL_free(ssl);
            SSL_CTX_free(sslCtx);
            ::close(sock);
            fatalTlsFailureOut = true;
            errReason = "TLS certificate or hostname verification failed for " + host;
            return false;
        }

        size_t written = 0;
        while (written < req.size()) {
            int chunk = static_cast<int>(
                std::min<size_t>(req.size() - written, 16384u));
            int nw = SSL_write(ssl, req.data() + written, chunk);
            if (nw <= 0) {
                SSL_free(ssl);
                SSL_CTX_free(sslCtx);
                ::close(sock);
                errReason = "TLS write failed while probing " + host;
                return false;
            }
            written += static_cast<size_t>(nw);
        }

        char buf[2048];
        while (respStr.find("\r\n\r\n") == std::string::npos) {
            if (respStr.size() >= kMaxProbeHeaderBytes) {
                SSL_shutdown(ssl);
                SSL_free(ssl);
                SSL_CTX_free(sslCtx);
                ::close(sock);
                errReason = "HTTP response headers exceeded 8 KiB limit";
                return false;
            }
            if (steadyMs() > readDeadlineMs) {
                SSL_free(ssl);
                SSL_CTX_free(sslCtx);
                ::close(sock);
                errReason = "timed out reading HTTP response headers from " + host;
                return false;
            }
            size_t maxChunk = std::min<size_t>(
                sizeof(buf), kMaxProbeHeaderBytes + 1 - respStr.size());
            int nr = SSL_read(ssl, buf, static_cast<int>(maxChunk));
            if (nr == 0) {
                break;
            }
            if (nr < 0) {
                SSL_free(ssl);
                SSL_CTX_free(sslCtx);
                ::close(sock);
                errReason = "error reading TLS response from " + host;
                return false;
            }
            respStr.append(buf, static_cast<size_t>(nr));
        }

        SSL_shutdown(ssl);
        SSL_free(ssl);
        SSL_CTX_free(sslCtx);
    } else {
        size_t written = 0;
        while (written < req.size()) {
            ssize_t nw = ::send(sock, req.data() + written,
                                req.size() - written, MSG_NOSIGNAL);
            if (nw < 0) {
                if (errno == EINTR) {
                    continue;
                }
                ::close(sock);
                errReason = "socket write failed while probing " + host;
                return false;
            }
            if (nw == 0) {
                ::close(sock);
                errReason = "socket closed during write to " + host;
                return false;
            }
            written += static_cast<size_t>(nw);
        }

        char buf[2048];
        while (respStr.find("\r\n\r\n") == std::string::npos) {
            if (respStr.size() >= kMaxProbeHeaderBytes) {
                ::close(sock);
                errReason = "HTTP response headers exceeded 8 KiB limit";
                return false;
            }
            if (steadyMs() > readDeadlineMs) {
                ::close(sock);
                errReason = "timed out reading HTTP response headers from " + host;
                return false;
            }
            size_t maxChunk = std::min<size_t>(
                sizeof(buf), kMaxProbeHeaderBytes + 1 - respStr.size());
            ssize_t nr = ::recv(sock, buf, maxChunk, 0);
            if (nr == 0) {
                break;
            }
            if (nr < 0) {
                if (errno == EINTR) {
                    continue;
                }
                ::close(sock);
                errReason = "error reading HTTP response from " + host;
                return false;
            }
            respStr.append(buf, static_cast<size_t>(nr));
        }
    }
    ::close(sock);

    if (respStr.empty()) {
        errReason = "no HTTP response received from " + host;
        return false;
    }

    size_t hdrEnd = respStr.find("\r\n\r\n");
    if (hdrEnd == std::string::npos) {
        if (respStr.size() > kMaxProbeHeaderBytes) {
            errReason = "HTTP response headers exceeded 8 KiB limit";
            return false;
        }
    } else {
        respStr.resize(hdrEnd + 2);
    }

    if (respStr.rfind("HTTP/", 0) != 0) {
        errReason = "malformed HTTP status line";
        return false;
    }
    size_t sp1 = respStr.find(' ');
    size_t crlf = respStr.find("\r\n");
    if (sp1 == std::string::npos || crlf == std::string::npos || sp1 >= crlf) {
        errReason = "malformed HTTP status line";
        return false;
    }
    size_t sp2 = respStr.find(' ', sp1 + 1);
    size_t endCode = (sp2 != std::string::npos && sp2 < crlf) ? sp2 : crlf;
    int64_t code64 = 0;
    if (!parseUnsignedStrict(respStr.substr(sp1 + 1, endCode - (sp1 + 1)),
                             599, code64) ||
        code64 < 100) {
        errReason = "invalid HTTP status code";
        return false;
    }
    statusCodeOut = static_cast<int>(code64);

    redirectLocationOut.clear();
    if (statusCodeOut >= 300 && statusCodeOut < 400) {
        size_t lineStart = crlf + 2;
        while (lineStart < respStr.size()) {
            size_t lineEnd = respStr.find("\r\n", lineStart);
            if (lineEnd == std::string::npos) {
                lineEnd = respStr.size();
            }
            if (lineEnd - lineStart >= 9) {
                std::string prefixLower = lower(respStr.substr(lineStart, 9));
                if (prefixLower == "location:") {
                    size_t valStart = lineStart + 9;
                    while (valStart < lineEnd &&
                           (respStr[valStart] == ' ' || respStr[valStart] == '\t')) {
                        ++valStart;
                    }
                    size_t valEnd = lineEnd;
                    while (valEnd > valStart &&
                           (respStr[valEnd - 1] == ' ' || respStr[valEnd - 1] == '\t')) {
                        --valEnd;
                    }
                    redirectLocationOut = respStr.substr(valStart, valEnd - valStart);
                    break;
                }
            }
            if (lineEnd >= respStr.size()) {
                break;
            }
            lineStart = lineEnd + 2;
        }
    }
    return true;
}

bool verifyRobotUrlReachable(const std::string &initialUrl,
                             std::string &errReason,
                             bool *outDisallowedTarget = nullptr)
{
    if (outDisallowedTarget != nullptr) {
        *outDisallowedTarget = false;
    }
    const char *allowLoopbackEnv = std::getenv("OCTI_ALLOW_LOOPBACK_ROBOT_URL_FOR_TEST");
    const bool loopbackAllowed =
        (allowLoopbackEnv != nullptr && std::string(allowLoopbackEnv) == "1");

    std::string currentUrl = initialUrl;
    for (int hop = 0; hop < 4; ++hop) {
        if (!AccountStore::validRobotUrlSyntax(currentUrl)) {
            errReason = "invalid URL syntax or rejected tracking query parameter: " + currentUrl;
            return false;
        }

        bool isHttps = false;
        std::string host;
        int port = 0;
        std::string path;
        if (!parseProbeUrlParts(currentUrl, isHttps, host, port, path, errReason)) {
            return false;
        }

        std::vector<ResolvedProbeEndpoint> endpoints;
        bool disallowedTarget = false;
        if (!resolveAndValidateProbeEndpoints(
                host, port, loopbackAllowed, endpoints, disallowedTarget, errReason)) {
            if (disallowedTarget && outDisallowedTarget != nullptr) {
                *outDisallowedTarget = true;
            }
            return false;
        }

        int statusCode = 0;
        std::string redirectLoc;
        bool fatalTls = false;
        bool ok = probeResolvedHttpOrHttps(
            endpoints, isHttps, host, port, path, "HEAD",
            statusCode, redirectLoc, fatalTls, errReason);
        if (!fatalTls &&
            (!ok || statusCode == 405 || statusCode == 403 || statusCode == 501)) {
            ok = probeResolvedHttpOrHttps(
                endpoints, isHttps, host, port, path, "GET",
                statusCode, redirectLoc, fatalTls, errReason);
        }
        if (!ok) {
            return false;
        }

        if (statusCode >= 200 && statusCode <= 299) {
            return true;
        }

        if (statusCode >= 300 && statusCode < 400 && !redirectLoc.empty()) {
            std::string redirLow = lower(redirectLoc);
            if (redirLow.rfind("http://", 0) != 0 &&
                redirLow.rfind("https://", 0) != 0) {
                size_t schemeEnd = currentUrl.find("://");
                if (schemeEnd != std::string::npos) {
                    schemeEnd += 3;
                    size_t slash = currentUrl.find('/', schemeEnd);
                    std::string origin = currentUrl.substr(0, slash);
                    if (redirectLoc.front() != '/') {
                        redirectLoc = "/" + redirectLoc;
                    }
                    currentUrl = origin + redirectLoc;
                } else {
                    errReason = "invalid redirect location";
                    return false;
                }
            } else {
                currentUrl = redirectLoc;
            }
            continue;
        }

        errReason = "target returned HTTP " + std::to_string(statusCode);
        return false;
    }
    errReason = "too many redirects";
    return false;
}

std::string hmacSha256(const std::string &data, const uint8_t *key,
                       size_t keyLen)
{
    unsigned char md[EVP_MAX_MD_SIZE];
    unsigned int mdLen = 0;
    if (HMAC(EVP_sha256(), key, static_cast<int>(keyLen),
             reinterpret_cast<const unsigned char *>(data.data()), data.size(),
             md, &mdLen) == nullptr) {
        return std::string();
    }
    return std::string(reinterpret_cast<const char *>(md), mdLen);
}

bool randomTokenB64Url(size_t numBytes, std::string &out)
{
    std::vector<unsigned char> buf(numBytes);
    if (numBytes == 0 ||
        RAND_bytes(buf.data(), static_cast<int>(buf.size())) != 1) {
        return false;
    }
    out = base64UrlEncode(buf.data(), buf.size());
    return !out.empty();
}

bool isValidOauthRedirectUri(const std::string &redirectUri,
                             bool &isNativeOut)
{
    isNativeOut = false;
    if (redirectUri.empty() || redirectUri.size() > 512) {
        return false;
    }
    for (size_t i = 0; i < redirectUri.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(redirectUri[i]);
        if (c <= 0x20 || c >= 0x7F) {
            return false;
        }
    }
    if (redirectUri[0] == '/' &&
        (redirectUri.size() == 1 ||
         (redirectUri[1] != '/' && redirectUri[1] != '\\'))) {
        return true;
    }
    if (redirectUri == "octi://auth/callback" ||
        redirectUri == "octi://callback") {
        isNativeOut = true;
        return true;
    }
    if (redirectUri.rfind("http://127.0.0.1:", 0) == 0 ||
        redirectUri.rfind("http://localhost:", 0) == 0) {
        size_t colon = redirectUri.find(':', 7);
        if (colon != std::string::npos) {
            size_t slash = redirectUri.find('/', colon);
            if (slash != std::string::npos) {
                std::string portStr =
                    redirectUri.substr(colon + 1, slash - (colon + 1));
                int64_t portNum = 0;
                if (portStr.size() >= 1 && portStr.size() <= 5) {
                    bool allDigits = true;
                    for (size_t i = 0; i < portStr.size(); ++i) {
                        if (portStr[i] < '0' || portStr[i] > '9') {
                            allDigits = false;
                            break;
                        }
                    }
                    if (allDigits) {
                        portNum = std::strtoll(portStr.c_str(), nullptr, 10);
                    }
                }
                if (portNum >= 1 && portNum <= 65535) {
                    std::string pathPart = redirectUri.substr(slash);
                    if (pathPart == "/callback" ||
                        pathPart == "/oauth/callback") {
                        isNativeOut = true;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool splitAuthorityHostOnly(const std::string &rawAuth, std::string &hostOut)
{
    hostOut.clear();
    std::string s = rawAuth;
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
        s.erase(s.begin());
    }
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
        s.pop_back();
    }
    if (s.empty() || s.size() > 260) {
        return false;
    }
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 0x20 || c >= 0x7F || c == '/' || c == '\\' || c == '@' ||
            c == '?' || c == '#') {
            return false;
        }
    }
    if (s[0] == '[') {
        size_t closeBracket = s.find(']');
        if (closeBracket == std::string::npos || closeBracket <= 1) {
            return false;
        }
        hostOut = lower(s.substr(0, closeBracket + 1));
        if (closeBracket + 1 < s.size()) {
            if (s[closeBracket + 1] != ':') {
                return false;
            }
            std::string portStr = s.substr(closeBracket + 2);
            if (portStr.empty() || portStr.size() > 5) {
                return false;
            }
            for (size_t i = 0; i < portStr.size(); ++i) {
                if (portStr[i] < '0' || portStr[i] > '9') {
                    return false;
                }
            }
            long p = std::strtol(portStr.c_str(), nullptr, 10);
            if (p < 1 || p > 65535) {
                return false;
            }
        }
        return true;
    }
    size_t colon = s.find(':');
    if (colon == std::string::npos) {
        hostOut = lower(s);
        return !hostOut.empty();
    }
    if (s.find(':', colon + 1) != std::string::npos) {
        return false;
    }
    hostOut = lower(s.substr(0, colon));
    std::string portStr = s.substr(colon + 1);
    if (hostOut.empty() || portStr.empty() || portStr.size() > 5) {
        return false;
    }
    for (size_t i = 0; i < portStr.size(); ++i) {
        if (portStr[i] < '0' || portStr[i] > '9') {
            return false;
        }
    }
    long p = std::strtol(portStr.c_str(), nullptr, 10);
    return p >= 1 && p <= 65535;
}

std::string urlEncode(const std::string &value)
{
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;
    for (size_t i = 0; i < value.size(); ++i) {
        char c = value[i];
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' ||
            c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::uppercase << std::setw(2)
                    << int(static_cast<unsigned char>(c));
        }
    }
    return escaped.str();
}

struct InitOidcRegistry {
    InitOidcRegistry()
    {
        (void)OidcVerifier::providerRegistry();
    }
} g_initOidcRegistry;

std::string cleanAgent(const std::string &in)
{
    std::string out;
    for (size_t i = 0; i < in.size() && out.size() < kMaxClientAgentBytes;
         ++i) {
        unsigned char c = static_cast<unsigned char>(in[i]);
        if (c >= 0x20 && c < 0x7F) {
            out += static_cast<char>(c);
        }
    }
    return out;
}

std::string detectClientType(const Request &req)
{
    if (req.has_header("X-Octi-Client-Type")) {
        std::string h = lower(req.get_header_value("X-Octi-Client-Type"));
        size_t first = h.find_first_not_of(" \t\r\n");
        if (first != std::string::npos) {
            size_t last = h.find_last_not_of(" \t\r\n");
            h = h.substr(first, (last - first + 1));
        } else {
            h.clear();
        }
        if (h == "web" || h == "android" || h == "desktop") {
            return h;
        }
    }
    if (req.has_param("client_type")) {
        std::string p = lower(req.get_param_value("client_type"));
        size_t first = p.find_first_not_of(" \t\r\n");
        if (first != std::string::npos) {
            size_t last = p.find_last_not_of(" \t\r\n");
            p = p.substr(first, (last - first + 1));
        } else {
            p.clear();
        }
        if (p == "web" || p == "android" || p == "desktop") {
            return p;
        }
    }
    if (req.has_header("User-Agent")) {
        std::string ua = lower(req.get_header_value("User-Agent"));
        if (ua.find("octi-android") != std::string::npos ||
            ua.find("okhttp") != std::string::npos) {
            return "android";
        }
        if (ua.find("octi-desktop") != std::string::npos) {
            return "desktop";
        }
        if (ua.find("mozilla") != std::string::npos ||
            ua.find("chrome") != std::string::npos ||
            ua.find("safari") != std::string::npos ||
            ua.find("firefox") != std::string::npos) {
            return "web";
        }
    }
    return "";
}

bool isDisallowedCrawlerUserAgent(const std::string &ua)
{
    if (ua.empty()) {
        return false;
    }
    std::string l = lower(ua);
    static const char *const kTokens[] = {
        "gptbot", "claudebot", "ccbot", "bytespider", "petalbot",
        "ahrefsbot", "semrushbot", "mj12bot", "dotbot", "scrapy"
    };
    for (size_t i = 0; i < sizeof(kTokens) / sizeof(kTokens[0]); ++i) {
        if (l.find(kTokens[i]) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool isLoopbackIp(const std::string &ip)
{
    return ip.empty() ||
           ip == "127.0.0.1" ||
           ip == "::1" ||
           ip == "::ffff:127.0.0.1" ||
           ip == "localhost" ||
           ip == "unknown";
}

/* ------------------------------------------------------------------ */
/* Bounded Rate Limiters (Token Bucket & Fixed Window)                 */
/* ------------------------------------------------------------------ */

class TokenBucketLimiter {
public:
    static const size_t kMaxKeys = 16384;

    struct BucketEntry {
        double tokens = 0.0;
        int64_t lastRefillSteadyMs = 0;
    };

    bool tryAcquire(const std::string &key, double refillPerSec, double burst,
                    int64_t &retryAfterSec)
    {
        if (burst <= 0.0 || refillPerSec <= 0.0 || key.empty()) {
            return true;
        }
        const int64_t now = steadyMs();
        std::lock_guard<std::mutex> lock(_mutex);
        BucketEntry &b = getOrCreateLocked(key, refillPerSec, burst, now);
        if (b.tokens >= 1.0) {
            b.tokens -= 1.0;
            return true;
        }
        double deficit = std::max(0.0, 1.0 - b.tokens);
        retryAfterSec = std::max<int64_t>(
            1, static_cast<int64_t>(std::ceil(deficit / refillPerSec)));
        return false;
    }

    bool tryAcquireDual(const std::string &actorKey, double actorRefill,
                        double actorBurst, const std::string &ipKey,
                        double ipRefill, double ipBurst,
                        int64_t &retryAfterSec)
    {
        const bool checkActor =
            !actorKey.empty() && actorBurst > 0.0 && actorRefill > 0.0;
        const bool checkIp =
            !ipKey.empty() && ipBurst > 0.0 && ipRefill > 0.0;
        if (!checkActor && !checkIp) {
            return true;
        }
        const int64_t now = steadyMs();
        std::lock_guard<std::mutex> lock(_mutex);
        BucketEntry *actorEntry = checkActor
            ? &getOrCreateLocked(actorKey, actorRefill, actorBurst, now)
            : nullptr;
        BucketEntry *ipEntry = checkIp
            ? &getOrCreateLocked(ipKey, ipRefill, ipBurst, now)
            : nullptr;

        int64_t waitSec = 0;
        if (actorEntry != nullptr && actorEntry->tokens < 1.0) {
            double deficit = std::max(0.0, 1.0 - actorEntry->tokens);
            waitSec = std::max<int64_t>(
                waitSec,
                std::max<int64_t>(1, static_cast<int64_t>(
                                         std::ceil(deficit / actorRefill))));
        }
        if (ipEntry != nullptr && ipEntry->tokens < 1.0) {
            double deficit = std::max(0.0, 1.0 - ipEntry->tokens);
            waitSec = std::max<int64_t>(
                waitSec,
                std::max<int64_t>(1, static_cast<int64_t>(
                                         std::ceil(deficit / ipRefill))));
        }
        if (waitSec > 0) {
            retryAfterSec = waitSec;
            return false;
        }
        if (actorEntry != nullptr) {
            actorEntry->tokens -= 1.0;
        }
        if (ipEntry != nullptr) {
            ipEntry->tokens -= 1.0;
        }
        return true;
    }

    size_t size() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _entries.size();
    }

private:
    BucketEntry &getOrCreateLocked(const std::string &key, double refillPerSec,
                                   double burst, int64_t now)
    {
        auto it = _entries.find(key);
        if (it == _entries.end()) {
            if (_entries.size() >= kMaxKeys) {
                auto oldest = _entries.begin();
                for (auto scan = _entries.begin(); scan != _entries.end();
                     ++scan) {
                    if (scan->second.lastRefillSteadyMs <
                        oldest->second.lastRefillSteadyMs) {
                        oldest = scan;
                    }
                }
                _entries.erase(oldest);
            }
            BucketEntry fresh;
            fresh.tokens = burst;
            fresh.lastRefillSteadyMs = now;
            it = _entries.emplace(key, fresh).first;
            return it->second;
        }
        int64_t elapsedMs = std::max<int64_t>(0, now - it->second.lastRefillSteadyMs);
        double refilled = it->second.tokens +
                          (static_cast<double>(elapsedMs) / 1000.0) * refillPerSec;
        it->second.tokens = std::min(burst, refilled);
        it->second.lastRefillSteadyMs = now;
        return it->second;
    }

    mutable std::mutex _mutex;
    std::unordered_map<std::string, BucketEntry> _entries;
};

class IpRateLimiter {
public:
    static const size_t kMaxKeys = 16384;

    struct WindowEntry {
        int64_t startMs = 0;
        int count = 0;
    };

    bool tryAcquire(const std::string &key, int maxCount, int64_t windowMs,
                    int64_t &retryAfterSec)
    {
        if (maxCount <= 0 || windowMs <= 0 || key.empty()) {
            return true;
        }
        const int64_t now = steadyMs();
        std::lock_guard<std::mutex> lock(_mutex);
        WindowEntry &w = getOrCreateLocked(key, windowMs, now);
        if (w.count < maxCount) {
            ++w.count;
            return true;
        }
        int64_t remMs = windowMs - (now - w.startMs);
        retryAfterSec = std::max<int64_t>(1, (remMs + 999) / 1000);
        return false;
    }

    bool tryAcquireDual(const std::string &k1, int max1,
                        const std::string &k2, int max2,
                        int64_t windowMs, int64_t &retryAfterSec)
    {
        if (windowMs <= 0) {
            return true;
        }
        const bool c1 = !k1.empty() && max1 > 0;
        const bool c2 = !k2.empty() && max2 > 0 && k2 != k1;
        if (!c1 && !c2) {
            return true;
        }
        const int64_t now = steadyMs();
        std::lock_guard<std::mutex> lock(_mutex);
        WindowEntry *w1 = c1 ? &getOrCreateLocked(k1, windowMs, now) : nullptr;
        WindowEntry *w2 = c2 ? &getOrCreateLocked(k2, windowMs, now) : nullptr;
        int64_t waitSec = 0;
        if (w1 != nullptr && w1->count >= max1) {
            int64_t remMs = windowMs - (now - w1->startMs);
            waitSec = std::max<int64_t>(waitSec,
                                        std::max<int64_t>(1, (remMs + 999) / 1000));
        }
        if (w2 != nullptr && w2->count >= max2) {
            int64_t remMs = windowMs - (now - w2->startMs);
            waitSec = std::max<int64_t>(waitSec,
                                        std::max<int64_t>(1, (remMs + 999) / 1000));
        }
        if (waitSec > 0) {
            retryAfterSec = waitSec;
            return false;
        }
        if (w1 != nullptr) {
            ++w1->count;
        }
        if (w2 != nullptr) {
            ++w2->count;
        }
        return true;
    }

    bool isBlocked(const std::string &key, int maxCount, int64_t windowMs,
                   int64_t &retryAfterSec) const
    {
        if (maxCount <= 0 || windowMs <= 0 || key.empty()) {
            return false;
        }
        const int64_t now = steadyMs();
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _entries.find(key);
        if (it == _entries.end()) {
            return false;
        }
        if (it->second.count < maxCount ||
            now - it->second.startMs >= windowMs) {
            return false;
        }
        int64_t remMs = windowMs - (now - it->second.startMs);
        retryAfterSec = std::max<int64_t>(1, (remMs + 999) / 1000);
        return true;
    }

private:
    WindowEntry &getOrCreateLocked(const std::string &key, int64_t windowMs,
                                   int64_t now)
    {
        auto it = _entries.find(key);
        if (it == _entries.end()) {
            if (_entries.size() >= kMaxKeys) {
                for (auto scan = _entries.begin(); scan != _entries.end();) {
                    if (now - scan->second.startMs >= windowMs) {
                        scan = _entries.erase(scan);
                    } else {
                        ++scan;
                    }
                }
                if (_entries.size() >= kMaxKeys) {
                    auto oldest = _entries.begin();
                    for (auto scan = _entries.begin(); scan != _entries.end();
                         ++scan) {
                        if (scan->second.startMs < oldest->second.startMs) {
                            oldest = scan;
                        }
                    }
                    _entries.erase(oldest);
                }
            }
            WindowEntry fresh;
            fresh.startMs = now;
            fresh.count = 0;
            it = _entries.emplace(key, fresh).first;
            return it->second;
        }
        if (it->second.count == 0 || now - it->second.startMs >= windowMs) {
            it->second.startMs = now;
            it->second.count = 0;
        }
        return it->second;
    }

    mutable std::mutex _mutex;
    std::unordered_map<std::string, WindowEntry> _entries;
};

const char *modeName(OctiGameMode m)
{
    switch (m) {
    case GAME_MODE_4: return "4P";
    case GAME_MODE_OFK: return "OFK";
    default: break;
    }
    return "2P";
}

bool parseModeName(const std::string &s, OctiGameMode &out)
{
    std::string u = upper(s);
    if (u == "2P") {
        out = GAME_MODE_2;
        return true;
    }
    if (u == "4P") {
        out = GAME_MODE_4;
        return true;
    }
    if (u == "OFK") {
        out = GAME_MODE_OFK;
        return true;
    }
    return false;
}

/* ------------------------------------------------------------------ */
/* JSON field access with type checking                                */
/* ------------------------------------------------------------------ */

bool fieldString(const Json &b, const char *key, bool required,
                 std::string &out, std::string &err)
{
    Json::const_iterator it = b.find(key);
    if (it == b.end() || it->is_null()) {
        if (required) {
            err = std::string("missing field '") + key + "'";
            return false;
        }
        return true;
    }
    if (!it->is_string()) {
        err = std::string("field '") + key + "' must be a string";
        return false;
    }
    out = it->get<std::string>();
    return true;
}

bool fieldBool(const Json &b, const char *key, bool required, bool &out,
               std::string &err)
{
    Json::const_iterator it = b.find(key);
    if (it == b.end() || it->is_null()) {
        if (required) {
            err = std::string("missing field '") + key + "'";
            return false;
        }
        return true;
    }
    if (!it->is_boolean()) {
        err = std::string("field '") + key + "' must be a boolean";
        return false;
    }
    out = it->get<bool>();
    return true;
}

bool jsonInt(const Json &v, int64_t &out)
{
    if (v.is_number_unsigned()) {
        uint64_t u = v.get<uint64_t>();
        if (u > static_cast<uint64_t>(INT64_MAX)) {
            return false;
        }
        out = static_cast<int64_t>(u);
        return true;
    }
    if (v.is_number_integer()) {
        out = v.get<int64_t>();
        return true;
    }
    return false;
}

bool fieldInt(const Json &b, const char *key, bool required, int64_t &out,
              std::string &err)
{
    Json::const_iterator it = b.find(key);
    if (it == b.end() || it->is_null()) {
        if (required) {
            err = std::string("missing field '") + key + "'";
            return false;
        }
        return true;
    }
    if (!jsonInt(*it, out)) {
        err = std::string("field '") + key + "' must be an integer";
        return false;
    }
    return true;
}

bool fieldPresent(const Json &b, const char *key)
{
    Json::const_iterator it = b.find(key);
    return it != b.end() && !it->is_null();
}

/* ------------------------------------------------------------------ */
/* Compression, ETag & Snapshot Cache Helpers                          */
/* ------------------------------------------------------------------ */

static constexpr size_t kMinDynamicGzipBytes = 1024;

std::string trimHttpToken(const std::string &s)
{
    size_t b = 0;
    size_t e = s.size();
    while (b < e && (s[b] == ' ' || s[b] == '\t' || s[b] == '\r' || s[b] == '\n')) {
        ++b;
    }
    while (e > b &&
           (s[e - 1] == ' ' || s[e - 1] == '\t' ||
            s[e - 1] == '\r' || s[e - 1] == '\n')) {
        --e;
    }
    return s.substr(b, e - b);
}

bool compressGzipWithLevel(const char *data, size_t len, int level,
                           std::string &out)
{
    out.clear();
    z_stream zs;
    std::memset(&zs, 0, sizeof(zs));
    if (deflateInit2(&zs, level, Z_DEFLATED, 15 + 16, 8,
                     Z_DEFAULT_STRATEGY) != Z_OK) {
        return false;
    }
    struct DeflateGuard {
        z_stream *s;
        ~DeflateGuard() { deflateEnd(s); }
    } guard{&zs};

    zs.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(data));
    zs.avail_in = static_cast<uInt>(len);

    char buf[16384];
    int ret = Z_OK;
    while (ret == Z_OK) {
        zs.next_out = reinterpret_cast<Bytef *>(buf);
        zs.avail_out = sizeof(buf);
        ret = deflate(&zs, Z_FINISH);
        if (ret != Z_OK && ret != Z_STREAM_END) {
            out.clear();
            return false;
        }
        size_t have = sizeof(buf) - zs.avail_out;
        out.append(buf, have);
    }
    return ret == Z_STREAM_END;
}

std::string compressGzipFast(const std::string &raw)
{
    if (raw.size() < kMinDynamicGzipBytes) {
        return std::string();
    }
    std::string gz;
    if (!compressGzipWithLevel(raw.data(), raw.size(), Z_BEST_SPEED, gz)) {
        return std::string();
    }
    return gz;
}

bool clientAcceptsGzip(const Request &req)
{
    if (!req.has_header("Accept-Encoding")) {
        return false;
    }
    const std::string hdr = req.get_header_value("Accept-Encoding");
    if (hdr.empty() || hdr.size() > 1024) {
        return false;
    }
    size_t pos = 0;
    while (pos <= hdr.size()) {
        size_t comma = hdr.find(',', pos);
        std::string item = (comma == std::string::npos)
                               ? hdr.substr(pos)
                               : hdr.substr(pos, comma - pos);
        size_t semi = item.find(';');
        std::string coding = lower(trimHttpToken(
            semi == std::string::npos ? item : item.substr(0, semi)));
        if (coding == "gzip") {
            if (semi == std::string::npos) {
                return true;
            }
            std::string params = lower(trimHttpToken(item.substr(semi + 1)));
            size_t qpos = params.find("q=");
            if (qpos == std::string::npos) {
                return true;
            }
            std::string qval = trimHttpToken(params.substr(qpos + 2));
            size_t qsemi = qval.find(';');
            if (qsemi != std::string::npos) {
                qval = trimHttpToken(qval.substr(0, qsemi));
            }
            if (qval == "0" || qval == "0.0" || qval == "0.00" ||
                qval == "0.000") {
                return false;
            }
            return true;
        }
        if (comma == std::string::npos) {
            break;
        }
        pos = comma + 1;
    }
    return false;
}

bool ifNoneMatchMatches(const Request &req, const std::string &etag)
{
    if (etag.empty() || !req.has_header("If-None-Match")) {
        return false;
    }
    const std::string hdr = req.get_header_value("If-None-Match");
    if (hdr.empty() || hdr.size() > 2048 || hasControlChars(hdr)) {
        return false;
    }
    auto stripWeak = [](const std::string &t) -> std::string {
        if (t.size() >= 2 && (t[0] == 'W' || t[0] == 'w') && t[1] == '/') {
            return trimHttpToken(t.substr(2));
        }
        return t;
    };
    const std::string normTarget = stripWeak(trimHttpToken(etag));
    if (normTarget.size() < 2 || normTarget.front() != '"' ||
        normTarget.back() != '"') {
        return false;
    }
    size_t pos = 0;
    while (pos <= hdr.size()) {
        size_t comma = hdr.find(',', pos);
        std::string cand = trimHttpToken(
            (comma == std::string::npos) ? hdr.substr(pos)
                                         : hdr.substr(pos, comma - pos));
        if (cand == "*") {
            return true;
        }
        std::string normCand = stripWeak(cand);
        if (normCand.size() >= 2 && normCand.front() == '"' &&
            normCand.back() == '"' && normCand == normTarget) {
            return true;
        }
        if (comma == std::string::npos) {
            break;
        }
        pos = comma + 1;
    }
    return false;
}

std::string makePayloadEtag(const char *prefix, const std::string &payload)
{
    return std::string("\"") + prefix +
           AccountStore::sha256Hex(payload).substr(0, 24) + "\"";
}

/*
 * Fast O(N) zero-parse single-line guard for any pre-existing legacy DB row
 * that was stored with pretty-printed newlines before Envelope 4.
 */
void appendSingleLineNdjsonRow(const std::string &transcript, std::string &out)
{
    if (transcript.find_first_of("\r\n") == std::string::npos) {
        out.append(transcript);
        out.push_back('\n');
        return;
    }
    bool inStr = false;
    bool escaped = false;
    bool skipIndent = false;
    for (size_t i = 0; i < transcript.size(); ++i) {
        char c = transcript[i];
        if (inStr) {
            if (escaped) {
                out.push_back(c);
                escaped = false;
            } else if (c == '\\') {
                out.push_back(c);
                escaped = true;
            } else if (c == '"') {
                out.push_back(c);
                inStr = false;
            } else if (c == '\r') {
                out += "\\r";
            } else if (c == '\n') {
                out += "\\n";
            } else {
                out.push_back(c);
            }
        } else {
            if (c == '\r' || c == '\n') {
                skipIndent = true;
                continue;
            }
            if (skipIndent && (c == ' ' || c == '\t')) {
                continue;
            }
            skipIndent = false;
            if (c == '"') {
                inStr = true;
            }
            out.push_back(c);
        }
    }
    out.push_back('\n');
}

struct CachedPayload {
    std::string jsonBytes;
    std::string gzipBytes;
    std::string etag;
    uint64_t rev = 0;
    int64_t expiresMs = 0;
};

class SnapshotLruCache {
public:
    explicit SnapshotLruCache(size_t maxEntries = 64)
        : _maxEntries(std::max<size_t>(1, maxEntries)), _rev(0)
    {
    }

    bool get(const std::string &key, uint64_t curRev, int64_t nowMs,
             CachedPayload &out)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (curRev != _rev) {
            _entries.clear();
            _lru.clear();
            _rev = curRev;
            return false;
        }
        auto it = _entries.find(key);
        if (it == _entries.end()) {
            return false;
        }
        const CachedPayload &entry = it->second.first;
        if (entry.expiresMs > 0 &&
            (nowMs >= entry.expiresMs || nowMs + 60000 < entry.expiresMs)) {
            _lru.erase(it->second.second);
            _entries.erase(it);
            return false;
        }
        _lru.erase(it->second.second);
        _lru.push_front(key);
        it->second.second = _lru.begin();
        out = entry;
        return true;
    }

    CachedPayload put(const std::string &key, uint64_t curRev, int64_t ttlMs,
                      int64_t nowMs, const char *etagPrefix,
                      std::string &&jsonBytes)
    {
        CachedPayload entry;
        entry.jsonBytes = std::move(jsonBytes);
        entry.etag = makePayloadEtag(etagPrefix, entry.jsonBytes);
        entry.gzipBytes = compressGzipFast(entry.jsonBytes);
        entry.rev = curRev;
        entry.expiresMs = (ttlMs > 0) ? (nowMs + ttlMs) : 0;

        std::lock_guard<std::mutex> lock(_mutex);
        if (curRev != _rev) {
            _entries.clear();
            _lru.clear();
            _rev = curRev;
        }
        auto existing = _entries.find(key);
        if (existing != _entries.end()) {
            _lru.erase(existing->second.second);
            _entries.erase(existing);
        }
        while (_entries.size() >= _maxEntries && !_lru.empty()) {
            const std::string &oldestKey = _lru.back();
            _entries.erase(oldestKey);
            _lru.pop_back();
        }
        _lru.push_front(key);
        _entries.emplace(key, std::make_pair(entry, _lru.begin()));
        return entry;
    }

private:
    size_t _maxEntries;
    uint64_t _rev;
    mutable std::mutex _mutex;
    std::list<std::string> _lru;
    std::unordered_map<std::string,
                       std::pair<CachedPayload, std::list<std::string>::iterator>>
        _entries;
};

void sendCachedSnapshot(const Request &req, Response &res,
                        const CachedPayload &snap, const char *cacheControl)
{
    res.set_header("Cache-Control", cacheControl);
    res.set_header("ETag", snap.etag);
    res.set_header("Vary", "Accept-Encoding");
    if (ifNoneMatchMatches(req, snap.etag)) {
        res.status = 304;
        return;
    }
    res.status = 200;
    if (!snap.gzipBytes.empty() &&
        snap.jsonBytes.size() >= kMinDynamicGzipBytes &&
        clientAcceptsGzip(req)) {
        res.set_header("Content-Encoding", "gzip");
        res.set_content(snap.gzipBytes, "application/json");
    } else {
        res.set_content(snap.jsonBytes, "application/json");
    }
}

/* ------------------------------------------------------------------ */
/* Responses                                                           */
/* ------------------------------------------------------------------ */

void sendJson(Response &res, int status, const Json &body)
{
    res.status = status;
    res.set_header("Cache-Control", "no-store");
    res.set_content(dumpJson(body), "application/json");
}

void sendError(Response &res, int status, const std::string &code,
               const std::string &message)
{
    Json j;
    j["error"] = code;
    j["message"] = message;
    sendJson(res, status, j);
}

int statusFor(RoomCode c)
{
    switch (c) {
    case RoomCode::OK:
        return 200;
    case RoomCode::UNKNOWN_PLAYER:
        return 401;
    case RoomCode::NOT_FOUND:
        return 404;
    case RoomCode::INVALID_ARGUMENT:
    case RoomCode::ILLEGAL_SYNTAX:
    case RoomCode::ILLEGAL_MOVE:
        return 400;
    case RoomCode::FORBIDDEN_BANNED:
    case RoomCode::FORBIDDEN_NOT_SEATED:
    case RoomCode::FORBIDDEN_NOT_PARTICIPANT:
    case RoomCode::FORBIDDEN_MUTED:
    case RoomCode::FORBIDDEN_POLICY:
    case RoomCode::FORBIDDEN_SEATED_TARGET:
    case RoomCode::FORBIDDEN_PASSCODE:
    case RoomCode::FORBIDDEN_NOT_HOST:
    case RoomCode::ROBOTS_NOT_ALLOWED:
    case RoomCode::NOT_YOUR_TURN:
        return 403;
    case RoomCode::INVALID_STATE:
    case RoomCode::SEAT_TAKEN:
    case RoomCode::ALREADY_SEATED:
    case RoomCode::CONFLICT:
        return 409;
    case RoomCode::PAYLOAD_TOO_LARGE:
        return 413;
    case RoomCode::RATE_LIMITED:
        return 429;
    case RoomCode::LIMIT_REACHED:
        return 503;
    case RoomCode::INTERNAL_ERROR:
        break;
    }
    return 500;
}

void sendRoomError(Response &res, const RoomResult &r)
{
    sendError(res, statusFor(r.code), toString(r.code), r.message);
    if (r.code == RoomCode::RATE_LIMITED) {
        res.set_header("Retry-After", "5");
    }
}

int statusForVerify(VerifyStatus s)
{
    switch (s) {
    case VerifyStatus::OK:
        return 200;
    case VerifyStatus::MALFORMED:
    case VerifyStatus::UNSUPPORTED_PROVIDER:
    case VerifyStatus::PROVIDER_DISABLED:
        return 400;
    case VerifyStatus::KEY_SET_UNAVAILABLE:
    case VerifyStatus::REMOTE_FAILURE:
        return 503;
    default:
        break;
    }
    return 401;
}

/* ------------------------------------------------------------------ */
/* JSON views                                                          */
/* ------------------------------------------------------------------ */

Json profileJson(const PlayerProfile &p, bool includeSeen)
{
    Json j;
    j["uuid"] = p.uuid;
    j["handle"] = p.handle;
    j["display_name"] = p.displayName;
    j["avatar_url"] = p.avatarUrl;
    j["location"] = p.location;
    j["bio"] = p.bio;
    j["elo_2p"] = p.elo2p3b;
    j["elo_2p_3b"] = p.elo2p3b;
    j["elo_2p_1b"] = p.elo2p1b;
    j["elo_4p"] = p.elo4p;
    j["elo_ofk"] = p.eloOfk;
    j["games_2p_3b"] = p.games2p3b;
    j["games_2p_1b"] = p.games2p1b;
    j["games_ofk"] = p.gamesOfk;
    j["games_played"] = p.gamesPlayed;
    j["wins"] = p.wins;
    j["losses"] = p.losses;
    j["draws"] = p.draws;
    j["arena_elo_2p_3b"] = p.arenaElo2p3b;
    j["arena_elo_2p_1b"] = p.arenaElo2p1b;
    j["arena_elo_ofk"] = p.arenaEloOfk;
    j["arena_games_2p_3b"] = p.arenaGames2p3b;
    j["arena_games_2p_1b"] = p.arenaGames2p1b;
    j["arena_games_ofk"] = p.arenaGamesOfk;
    j["arena_games_played"] = p.arenaGamesPlayed;
    j["arena_wins"] = p.arenaWins;
    j["arena_losses"] = p.arenaLosses;
    j["arena_draws"] = p.arenaDraws;
    j["is_admin"] = p.isAdmin;
    j["created_at"] = p.createdAt;
    if (includeSeen) {
        j["last_seen_at"] = p.lastSeenAt;
    }
    return j;
}

Json identitiesJson(const std::vector<LinkedIdentity> &ids)
{
    Json arr = Json::array();
    for (size_t i = 0; i < ids.size(); ++i) {
        Json j;
        j["provider"] = ids[i].provider;
        j["provider_email"] = ids[i].providerEmail;
        j["email"] = ids[i].providerEmail;
        j["display_hint"] = ids[i].displayHint;
        j["linked_at"] = ids[i].linkedAt;
        j["last_login_at"] = ids[i].lastLoginAt;
        arr.push_back(j);
    }
    return arr;
}

Json configJson(const GameTableConfig &c)
{
    Json j;
    j["mode"] = modeName(c.mode);
    j["bases"] = static_cast<int>(c.bases);
    j["super_prongs"] = c.useSuperProngs;
    j["edgeless"] = c.useEdgelessBoard;
    j["variant"] = octiVariantCode(c.mode, c.bases,
                                   c.useSuperProngs, c.useEdgelessBoard);
    Json tc;
    tc["enabled"] = c.timeControl.enabled;
    tc["initial_ms"] = c.timeControl.initialMs;
    tc["increment_ms"] = c.timeControl.incrementMs;
    tc["disconnect_grace_ms"] = c.timeControl.disconnectGraceMs;
    j["time_control"] = tc;
    j["rated"] = c.rated;
    j["allow_takeback"] = c.allowTakeback;
    j["allow_robots"] = c.allowRobots;
    j["observer_policy"] = toString(c.observerPolicy);
    return j;
}

/* ------------------------------------------------------------------ */
/* Request parsing                                                     */
/* ------------------------------------------------------------------ */

bool isJsonContentType(const std::string &contentType)
{
    std::string ct = lower(contentType);
    const std::string want = "application/json";
    if (ct.compare(0, want.size(), want) != 0) {
        return false;
    }
    return ct.size() == want.size() || ct[want.size()] == ';' ||
           ct[want.size()] == ' ';
}

bool isFormUrlEncodedContentType(const std::string &contentType)
{
    std::string ct = lower(contentType);
    const std::string want = "application/x-www-form-urlencoded";
    if (ct.compare(0, want.size(), want) != 0) {
        return false;
    }
    return ct.size() == want.size() || ct[want.size()] == ';' ||
           ct[want.size()] == ' ';
}

bool parseTimeControl(const Json &b, TimeControlConfig &tc, std::string &err)
{
    tc = TimeControlConfig();
    Json::const_iterator it = b.find("time_control");
    if (it == b.end() || it->is_null()) {
        return true;
    }
    if (it->is_string()) {
        std::string s = it->get<std::string>();
        if (s.empty() || s == "untimed") {
            return true;
        }
        size_t plus = s.find('+');
        int64_t base = 0;
        int64_t inc = 0;
        if (plus == std::string::npos ||
            !parseUnsignedStrict(s.substr(0, plus), 14400, base) ||
            !parseUnsignedStrict(s.substr(plus + 1), 14400, inc) || base < 1) {
            err = "time_control must be 'untimed', '<seconds>+<increment "
                  "seconds>' or an object";
            return false;
        }
        tc.enabled = true;
        tc.initialMs = base * 1000;
        tc.incrementMs = inc * 1000;
        return true;
    }
    if (it->is_object()) {
        int64_t initial = 0;
        int64_t increment = 0;
        int64_t grace = tc.disconnectGraceMs;
        if (!fieldInt(*it, "initial_ms", true, initial, err) ||
            !fieldInt(*it, "increment_ms", false, increment, err) ||
            !fieldInt(*it, "disconnect_grace_ms", false, grace, err)) {
            return false;
        }
        tc.enabled = true;
        tc.initialMs = initial;
        tc.incrementMs = increment;
        tc.disconnectGraceMs = grace;
        return true;
    }
    err = "time_control must be a string or an object";
    return false;
}

bool parseTableConfig(const Json &b, GameTableConfig &cfg, std::string &err)
{
    cfg = GameTableConfig();
    std::string variantText;
    if (!fieldString(b, "variant", false, variantText, err)) {
        return false;
    }
    if (!variantText.empty() &&
        !parseOctiVariantCode(variantText, cfg.mode, cfg.bases,
                              cfg.useSuperProngs, cfg.useEdgelessBoard)) {
        err = "variant must be one of the 13 canonical OCTI variant codes";
        return false;
    }
    std::string modeText;
    if (!fieldString(b, "mode", false, modeText, err)) {
        return false;
    }
    if (!modeText.empty() && !parseModeName(modeText, cfg.mode)) {
        err = "mode must be one of 2P, 4P, OFK";
        return false;
    }
    int64_t bases = static_cast<int64_t>(cfg.bases);
    if (!fieldInt(b, "bases", false, bases, err)) {
        return false;
    }
    if (bases != 1 && bases != 3) {
        err = "bases must be 1 or 3";
        return false;
    }
    cfg.bases = bases == 1 ? GAME_BASES_1 : GAME_BASES_3;
    if (!fieldBool(b, "super_prongs", false, cfg.useSuperProngs, err) ||
        !fieldBool(b, "edgeless", false, cfg.useEdgelessBoard, err) ||
        !fieldBool(b, "rated", false, cfg.rated, err) ||
        !fieldBool(b, "allow_takeback", false, cfg.allowTakeback, err) ||
        !fieldBool(b, "allow_robots", false, cfg.allowRobots, err) ||
        !parseTimeControl(b, cfg.timeControl, err)) {
        return false;
    }
    std::string text;
    if (!fieldString(b, "observer_policy", false, text, err)) {
        return false;
    }
    if (!text.empty() && !parseObserverPolicy(text, cfg.observerPolicy)) {
        err = "observer_policy must be OPEN, SILENT_OBSERVERS or NO_OBSERVERS";
        return false;
    }
    return fieldString(b, "passcode", false, cfg.passcode, err);
}

/* ------------------------------------------------------------------ */
/* Event streams                                                       */
/* ------------------------------------------------------------------ */

struct Frame {
    int64_t seq = 0;
    std::string type;
    std::string json;           // compact single-line JSON
};

struct PollBatch {
    std::vector<Frame> frames;
    int64_t latestSeq = 0;
    int64_t nextSeq = 0;        // cursor to resume from
    bool gap = false;
    bool hasMore = false;
    std::string errorCode;
    std::string message;
};

typedef std::function<int(int64_t since, size_t limit, PollBatch &out)>
    PollFn;

/* Holds one SSE slot (global, per-IP, and per-player) via RAII. */
struct SseStreamSlotGuard {
    std::atomic<int> &active;
    std::mutex &mutex;
    std::map<std::string, int> &byIp;
    std::map<int64_t, int> &byPlayer;
    std::string ip;
    int64_t playerId;

    SseStreamSlotGuard(std::atomic<int> &act, std::mutex &m,
                       std::map<std::string, int> &ipMap,
                       std::map<int64_t, int> &plMap,
                       const std::string &ipKey, int64_t pid)
        : active(act), mutex(m), byIp(ipMap), byPlayer(plMap),
          ip(ipKey), playerId(pid)
    {
    }

    ~SseStreamSlotGuard()
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (!ip.empty()) {
            auto it = byIp.find(ip);
            if (it != byIp.end()) {
                if (--it->second <= 0) {
                    byIp.erase(it);
                }
            }
        }
        if (playerId != 0) {
            auto it = byPlayer.find(playerId);
            if (it != byPlayer.end()) {
                if (--it->second <= 0) {
                    byPlayer.erase(it);
                }
            }
        }
        --active;
    }

    SseStreamSlotGuard(const SseStreamSlotGuard &) = delete;
    SseStreamSlotGuard &operator=(const SseStreamSlotGuard &) = delete;
};

std::string generateLeaseId()
{
    unsigned char b[16];
    if (RAND_bytes(b, sizeof(b)) != 1) {
        return std::string(32, '0');
    }
    static const char hex[] = "0123456789abcdef";
    std::string out;
    out.reserve(32);
    for (size_t i = 0; i < sizeof(b); ++i) {
        out.push_back(hex[(b[i] >> 4) & 0x0F]);
        out.push_back(hex[b[i] & 0x0F]);
    }
    return out;
}

struct StreamState {
    std::shared_ptr<SseStreamSlotGuard> slot;
    PollFn fetch;
    int64_t cursor = 0;
    std::string token;          // empty for anonymous streams
    int64_t playerId = 0;
    std::string playerUuid;
    std::string remoteIp;
    std::string clientType;
    int64_t startedMs = 0;
    int64_t lastWriteMs = 0;
    int64_t lastAuthMs = 0;
    bool first = true;
    bool gapSent = false;
    bool isRobot = false;
    int64_t robotId = 0;
    std::string robotUuid;
    std::string ownerUuid;
    std::string leaseId;
    std::shared_ptr<void> robotStreamGuard;
};

std::string sseFrame(const Frame &f)
{
    return "id: " + std::to_string(f.seq) + "\nevent: " + f.type +
           "\ndata: " + f.json + "\n\n";
}

bool putText(httplib::DataSink &sink, const std::string &text)
{
    return sink.write(text.data(), text.size());
}

} // namespace

/* ------------------------------------------------------------------ */
/* Implementation                                                      */
/* ------------------------------------------------------------------ */

struct OctiHttpServer::Impl {
    struct ActiveRobotLease {
        std::string leaseId;
        int64_t connectedAtMs = 0;
        int64_t lastSeenMs = 0;
        std::string clientIp;
        std::string clientType = "robot";
        int activeStreams = 0;
    };

    struct RobotStreamGuard {
        std::mutex &mutex;
        std::map<int64_t, ActiveRobotLease> &leases;
        int64_t robotId;
        RobotStreamGuard(std::mutex &m, std::map<int64_t, ActiveRobotLease> &l, int64_t rId)
            : mutex(m), leases(l), robotId(rId)
        {
            std::lock_guard<std::mutex> lock(mutex);
            auto it = leases.find(robotId);
            if (it != leases.end()) {
                ++it->second.activeStreams;
            }
        }
        ~RobotStreamGuard()
        {
            std::lock_guard<std::mutex> lock(mutex);
            auto it = leases.find(robotId);
            if (it != leases.end() && it->second.activeStreams > 0) {
                --it->second.activeStreams;
            }
        }
    };

    struct UrlProbeSlotGuard {
        std::atomic<int> &counter;
        bool unlimited;
        bool acquired;
        UrlProbeSlotGuard(std::atomic<int> &c, int maxConcurrent)
            : counter(c), unlimited(maxConcurrent <= 0), acquired(false)
        {
            if (unlimited) {
                return;
            }
            int cur = counter.load(std::memory_order_relaxed);
            while (cur < maxConcurrent) {
                if (counter.compare_exchange_weak(
                        cur, cur + 1,
                        std::memory_order_acquire,
                        std::memory_order_relaxed)) {
                    acquired = true;
                    return;
                }
            }
        }
        ~UrlProbeSlotGuard()
        {
            if (acquired) {
                counter.fetch_sub(1, std::memory_order_release);
            }
        }
        UrlProbeSlotGuard(const UrlProbeSlotGuard &) = delete;
        UrlProbeSlotGuard &operator=(const UrlProbeSlotGuard &) = delete;
        bool allowed() const { return unlimited || acquired; }
    };

    struct Auth {
        int64_t playerId = 0;
        std::string token;
        std::string clientType;
        bool isRobot = false;
        RobotRecord robotRecord;
    };

    Impl(AccountStore &s, OidcVerifier &v, GameRoomManager &r,
         const HttpServerConfig &c);

    /* lifecycle */
    bool start(std::string &error);
    void stop();
    void tickLoop();
    void registerRoutes();

    /* identity cache */
    std::string uuidOf(int64_t playerId);
    bool playerIdOfUuid(const std::string &uuid, int64_t &playerId);
    bool resolveTarget(const Json &body, const char *key, Response &res,
                       int64_t &playerId);

    /* auth + parsing */
    std::string clientRemoteIp(const Request &req) const;
    bool isHttpsRequest(const Request &req) const;
    size_t routeMaxBodyBytes(const std::string &method,
                             const std::string &path) const;
    bool hasValidBearerAuth(const Request &req);
    static bool extractBearer(const Request &req, std::string &token);
    void maybeBindClientInstallId(const Request &req, int64_t playerId,
                                  const std::string &bodyInstallId = "");
    void sendAuthUnauthorized(const Request &req, Response &res,
                              const char *wwwAuth, const std::string &message,
                              bool tokenAttempted);
    bool requireAuth(const Request &req, Response &res, Auth &auth);
    bool optionalAuth(const Request &req, Response &res, Auth &auth);
    bool readJsonBody(const Request &req, Response &res, Json &out,
                      bool allowEmpty);
    bool pathId(const Request &req, const char *name, Response &res,
                std::string &out);
    bool verifyCredential(const Request &req, const Json &body, Response &res,
                          VerifiedClaims &claims);
    void sendIpBanned(Response &res, const BanMatch &m, int64_t nowSec);
    void sendAccountBanned(Response &res, const BanMatch *m, int64_t nowSec);
    AbuseActorContext buildAbuseContext(const Request &req,
                                        int64_t actorId = 0);
    void applyBanSideEffects(const BanSideEffects &fx);
    void noteRateLimit429(const Request &req, int64_t actorId = 0);
    bool requireAdminAuth(const Request &req, Response &res, Auth &auth,
                          std::string &actorLabelOut);
    Json buildBansPayloadJson(int64_t nowSec);
    bool allowCredentialAttempt(const Request &req, Response &res);
    bool allowUploadAttempt(const Request &req, Response &res);
    bool allowExportAttempt(const Request &req, Response &res,
                            int64_t actorId = 0);
    bool allowRobotExportConfAttempt(const Request &req, Response &res,
                                     int64_t actorId);
    bool allowPliesAttempt(const Request &req, Response &res);
    bool allowProbeAttempt(const Request &req, Response &res, int64_t actorId);
    bool allowMutationAttempt(const Request &req, Response &res,
                              int64_t actorId);
    bool checkPasscodeBlocked(const Request &req, Response &res,
                              int64_t actorId);
    void recordPasscodeFailure(const Request &req, Response &res,
                               int64_t actorId, const RoomResult &r);
    bool allowLivePoll(const Request &req, Response &res, const Auth &auth);
    bool allowLobbySnap(const Request &req, Response &res, const Auth &auth);
    bool allowBrowseRead(const Request &req, Response &res, const Auth &auth);
    bool tryAcquireSseSlot(const std::string &ip, int64_t playerId,
                           std::shared_ptr<SseStreamSlotGuard> &outSlot,
                           int &outStatus, std::string &outCode,
                           std::string &outMsg);

    /* json views needing the cache */
    static void populateHumanEntitySummaryJson(Json &target,
                                               const PlayerProfile &prof);
    static void populateRobotEntitySummaryJson(
        Json &target, const RobotRecord &rRec,
        const std::string &fallbackAvatarUrl = "");
    bool populateActorEntitySummaryJson(int64_t actorId,
                                        const std::string &fallbackHandle,
                                        const std::string &fallbackRobotUuid,
                                        const std::string &fallbackAvatarUrl,
                                        Json &target);
    Json tableJson(const TableSummary &t);
    Json seatJson(const SeatView &s);
    Json roomViewJson(const RoomView &v);
    Json roomEventJson(const RoomEvent &e);
    Json lobbyEventJson(const LobbyEvent &e);
    Json gameSummaryJson(const GameRecord &g);
    Json buildRobotProfilePayload(const RobotRecord &r);

    /* streaming */
    bool wantsStream(const Request &req) const;
    void respondEvents(const Request &req, Response &res, const Auth &auth,
                       PollFn fetch);
    bool streamStep(StreamState &st, httplib::DataSink &sink);
    void waitWake(int ms);
    uint64_t currentEventWakeGen();
    void waitEventOrStop(uint64_t &seenGen, int64_t ms);

    /* handlers */
    void handleRobotsTxt(const Request &req, Response &res);
    void handleHealth(const Request &req, Response &res);
    void handleAvatarGet(const Request &req, Response &res);
    void handleSoundAssetGet(const Request &req, Response &res);
    void handleRulebookAssetGet(const Request &req, Response &res);
    void handlePrivacy(const Request &req, Response &res);
    void handleDataDeletion(const Request &req, Response &res);
    void handleAuthAuthorize(const Request &req, Response &res);
    void handleAuthCallback(const Request &req, Response &res);
    bool validateOauthHostAndCallbackUrl(const Request &req,
                                         std::string &outCallbackUrl) const;
    void handleProviders(const Request &req, Response &res);
    void handleSso(const Request &req, Response &res);
    void handleLink(const Request &req, Response &res);
    void handleUnlink(const Request &req, Response &res);
    void handleAuthListIdentities(const Request &req, Response &res);
    void handleAuthUnlinkProvider(const Request &req, Response &res);
    void handleLogout(const Request &req, Response &res);
    void handleProfileGet(const Request &req, Response &res);
    void handleProfilePatch(const Request &req, Response &res);
    void handlePlayer(const Request &req, Response &res);
    void handleUsersList(const Request &req, Response &res);
    void handleUserGet(const Request &req, Response &res);
    void handleRobotsList(const Request &req, Response &res);
    void handleRobotsDirectory(const Request &req, Response &res);
    void handleRobotProfileGet(const Request &req, Response &res);
    void handleRobotCreate(const Request &req, Response &res);
    void handleRobotPatch(const Request &req, Response &res);
    void handleRobotDelete(const Request &req, Response &res);
    void handleRobotsConnect(const Request &req, Response &res);
    void handleRobotsDisconnect(const Request &req, Response &res);
    void handleRobotsExportConf(const Request &req, Response &res);
    void handleLeaderboard(const Request &req, Response &res);
    void handleLobbyState(const Request &req, Response &res);
    void handleLobbyChat(const Request &req, Response &res);
    void handleLobbyEvents(const Request &req, Response &res);
    void handleTables(const Request &req, Response &res);
    void handleTableCreate(const Request &req, Response &res);
    void handleTableJoin(const Request &req, Response &res);
    void handleTableLeave(const Request &req, Response &res);
    void handleTablePurge(const Request &req, Response &res);
    void handleTableKick(const Request &req, Response &res);
    void handleTableObserve(const Request &req, Response &res);
    void handleTableUnobserve(const Request &req, Response &res);
    void handleEnqueue(const Request &req, Response &res);
    void handleDequeue(const Request &req, Response &res);
    void handleMatchStatus(const Request &req, Response &res);
    void handleGameGet(const Request &req, Response &res);
    void handleMove(const Request &req, Response &res);
    void handleResign(const Request &req, Response &res);
    void handleAction(const Request &req, Response &res);
    void handleChat(const Request &req, Response &res);
    void handleBan(const Request &req, Response &res);
    void handleMute(const Request &req, Response &res);
    void handleSettings(const Request &req, Response &res);
    void handleGameEvents(const Request &req, Response &res);
    void handleArchiveUpload(const Request &req, Response &res);
    void handleArchiveDeleteInstall(const Request &req, Response &res);
    void handleArchivePurgeGame(const Request &req, Response &res);
    void handleAdminRecomputeRatings(const Request &req, Response &res);
    void handleAdminBansList(const Request &req, Response &res);
    void handleAdminBanAdd(const Request &req, Response &res);
    void handleAdminBansMove(const Request &req, Response &res);
    void handleAdminBansRemove(const Request &req, Response &res);
    void handleAdminBanPatch(const Request &req, Response &res);
    void handleAdminBanDelete(const Request &req, Response &res);
    void handleArchiveList(const Request &req, Response &res);
    void handleArchiveGet(const Request &req, Response &res);
    void handleArchiveTranscript(const Request &req, Response &res);
    void handleArchiveOcti(const Request &req, Response &res);
    void handleArchivePlies(const Request &req, Response &res);
    void handleArchiveStats(const Request &req, Response &res);
    void handleArchiveExport(const Request &req, Response &res);
    void handleTelemetryCrash(const Request &req, Response &res);
    void handleTelemetryCrashes(const Request &req, Response &res);
    void handleTelemetryCrashGet(const Request &req, Response &res);
    bool pathCrashId(const Request &req, Response &res, std::string &out);
    void handleFavicon(const Request &req, Response &res);
    void handleIndex(const Request &req, Response &res);

    void finishRoom(const Request &req, int64_t actorId, Response &res,
                    const RoomResult &r, Json body);
    void finishRoom(Response &res, const RoomResult &r, Json body);

    AccountStore &store;
    OidcVerifier &verifier;
    GameRoomManager &rooms;
    HttpServerConfig cfg;
    BanManager banManager;
    httplib::Server server;

    std::thread listenThread;
    std::thread tickThread;
    std::mutex lifecycleMutex;
    bool started = false;
    bool stopped = false;
    std::atomic<bool> stopping;
    std::atomic<int> boundPort;
    std::atomic<uint64_t> ticks;
    std::atomic<int> sseActive;
    std::atomic<int> activeUrlProbes;
    std::atomic<int> activeExportsCount;
    int64_t startedAtSec = 0;

    std::string indexGzipBytes;
    std::string indexEtag;

    std::mutex wakeMutex;
    std::condition_variable wakeCv;
    uint64_t eventWakeGen = 0;

    std::vector<std::string> normalizedTrustedProxies;
    std::vector<std::string> normalizedAllowedHosts;
    std::array<uint8_t, 32> oauthSecret;

    struct PendingOauthState {
        int64_t expiresAtSec = 0;
        uint64_t seq = 0;
        std::string oidcNonce;
    };
    std::mutex oauthStateMutex;
    uint64_t oauthStateSeq = 0;
    std::unordered_map<std::string, PendingOauthState> pendingOauthStates;
    std::deque<std::pair<uint64_t, std::string> > oauthStateOrder;

    std::mutex sseQuotaMutex;
    std::map<std::string, int> sseByIp;
    std::map<int64_t, int> sseByPlayer;

    TokenBucketLimiter livePollLimiter;
    TokenBucketLimiter lobbySnapLimiter;
    TokenBucketLimiter browseLimiter;

    IpRateLimiter credentialLimiter;
    IpRateLimiter authFailureLimiter;
    IpRateLimiter passcodeFailureLimiter;
    IpRateLimiter uploadLimiter;
    IpRateLimiter exportLimiter;
    IpRateLimiter robotExportConfLimiter;
    IpRateLimiter pliesLimiter;
    IpRateLimiter probeLimiter;
    IpRateLimiter mutationLimiter;

    SnapshotLruCache lobbyStateCache{1};
    SnapshotLruCache tablesCache{4};
    SnapshotLruCache robotsDirCache{64};
    SnapshotLruCache leaderboardCache{64};
    SnapshotLruCache pliesCache{128};
    SnapshotLruCache archiveStatsCache{1};

    std::mutex cacheMutex;
    std::map<int64_t, std::string> uuidById;
    std::map<std::string, int64_t> idByUuid;

    std::mutex _robotLeaseMutex;
    std::map<int64_t, ActiveRobotLease> _activeRobotLeases;
};

OctiHttpServer::Impl::Impl(AccountStore &s, OidcVerifier &v,
                           GameRoomManager &r, const HttpServerConfig &c)
    : store(s), verifier(v), rooms(r), cfg(c), stopping(false), boundPort(0),
      ticks(0), sseActive(0), activeUrlProbes(0), activeExportsCount(0)
{
    const char *rawHtml = webUiHtml();
    const size_t rawHtmlLen = rawHtml ? std::strlen(rawHtml) : 0;
    const std::string htmlSlice(rawHtml ? rawHtml : "", rawHtmlLen);
    indexEtag = makePayloadEtag("ui-", htmlSlice);
    if (!compressGzipWithLevel(htmlSlice.data(), htmlSlice.size(),
                               Z_DEFAULT_COMPRESSION, indexGzipBytes)) {
        throw std::runtime_error("zlib startup compression failed for WebUI");
    }

    rooms.setAnonymizeObserverHandles(cfg.anonymizeObserverHandles);
    rooms.setEventWakeCallback([this]() {
        {
            std::lock_guard<std::mutex> wake(wakeMutex);
            ++eventWakeGen;
        }
        wakeCv.notify_all();
    });

    oauthSecret.fill(0);
    if (!cfg.oauthStateSecret.empty()) {
        unsigned int mdLen = 0;
        if (EVP_Digest(cfg.oauthStateSecret.data(),
                       cfg.oauthStateSecret.size(), oauthSecret.data(),
                       &mdLen, EVP_sha256(), nullptr) != 1 ||
            mdLen != oauthSecret.size()) {
            throw std::runtime_error("EVP_Digest failed for OAuth state secret");
        }
    } else {
        if (RAND_bytes(oauthSecret.data(),
                       static_cast<int>(oauthSecret.size())) != 1) {
            throw std::runtime_error("RAND_bytes failed for OAuth state secret");
        }
    }

    auto addAllowedHost = [this](const std::string &h) {
        std::string hostOnly;
        if (splitAuthorityHostOnly(h, hostOnly) && !hostOnly.empty() &&
            std::find(normalizedAllowedHosts.begin(),
                      normalizedAllowedHosts.end(),
                      hostOnly) == normalizedAllowedHosts.end()) {
            normalizedAllowedHosts.push_back(hostOnly);
        }
    };
    for (size_t i = 0; i < cfg.allowedHosts.size(); ++i) {
        addAllowedHost(cfg.allowedHosts[i]);
    }
    if (!cfg.bindAddress.empty() && cfg.bindAddress != "0.0.0.0" &&
        cfg.bindAddress != "::") {
        if (cfg.bindAddress.find(':') != std::string::npos &&
            cfg.bindAddress[0] != '[') {
            addAllowedHost("[" + cfg.bindAddress + "]");
        } else {
            addAllowedHost(cfg.bindAddress);
        }
    }
    if (!cfg.publicBaseUrl.empty()) {
        std::string pb = cfg.publicBaseUrl;
        size_t schemePos = pb.find("://");
        if (schemePos != std::string::npos) {
            size_t authStart = schemePos + 3;
            size_t slashPos = pb.find('/', authStart);
            std::string authPart = (slashPos == std::string::npos)
                ? pb.substr(authStart)
                : pb.substr(authStart, slashPos - authStart);
            addAllowedHost(authPart);
        }
    }

    for (size_t i = 0; i < cfg.trustedProxies.size(); ++i) {
        std::string norm = OctiHttpServer::normalizeIpLiteral(
            cfg.trustedProxies[i]);
        if (!norm.empty() &&
            std::find(normalizedTrustedProxies.begin(),
                      normalizedTrustedProxies.end(),
                      norm) == normalizedTrustedProxies.end()) {
            normalizedTrustedProxies.push_back(norm);
        }
    }
    std::string effectiveBanPath = cfg.banConfigPath;
    if (effectiveBanPath.empty()) {
        const std::string storePath = store.dbPath();
        if (!storePath.empty()) {
            std::filesystem::path parent =
                std::filesystem::path(storePath).parent_path();
            if (parent.empty()) {
                parent = ".";
            }
            effectiveBanPath = (parent / "bans.json").string();
        }
    }
    banManager.configure(effectiveBanPath, cfg.bindAddress,
                         normalizedTrustedProxies);
    if (cfg.maxWorkerThreads < 4) {
        cfg.maxWorkerThreads = 4;
    }
    if (cfg.workerThreads < 2) {
        cfg.workerThreads = 2;
    }
    if (cfg.workerThreads > cfg.maxWorkerThreads) {
        cfg.workerThreads = cfg.maxWorkerThreads;
    }
    int sseCeiling = static_cast<int>(cfg.maxWorkerThreads / 2);
    if (cfg.maxSseStreams > sseCeiling) {
        cfg.maxSseStreams = sseCeiling;
    }
    if (cfg.maxSseStreams < 0) {
        cfg.maxSseStreams = 0;
    }
    if (cfg.ssePollMs < 10) {
        cfg.ssePollMs = 10;
    }
    if (cfg.tickIntervalMs < 10) {
        cfg.tickIntervalMs = 10;
    }
    if (cfg.sseBatchLimit < 1) {
        cfg.sseBatchLimit = 1;
    }
    if (cfg.sseBatchLimit > kMaxQueryLimit) {
        cfg.sseBatchLimit = kMaxQueryLimit;
    }
}

/* ------------------------------------------------------------------ */
/* Lifecycle                                                           */
/* ------------------------------------------------------------------ */

bool OctiHttpServer::Impl::start(std::string &error)
{
    std::lock_guard<std::mutex> lock(lifecycleMutex);
    if (started || stopped) {
        error = "server already started or stopped";
        return false;
    }
    if (cfg.port < 0 || cfg.port > 65535) {
        error = "port out of range";
        return false;
    }
    if (cfg.bindAddress.empty()) {
        error = "empty bind address";
        return false;
    }
    if (!banManager.loadFromStorage(store, store.currentEpochSec(), error)) {
        return false;
    }

    /*
     * httplib defaults to SO_REUSEPORT on Linux, which lets a second
     * process bind the very same port and silently share (or steal) the
     * traffic. SO_REUSEADDR alone still allows a prompt restart.
     */
    server.set_socket_options([](socket_t sock) {
        httplib::set_socket_opt(sock, SOL_SOCKET, SO_REUSEADDR, 1);
    });
    server.set_payload_max_length(cfg.maxBodyBytes);
    server.set_keep_alive_max_count(
        static_cast<size_t>(std::max(1, cfg.keepAliveMaxCount)));
    server.set_keep_alive_timeout(std::max(1, cfg.keepAliveTimeoutSec));
    server.set_read_timeout(cfg.readTimeoutSec, 0);
    server.set_write_timeout(cfg.writeTimeoutSec, 0);
    const size_t baseThreads = cfg.workerThreads;
    const size_t maxThreads = cfg.maxWorkerThreads;
    const size_t maxQueued = cfg.maxQueuedRequests;
    server.new_task_queue = [baseThreads, maxThreads, maxQueued] {
        return new httplib::ThreadPool(baseThreads, maxThreads, maxQueued);
    };
    registerRoutes();

    int bound = 0;
    errno = 0;
    if (cfg.port == 0) {
        bound = server.bind_to_any_port(cfg.bindAddress);
    } else if (server.bind_to_port(cfg.bindAddress, cfg.port)) {
        bound = cfg.port;
    }
    if (bound <= 0) {
        error = "cannot bind " + cfg.bindAddress + ":" +
                std::to_string(cfg.port) +
                (errno != 0 ? std::string(" (") + std::strerror(errno) + ")"
                            : std::string());
        return false;
    }
    boundPort = bound;
    startedAtSec = epochSec();
    stopping = false;

    listenThread = std::thread([this]() { server.listen_after_bind(); });
    server.wait_until_ready();
    if (!server.is_running()) {
        if (listenThread.joinable()) {
            listenThread.join();
        }
        boundPort = 0;
        error = "listener failed to start";
        return false;
    }
    tickThread = std::thread([this]() { tickLoop(); });
    started = true;
    return true;
}

void OctiHttpServer::Impl::stop()
{
    std::lock_guard<std::mutex> lock(lifecycleMutex);
    if (stopped) {
        return;
    }
    rooms.setEventWakeCallback(nullptr);
    if (!started) {
        stopped = true;
        return;
    }
    stopped = true;
    {
        std::lock_guard<std::mutex> wake(wakeMutex);
        stopping = true;
        ++eventWakeGen;
    }
    wakeCv.notify_all();
    for (int i = 0; i < 250 &&
                    (sseActive.load() > 0 || activeExportsCount.load() > 0);
         ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    server.stop();
    if (listenThread.joinable()) {
        listenThread.join();
    }
    if (tickThread.joinable()) {
        tickThread.join();
    }
    rooms.checkpointToStore();
}

void OctiHttpServer::Impl::waitWake(int ms)
{
    std::unique_lock<std::mutex> lock(wakeMutex);
    wakeCv.wait_for(lock, std::chrono::milliseconds(ms),
                    [this]() { return stopping.load(); });
}

uint64_t OctiHttpServer::Impl::currentEventWakeGen()
{
    std::lock_guard<std::mutex> lock(wakeMutex);
    return eventWakeGen;
}

void OctiHttpServer::Impl::waitEventOrStop(uint64_t &seenGen, int64_t ms)
{
    if (ms <= 0) {
        return;
    }
    std::unique_lock<std::mutex> lock(wakeMutex);
    wakeCv.wait_for(lock, std::chrono::milliseconds(ms),
                    [this, &seenGen]() {
                        return stopping.load() || eventWakeGen != seenGen;
                    });
    seenGen = eventWakeGen;
}

void OctiHttpServer::Impl::tickLoop()
{
    int64_t lastBanPurge = 0;
    int64_t lastPurge = 0;
    int64_t lastRetentionPurge = 0;
    int64_t lastReconcile = 0;
    while (!stopping.load()) {
        waitWake(cfg.tickIntervalMs);
        if (stopping.load()) {
            break;
        }
        try {
            rooms.tick();
            int64_t now = store.currentEpochSec();
            if (cfg.tempBanPurgeIntervalSec > 0 &&
                (lastBanPurge == 0 ||
                 now - lastBanPurge >= cfg.tempBanPurgeIntervalSec)) {
                banManager.purgeExpired(store, now);
                lastBanPurge = now;
            }
            if (now - lastPurge >= cfg.sessionPurgeIntervalSec) {
                store.purgeExpiredSessions(now);
                lastPurge = now;
            }
            if (cfg.retentionPurgeIntervalSec > 0 &&
                (lastRetentionPurge == 0 ||
                 now - lastRetentionPurge >= cfg.retentionPurgeIntervalSec)) {
                store.purgeExpiredSessions(now);
                store.purgeOldCrashReports(now - cfg.crashRetentionAgeSec,
                                           cfg.retentionMaxRows);
                store.purgeLobbyChatLogsBefore(
                    now * 1000LL - cfg.lobbyChatRetentionAgeMs,
                    cfg.retentionMaxRows);
                lastRetentionPurge = now;
            }
            if (lastReconcile == 0 || (now - lastReconcile >= cfg.reconcileIntervalSec)) {
                store.reconcileAnonymousGames();
                lastReconcile = now;
            }
        } catch (const std::exception &ex) {
            std::cerr << "octi_server: maintenance tick failed: " << ex.what()
                      << "\n";
        }
        ++ticks;
    }
}

/* ------------------------------------------------------------------ */
/* Identity cache                                                      */
/* ------------------------------------------------------------------ */

std::string OctiHttpServer::Impl::uuidOf(int64_t playerId)
{
    if (playerId == 0) {
        return std::string();
    }
    if (isRobotActorId(playerId)) {
        int64_t robotId = robotIdFromActorId(playerId);
        {
            std::lock_guard<std::mutex> lock(cacheMutex);
            std::map<int64_t, std::string>::const_iterator it =
                uuidById.find(playerId);
            if (it != uuidById.end()) {
                return it->second;
            }
        }
        RobotRecord r;
        if (!store.getRobotById(robotId, r)) {
            return std::string();
        }
        std::lock_guard<std::mutex> lock(cacheMutex);
        if (uuidById.size() >= kMaxCacheEntries) {
            uuidById.clear();
            idByUuid.clear();
        }
        uuidById[playerId] = r.robotUuid;
        idByUuid[r.robotUuid] = playerId;
        return r.robotUuid;
    }
    if (playerId < 0) {
        return std::string();
    }
    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        std::map<int64_t, std::string>::const_iterator it =
            uuidById.find(playerId);
        if (it != uuidById.end()) {
            return it->second;
        }
    }
    PlayerProfile p;
    if (!store.getProfileById(playerId, p)) {
        return std::string();
    }
    std::lock_guard<std::mutex> lock(cacheMutex);
    if (uuidById.size() >= kMaxCacheEntries) {
        uuidById.clear();
        idByUuid.clear();
    }
    uuidById[playerId] = p.uuid;
    idByUuid[p.uuid] = playerId;
    return p.uuid;
}

bool OctiHttpServer::Impl::playerIdOfUuid(const std::string &uuid,
                                          int64_t &playerId)
{
    if (!isSafeId(uuid)) {
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        std::map<std::string, int64_t>::const_iterator it =
            idByUuid.find(uuid);
        if (it != idByUuid.end()) {
            playerId = it->second;
            return true;
        }
    }
    PlayerProfile p;
    if (store.getProfileByUuid(uuid, p)) {
        std::lock_guard<std::mutex> lock(cacheMutex);
        if (uuidById.size() >= kMaxCacheEntries) {
            uuidById.clear();
            idByUuid.clear();
        }
        uuidById[p.playerId] = p.uuid;
        idByUuid[p.uuid] = p.playerId;
        playerId = p.playerId;
        return true;
    }
    RobotRecord r;
    if (store.getRobotByUuid(uuid, r)) {
        int64_t actorId = makeRobotActorId(r.robotId);
        std::lock_guard<std::mutex> lock(cacheMutex);
        if (uuidById.size() >= kMaxCacheEntries) {
            uuidById.clear();
            idByUuid.clear();
        }
        uuidById[actorId] = r.robotUuid;
        idByUuid[r.robotUuid] = actorId;
        playerId = actorId;
        return true;
    }
    return false;
}

/* Reads body[key] (a player uuid) and resolves it; writes 400/404 on error. */
bool OctiHttpServer::Impl::resolveTarget(const Json &body, const char *key,
                                         Response &res, int64_t &playerId)
{
    std::string uuid;
    std::string err;
    if (!fieldString(body, key, true, uuid, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return false;
    }
    if (!playerIdOfUuid(uuid, playerId)) {
        sendError(res, 404, "UNKNOWN_TARGET", "no such player");
        return false;
    }
    return true;
}

/* ------------------------------------------------------------------ */
/* Auth and request parsing                                            */
/* ------------------------------------------------------------------ */

std::string OctiHttpServer::Impl::clientRemoteIp(const Request &req) const
{
    std::string combinedXff;
    size_t xffCount = req.get_header_value_count("X-Forwarded-For");
    for (size_t i = 0; i < xffCount; ++i) {
        if (i > 0) {
            combinedXff += ", ";
        }
        combinedXff += req.get_header_value("X-Forwarded-For", "", i);
    }
    std::string xri;
    size_t xriCount = req.get_header_value_count("X-Real-IP");
    if (xriCount == 1) {
        xri = req.get_header_value("X-Real-IP", "", 0);
    } else if (xriCount > 1) {
        xri = "invalid";
    }
    return OctiHttpServer::resolveClientIp(req.remote_addr, combinedXff, xri,
                                           normalizedTrustedProxies);
}

bool OctiHttpServer::Impl::isHttpsRequest(const Request &req) const
{
    if (cfg.behindTls) {
        return true;
    }
    const std::string peer = OctiHttpServer::normalizeIpLiteral(req.remote_addr);
    if (peer.empty()) {
        return false;
    }
    const bool trustedPeer =
        std::find(normalizedTrustedProxies.begin(),
                  normalizedTrustedProxies.end(),
                  peer) != normalizedTrustedProxies.end();
    if (!trustedPeer || !req.has_header("X-Forwarded-Proto")) {
        return false;
    }
    std::string proto = lower(trimHttpToken(
        req.get_header_value("X-Forwarded-Proto")));
    size_t comma = proto.find(',');
    if (comma != std::string::npos) {
        proto = trimHttpToken(proto.substr(0, comma));
    }
    return proto == "https";
}

size_t OctiHttpServer::Impl::routeMaxBodyBytes(const std::string &method,
                                               const std::string &path) const
{
    size_t routeCap = 16384;
    if (method == "PATCH" &&
        (path == "/api/v1/profile" || path == "/api/v1/profile/me")) {
        routeCap = 524288;
    } else if (method == "POST" && path == "/api/v1/archive/upload") {
        routeCap = 131072;
    } else if (method == "POST" && path == "/api/v1/telemetry/crash") {
        routeCap = 65536;
    }
    return std::min(cfg.maxBodyBytes, routeCap);
}

bool OctiHttpServer::Impl::hasValidBearerAuth(const Request &req)
{
    std::string token;
    if (!extractBearer(req, token)) {
        return false;
    }
    const int64_t nowSec = store.currentEpochSec();
    BanMatch bm;
    if (token.rfind("octi_bot_", 0) == 0) {
        RobotTokenValidation val = store.validateRobotToken(token);
        if (!val.valid || val.banned || val.robot.revoked) {
            return false;
        }
        if (banManager.matchRobot(val.robot.robotUuid, nowSec, bm) ||
            banManager.matchAccount(val.owner.uuid, nowSec, bm)) {
            return false;
        }
        return true;
    }
    SessionValidation sv = store.validateSession(token, nowSec);
    if (!sv.valid || sv.banned) {
        return false;
    }
    const std::string u = uuidOf(sv.playerId);
    if (!u.empty() && banManager.matchAccount(u, nowSec, bm)) {
        return false;
    }
    return true;
}

bool OctiHttpServer::Impl::extractBearer(const Request &req,
                                         std::string &token)
{
    if (!req.has_header("Authorization")) {
        return false;
    }
    std::string h = req.get_header_value("Authorization");
    const std::string scheme = "bearer ";
    if (h.size() <= scheme.size() ||
        lower(h.substr(0, scheme.size())) != scheme) {
        return false;
    }
    size_t b = scheme.size();
    while (b < h.size() && h[b] == ' ') {
        ++b;
    }
    std::string t = h.substr(b);
    if (t.empty() || t.size() > kMaxBearerBytes) {
        return false;
    }
    for (size_t i = 0; i < t.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(t[i]);
        if (c <= 0x20 || c >= 0x7F) {
            return false;
        }
    }
    token = t;
    return true;
}

void OctiHttpServer::Impl::maybeBindClientInstallId(const Request &req,
                                                    int64_t playerId,
                                                    const std::string &bodyInstallId)
{
    if (playerId <= 0) {
        return;
    }
    if (req.method == "DELETE" &&
        req.path.rfind("/api/v1/archive/install/", 0) == 0) {
        return;
    }
    std::string candidateId;
    if (req.has_header("X-Octi-Install-Id")) {
        std::string h = req.get_header_value("X-Octi-Install-Id");
        if (isSafeId(h)) {
            candidateId = h;
        }
    }
    if (candidateId.empty() && !bodyInstallId.empty() && isSafeId(bodyInstallId)) {
        candidateId = bodyInstallId;
    }
    if (!candidateId.empty()) {
        if (req.has_header("X-Octi-Install-Token")) {
            std::string tok = req.get_header_value("X-Octi-Install-Token");
            if (!store.registerOrVerifyInstallToken(candidateId, tok)) {
                return;
            }
        } else if (store.hasInstallToken(candidateId)) {
            return;
        }
        store.bindInstallToPlayer(candidateId, playerId);
    }
}

void OctiHttpServer::Impl::sendIpBanned(Response &res, const BanMatch &m,
                                        int64_t nowSec)
{
    Json j;
    j["error"] = "IP_BANNED";
    j["message"] = "IP address is banned";
    j["ban_type"] =
        (m.duration == BanDuration::PERMANENT) ? "permanent" : "24h";
    if (m.duration == BanDuration::TEMP_24H) {
        j["expires_at"] = m.expiresAt;
        const int64_t rem = std::max<int64_t>(1, m.expiresAt - nowSec);
        res.set_header("Retry-After", std::to_string(rem));
    } else {
        j["expires_at"] = 0;
    }
    sendJson(res, 403, j);
}

void OctiHttpServer::Impl::sendAccountBanned(Response &res, const BanMatch *m,
                                             int64_t nowSec)
{
    Json j;
    j["error"] = "ACCOUNT_BANNED";
    j["message"] = "Account or robot is banned";
    if (m != nullptr) {
        j["ban_type"] =
            (m->duration == BanDuration::PERMANENT) ? "permanent" : "24h";
        if (m->duration == BanDuration::TEMP_24H) {
            j["expires_at"] = m->expiresAt;
            const int64_t rem = std::max<int64_t>(1, m->expiresAt - nowSec);
            res.set_header("Retry-After", std::to_string(rem));
        } else {
            j["expires_at"] = 0;
        }
    }
    sendJson(res, 403, j);
}

AbuseActorContext OctiHttpServer::Impl::buildAbuseContext(const Request &req,
                                                          int64_t actorId)
{
    AbuseActorContext ctx;
    ctx.remoteIp = clientRemoteIp(req);
    if (actorId == 0) {
        return ctx;
    }
    if (isRobotActorId(actorId)) {
        const int64_t rId = robotIdFromActorId(actorId);
        RobotRecord rRec;
        if (rId > 0 && store.getRobotById(rId, rRec)) {
            ctx.robotId = rRec.robotId;
            ctx.robotUuid = rRec.robotUuid;
            ctx.robotName = rRec.robotName;
            ctx.ownerPlayerId = rRec.ownerPlayerId;
            ctx.isAdmin = store.isPlayerAdmin(rRec.ownerPlayerId);
        }
        return ctx;
    }
    if (actorId > 0) {
        PlayerProfile prof;
        if (store.getProfileById(actorId, prof)) {
            ctx.playerId = prof.playerId;
            ctx.playerUuid = prof.uuid;
            ctx.playerHandle = prof.handle;
            ctx.isAdmin = prof.isAdmin;
        }
    }
    return ctx;
}

void OctiHttpServer::Impl::applyBanSideEffects(const BanSideEffects &fx)
{
    if (!fx.changed) {
        return;
    }

    std::vector<int64_t> allRobotIds = fx.bannedRobotIds;
    for (size_t i = 0; i < fx.bannedPlayerIds.size(); ++i) {
        const int64_t pid = fx.bannedPlayerIds[i];
        std::vector<RobotRecord> owned;
        if (store.listRobots(pid, owned)) {
            for (size_t k = 0; k < owned.size(); ++k) {
                allRobotIds.push_back(owned[k].robotId);
            }
        }
    }

    if (!allRobotIds.empty()) {
        std::lock_guard<std::mutex> lock(_robotLeaseMutex);
        for (size_t i = 0; i < allRobotIds.size(); ++i) {
            _activeRobotLeases.erase(allRobotIds[i]);
        }
    }

    for (size_t i = 0; i < fx.bannedPlayerIds.size(); ++i) {
        const int64_t pid = fx.bannedPlayerIds[i];
        rooms.dequeueMatchmaking(pid);
        rooms.leaveLobby(pid);
    }
    for (size_t i = 0; i < allRobotIds.size(); ++i) {
        const int64_t botActorId = makeRobotActorId(allRobotIds[i]);
        rooms.dequeueMatchmaking(botActorId);
        rooms.leaveLobby(botActorId);
    }

    {
        std::lock_guard<std::mutex> wake(wakeMutex);
        ++eventWakeGen;
    }
    wakeCv.notify_all();
}

void OctiHttpServer::Impl::noteRateLimit429(const Request &req, int64_t actorId)
{
    applyBanSideEffects(banManager.recordRateLimit429(
        store, buildAbuseContext(req, actorId), store.currentEpochSec()));
}

void OctiHttpServer::Impl::sendAuthUnauthorized(const Request &req,
                                                Response &res,
                                                const char *wwwAuth,
                                                const std::string &message,
                                                bool tokenAttempted)
{
    if (tokenAttempted) {
        applyBanSideEffects(banManager.recordAuthFailure(
            store, buildAbuseContext(req, 0), store.currentEpochSec()));
    }
    if (tokenAttempted && cfg.authFailureRateLimitMax > 0 &&
        cfg.authFailureRateLimitWindowMs > 0) {
        int64_t retrySec = 1;
        std::string ip = clientRemoteIp(req);
        if (!authFailureLimiter.tryAcquire("auth401:" + ip,
                                           cfg.authFailureRateLimitMax,
                                           cfg.authFailureRateLimitWindowMs,
                                           retrySec)) {
            noteRateLimit429(req, 0);
            res.set_header("Retry-After", std::to_string(retrySec));
            sendError(res, 429, "RATE_LIMITED",
                      "too many failed authentication attempts; retry later");
            return;
        }
    }
    if (wwwAuth != nullptr && wwwAuth[0] != '\0') {
        res.set_header("WWW-Authenticate", wwwAuth);
    }
    sendError(res, 401, "UNAUTHORIZED", message);
}

bool OctiHttpServer::Impl::requireAuth(const Request &req, Response &res,
                                       Auth &auth)
{
    std::string token;
    if (!extractBearer(req, token)) {
        sendAuthUnauthorized(req, res, "Bearer",
                             "missing or malformed Authorization: Bearer header",
                             req.has_header("Authorization"));
        return false;
    }
    const int64_t nowSec = store.currentEpochSec();
    if (token.rfind("octi_bot_", 0) == 0) {
        RobotTokenValidation val = store.validateRobotToken(token);
        if (val.banned) {
            BanMatch bm;
            const BanMatch *bmp =
                (!val.owner.uuid.empty() &&
                 banManager.matchAccount(val.owner.uuid, nowSec, bm))
                    ? &bm
                    : nullptr;
            sendAccountBanned(res, bmp, nowSec);
            return false;
        }
        if (!val.valid || val.robot.revoked) {
            sendAuthUnauthorized(req, res, "Bearer error=\"invalid_token\"",
                                 "invalid or revoked robot token", true);
            return false;
        }
        BanMatch bm;
        if (banManager.matchRobot(val.robot.robotUuid, nowSec, bm) ||
            (!val.owner.uuid.empty() &&
             banManager.matchAccount(val.owner.uuid, nowSec, bm))) {
            sendAccountBanned(res, &bm, nowSec);
            return false;
        }
        if (val.owner.isAdmin) {
            banManager.noteVerifiedAdminIp(clientRemoteIp(req), nowSec);
        }
        RobotRecord robot = val.robot;
        if (req.has_header("X-Octi-Robot-Lease")) {
            std::string leaseHeader = req.get_header_value("X-Octi-Robot-Lease");
            std::lock_guard<std::mutex> lock(_robotLeaseMutex);
            auto it = _activeRobotLeases.find(robot.robotId);
            if (it != _activeRobotLeases.end() && !it->second.leaseId.empty() &&
                it->second.leaseId != leaseHeader) {
                sendError(res, 409, "ROBOT_TOKEN_SUPERSEDED",
                          "robot connection lease has been superseded by a newer connection");
                return false;
            }
        }

        const std::string &path = req.path;
        if (path.rfind("/api/v1/auth", 0) == 0) {
            sendError(res, 403, "FORBIDDEN_ROBOT",
                      "robots cannot access human authentication endpoints");
            return false;
        }
        if (path == "/api/v1/robots" || path.rfind("/api/v1/robots/", 0) == 0) {
            if (path != "/api/v1/robots/connect" &&
                path != "/api/v1/robots/disconnect" &&
                path != "/api/v1/robots/directory" &&
                path.rfind("/api/v1/robots/profile/", 0) != 0) {
                sendError(res, 403, "FORBIDDEN_ROBOT",
                          "robots cannot manage robot registrations");
                return false;
            }
        }
        if (path.rfind("/api/v1/profile", 0) == 0 && req.method == "PATCH") {
            sendError(res, 403, "FORBIDDEN_ROBOT",
                      "robots cannot update human profiles");
            return false;
        }

        auth.isRobot = true;
        auth.robotRecord = robot;
        auth.playerId = makeRobotActorId(robot.robotId);
        auth.token = token;
        auth.clientType = "robot";

        {
            std::lock_guard<std::mutex> lock(_robotLeaseMutex);
            auto it = _activeRobotLeases.find(robot.robotId);
            if (it != _activeRobotLeases.end()) {
                it->second.lastSeenMs = steadyMs();
            }
        }
        rooms.recordClientHeartbeat(auth.playerId, "robot", true);
        return true;
    }

    SessionValidation sv = store.validateSession(token, nowSec);
    if (sv.banned) {
        const std::string u = uuidOf(sv.playerId);
        BanMatch bm;
        const BanMatch *bmp =
            (!u.empty() && banManager.matchAccount(u, nowSec, bm)) ? &bm : nullptr;
        sendAccountBanned(res, bmp, nowSec);
        return false;
    }
    if (!sv.valid) {
        sendAuthUnauthorized(req, res, "Bearer error=\"invalid_token\"",
                             "invalid or expired session", true);
        return false;
    }
    const std::string u = uuidOf(sv.playerId);
    BanMatch bm;
    if (!u.empty() && banManager.matchAccount(u, nowSec, bm)) {
        sendAccountBanned(res, &bm, nowSec);
        return false;
    }
    if (store.isPlayerAdmin(sv.playerId)) {
        banManager.noteVerifiedAdminIp(clientRemoteIp(req), nowSec);
    }
    auth.isRobot = false;
    auth.playerId = sv.playerId;
    auth.token = token;
    auth.clientType = detectClientType(req);
    maybeBindClientInstallId(req, auth.playerId);
    return true;
}

/* No header: anonymous viewer (player id 0). A bad header is still a 401. */
bool OctiHttpServer::Impl::optionalAuth(const Request &req, Response &res,
                                        Auth &auth)
{
    if (!req.has_header("Authorization")) {
        auth = Auth();
        auth.clientType = detectClientType(req);
        return true;
    }
    return requireAuth(req, res, auth);
}

bool OctiHttpServer::Impl::readJsonBody(const Request &req, Response &res,
                                        Json &out, bool allowEmpty)
{
    const size_t maxBytes = routeMaxBodyBytes(req.method, req.path);
    if (req.body.size() > maxBytes) {
        sendError(res, 413, "PAYLOAD_TOO_LARGE", "request body is too large");
        return false;
    }
    if (req.body.empty()) {
        if (allowEmpty) {
            out = Json::object();
            return true;
        }
        sendError(res, 400, "MALFORMED_JSON", "a JSON request body is required");
        return false;
    }
    if (!isJsonContentType(req.get_header_value("Content-Type"))) {
        sendError(res, 415, "UNSUPPORTED_MEDIA_TYPE",
                  "Content-Type must be application/json");
        return false;
    }
    out = Json::parse(req.body, nullptr, false);
    if (out.is_discarded() || !out.is_object()) {
        sendError(res, 400, "MALFORMED_JSON",
                  "request body is not a valid JSON object");
        return false;
    }
    return true;
}

bool OctiHttpServer::Impl::pathId(const Request &req, const char *name,
                                  Response &res, std::string &out)
{
    std::unordered_map<std::string, std::string>::const_iterator it =
        req.path_params.find(name);
    if (it == req.path_params.end() || !isSafeId(it->second)) {
        sendError(res, 404, "NOT_FOUND", "no such resource");
        return false;
    }
    out = it->second;
    return true;
}

/* Verifies the SSO credential in body; on failure the response is written. */
bool OctiHttpServer::Impl::verifyCredential(const Request &req,
                                            const Json &body, Response &res,
                                            VerifiedClaims &claims)
{
    std::string provider;
    std::string idToken;
    std::string accessToken;
    std::string err;
    if (!fieldString(body, "provider", true, provider, err) ||
        !fieldString(body, "id_token", false, idToken, err) ||
        !fieldString(body, "access_token", false, accessToken, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return false;
    }
    if (!OidcVerifier::isSupportedProvider(provider)) {
        sendError(res, 400, "UNSUPPORTED_PROVIDER", "unsupported provider");
        return false;
    }
    if (!idToken.empty() && !accessToken.empty()) {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "send either id_token or access_token, not both");
        return false;
    }
    if (provider == "facebook" && accessToken.empty() && !idToken.empty()) {
        int dots = 0;
        for (size_t i = 0; i < idToken.size(); ++i) {
            if (idToken[i] == '.') {
                ++dots;
            }
        }
        if (dots < 2) {
            accessToken = idToken;
            idToken.clear();
        }
    }
    VerifyResult vr;
    if (!accessToken.empty()) {
        if (provider != "facebook") {
            sendError(res, 400, "INVALID_ARGUMENT",
                      "access_token is only accepted for facebook");
            return false;
        }
        vr = verifier.verifyFacebookAccessToken(accessToken);
    } else if (!idToken.empty()) {
        vr = verifier.verifyIdToken(provider, idToken);
    } else {
        sendError(res, 400, "INVALID_ARGUMENT", "missing field 'id_token'");
        return false;
    }
    if (!vr.ok()) {
        const int stCode = statusForVerify(vr.status);
        if (stCode == 401) {
            applyBanSideEffects(banManager.recordAuthFailure(
                store, buildAbuseContext(req, 0), store.currentEpochSec()));
        }
        sendError(res, stCode, verifyStatusName(vr.status), vr.detail);
        return false;
    }
    claims = vr.claims;
    return true;
}

/* ------------------------------------------------------------------ */
/* JSON views needing the identity cache                               */
/* ------------------------------------------------------------------ */

Json OctiHttpServer::Impl::seatJson(const SeatView &s)
{
    Json j;
    j["seat"] = s.seat;
    j["occupied"] = (s.playerId != 0);
    j["player_uuid"] = s.playerId != 0 ? Json(uuidOf(s.playerId)) : Json();
    j["handle"] = s.handle;
    j["rating"] = s.rating;
    j["connected"] = s.connected;
    j["disconnected_for_ms"] = s.disconnectedForMs;
    j["forfeit_claimable"] = s.forfeitClaimable;
    j["clock_ms"] = s.clockMs;
    j["eliminated"] = s.eliminated;
    j["is_robot"] = s.isRobot;
    if (s.isRobot) {
        j["robot_uuid"] = s.robotUuid;
        j["robot_description"] = s.robotDescription;
        j["owner_player_id"] = s.ownerPlayerId;
        j["owner_uuid"] = s.ownerUuid;
        j["owner_handle"] = s.ownerHandle;
        j["avatar_url"] = s.avatarUrl;
        j["robot_games_played"] = s.robotGamesPlayed;
        j["robot_wins"] = s.robotWins;
        j["robot_losses"] = s.robotLosses;
        j["robot_draws"] = s.robotDraws;
    }
    return j;
}

Json OctiHttpServer::Impl::roomViewJson(const RoomView &v)
{
    Json j = configJson(v.config);
    j["ok"] = true;
    j["live"] = true;
    j["room_id"] = v.roomId;
    j["phase"] = toString(v.phase);
    j["paused"] = v.paused;
    j["has_passcode"] = v.hasPasscode;
    j["host_uuid"] = uuidOf(v.hostPlayerId);
    j["viewer_seat"] = v.viewerSeat;
    Json seats = Json::array();
    for (size_t i = 0; i < v.seats.size(); ++i) {
        seats.push_back(seatJson(v.seats[i]));
    }
    j["seats"] = seats;
    Json obs = Json::array();
    for (size_t i = 0; i < v.observerIds.size(); ++i) {
        obs.push_back(uuidOf(v.observerIds[i]));
    }
    j["observers"] = obs;
    j["observer_count"] = static_cast<int>(v.observerIds.size());
    j["observer_handles"] = v.observerHandles;
    j["fen"] = v.fen;
    j["turn_seat"] = v.turnSeat;
    j["ply_count"] = v.plyCount;
    j["no_progress_plies"] = v.noProgressPlies;
    j["moves"] = v.moves;
    j["result"] = v.result;
    j["termination"] = v.termination;
    j["winner_seat"] = v.winnerSeat;
    Json props = Json::array();
    for (size_t i = 0; i < v.proposals.size(); ++i) {
        const ProposalView &p = v.proposals[i];
        Json pj;
        pj["type"] = toString(p.type);
        pj["proposer_uuid"] = uuidOf(p.proposerPlayerId);
        Json acc = Json::array();
        for (size_t k = 0; k < p.acceptedBy.size(); ++k) {
            acc.push_back(uuidOf(p.acceptedBy[k]));
        }
        pj["accepted_by"] = acc;
        Json wait = Json::array();
        for (size_t k = 0; k < p.awaiting.size(); ++k) {
            wait.push_back(uuidOf(p.awaiting[k]));
        }
        pj["awaiting"] = wait;
        pj["expires_at_ms"] = p.expiresAtMs;
        props.push_back(pj);
    }
    j["proposals"] = props;
    j["seq"] = v.latestSeq;
    j["archived"] = v.archived;
    j["rematch_room_id"] = v.rematchRoomId;
    j["created_at_ms"] = v.createdAtMs;
    j["last_move_at_ms"] = v.lastMoveAtMs;
    j["idle_ms"] = v.idleMs;
    j["ttl_remaining_ms"] = v.ttlRemainingMs;
    return j;
}

Json OctiHttpServer::Impl::roomEventJson(const RoomEvent &e)
{
    Json j;
    j["seq"] = e.seq;
    j["type"] = toString(e.type);
    j["at_ms"] = e.atMs;
    j["is_robot"] = e.isRobot;
    if (e.isRobot) {
        j["robot_uuid"] = e.robotUuid;
        j["robot_description"] = e.robotDescription;
        j["owner_player_id"] = e.ownerPlayerId;
        j["owner_uuid"] = e.ownerUuid;
        j["owner_handle"] = e.ownerHandle;
        j["avatar_url"] = e.avatarUrl;
    }
    if (e.playerId != 0) {
        j["player_uuid"] = uuidOf(e.playerId);
        j["handle"] = e.handle;
    }
    if (e.targetPlayerId != 0) {
        j["target_uuid"] = uuidOf(e.targetPlayerId);
        if (!e.targetHandle.empty()) {
            j["target_handle"] = e.targetHandle;
        }
    }
    j["seat"] = e.seat;
    switch (e.type) {
    case RoomEventType::MOVE:
        j["ply"] = e.ply;
        j["move"] = e.text;
        j["clock_ms"] = e.clockMs;
        j["elapsed_ms"] = e.elapsedMs;
        break;
    case RoomEventType::CHAT:
        j["scope"] = toString(e.scope);
        j["text"] = e.text;
        break;
    case RoomEventType::OBSERVER_BANNED:
        j["reason"] = e.text;
        break;
    case RoomEventType::USER_MUTED:
        j["mute_chat"] = e.detail.find("mute_chat=1") != std::string::npos;
        break;
    case RoomEventType::POLICY_UPDATED: {
        const std::string op = "observer_policy=";
        if (e.detail.compare(0, op.size(), op) == 0) {
            size_t sep = e.detail.find(';');
            j["observer_policy"] = sep != std::string::npos
                                       ? e.detail.substr(op.size(), sep - op.size())
                                       : e.detail.substr(op.size());
        }
        break;
    }
    case RoomEventType::PROPOSAL_OFFERED:
    case RoomEventType::PROPOSAL_ACCEPTED:
    case RoomEventType::PROPOSAL_DECLINED:
    case RoomEventType::PROPOSAL_EXPIRED:
        j["proposal"] = e.detail;
        if (!e.text.empty()) {
            j["note"] = e.text;
        }
        break;
    case RoomEventType::UNDO_APPLIED:
        j["ply"] = e.ply;
        j["move"] = e.text;
        break;
    case RoomEventType::REMATCH_CREATED:
        j["room_id"] = e.text;
        break;
    case RoomEventType::GAME_OVER:
        j["termination"] = e.detail;
        j["result"] = e.text;
        j["winner_seat"] = e.seat;
        break;
    default:
        if (!e.detail.empty()) {
            j["detail"] = e.detail;
        }
        if (!e.text.empty()) {
            j["text"] = e.text;
        }
        break;
    }
    return j;
}

void OctiHttpServer::Impl::populateHumanEntitySummaryJson(
    Json &target, const PlayerProfile &prof)
{
    target["is_robot"] = false;
    target["player_uuid"] = prof.uuid;
    target["uuid"] = prof.uuid;
    target["handle"] = prof.handle;
    target["display_name"] = prof.displayName;
    target["avatar_url"] = prof.avatarUrl;
    target["bio"] = prof.bio;
    target["location"] = prof.location;
    target["elo_2p_3b"] = prof.elo2p3b;
    target["elo_2p_1b"] = prof.elo2p1b;
    target["elo_ofk"] = prof.eloOfk;
    target["games_2p_3b"] = prof.games2p3b;
    target["games_2p_1b"] = prof.games2p1b;
    target["games_ofk"] = prof.gamesOfk;
    target["games_played"] = prof.gamesPlayed;
    target["wins"] = prof.wins;
    target["losses"] = prof.losses;
    target["draws"] = prof.draws;
    target["arena_elo_2p_3b"] = prof.arenaElo2p3b;
    target["arena_elo_2p_1b"] = prof.arenaElo2p1b;
    target["arena_elo_ofk"] = prof.arenaEloOfk;
    target["arena_games_2p_3b"] = prof.arenaGames2p3b;
    target["arena_games_2p_1b"] = prof.arenaGames2p1b;
    target["arena_games_ofk"] = prof.arenaGamesOfk;
    target["arena_games_played"] = prof.arenaGamesPlayed;
    target["arena_wins"] = prof.arenaWins;
    target["arena_losses"] = prof.arenaLosses;
    target["arena_draws"] = prof.arenaDraws;
    target["created_at"] = prof.createdAt;
}

void OctiHttpServer::Impl::populateRobotEntitySummaryJson(
    Json &target, const RobotRecord &rRec,
    const std::string &fallbackAvatarUrl)
{
    target["is_robot"] = true;
    target["robot_uuid"] = rRec.robotUuid;
    target["uuid"] = rRec.robotUuid;
    target["handle"] = rRec.robotName;
    target["robot_name"] = rRec.robotName;
    target["display_name"] = rRec.robotName;
    target["avatar_url"] =
        !rRec.ownerAvatarUrl.empty() ? rRec.ownerAvatarUrl : fallbackAvatarUrl;
    target["robot_url"] = rRec.robotUrl;
    target["robot_description"] = rRec.description;
    target["description"] = rRec.description;
    target["owner_player_id"] = rRec.ownerPlayerId;
    target["owner_uuid"] = rRec.ownerUuid;
    target["owner_handle"] = rRec.ownerHandle;
    target["owner_avatar_url"] = rRec.ownerAvatarUrl;
    target["elo_2p_3b"] = rRec.elo2p3b;
    target["elo_2p_1b"] = rRec.elo2p1b;
    target["elo_ofk"] = rRec.eloOfk;
    target["games_2p_3b"] = rRec.games2p3b;
    target["games_2p_1b"] = rRec.games2p1b;
    target["games_ofk"] = rRec.gamesOfk;
    target["robot_games_played"] = rRec.gamesPlayed;
    target["games_played"] = rRec.gamesPlayed;
    target["robot_wins"] = rRec.wins;
    target["wins"] = rRec.wins;
    target["robot_losses"] = rRec.losses;
    target["losses"] = rRec.losses;
    target["robot_draws"] = rRec.draws;
    target["draws"] = rRec.draws;
    target["created_at"] = rRec.createdAt;
}

bool OctiHttpServer::Impl::populateActorEntitySummaryJson(
    int64_t actorId, const std::string &fallbackHandle,
    const std::string &fallbackRobotUuid,
    const std::string &fallbackAvatarUrl, Json &target)
{
    (void)fallbackHandle;
    if (isRobotActorId(actorId) || !fallbackRobotUuid.empty()) {
        int64_t robotId =
            isRobotActorId(actorId) ? robotIdFromActorId(actorId) : 0;
        RobotRecord rRec;
        bool found = (robotId > 0 && store.getRobotById(robotId, rRec)) ||
                     (!fallbackRobotUuid.empty() &&
                      store.getRobotByUuid(fallbackRobotUuid, rRec));
        if (found) {
            populateRobotEntitySummaryJson(target, rRec, fallbackAvatarUrl);
            return true;
        }
        return false;
    }
    if (actorId > 0) {
        PlayerProfile prof;
        if (store.getProfileById(actorId, prof)) {
            populateHumanEntitySummaryJson(target, prof);
            return true;
        }
    }
    return false;
}

Json OctiHttpServer::Impl::tableJson(const TableSummary &t)
{
    Json j = configJson(t.config);
    j["room_id"] = t.roomId;
    j["phase"] = toString(t.phase);
    j["has_passcode"] = t.hasPasscode;
    j["host_uuid"] = t.hostPlayerId != 0 ? uuidOf(t.hostPlayerId) : "";
    j["host_handle"] = t.hostHandle;
    if (t.hostPlayerId != 0) {
        Json hostDetail = Json::object();
        if (!populateActorEntitySummaryJson(t.hostPlayerId, t.hostHandle, "",
                                            "", hostDetail)) {
            const bool hostIsBot = isRobotActorId(t.hostPlayerId);
            hostDetail["is_robot"] = hostIsBot;
            hostDetail["handle"] = t.hostHandle;
            hostDetail["display_name"] = t.hostHandle;
            hostDetail["avatar_url"] = "";
            if (hostIsBot) {
                hostDetail["robot_uuid"] = j["host_uuid"];
                hostDetail["robot_name"] = t.hostHandle;
                hostDetail["robot_url"] = "";
                hostDetail["robot_description"] = "";
                hostDetail["owner_uuid"] = "";
                hostDetail["owner_handle"] = "";
                hostDetail["robot_games_played"] = 0;
                hostDetail["robot_wins"] = 0;
                hostDetail["robot_losses"] = 0;
                hostDetail["robot_draws"] = 0;
            } else {
                hostDetail["player_uuid"] = j["host_uuid"];
                hostDetail["bio"] = "";
                hostDetail["location"] = "";
                hostDetail["arena_elo_2p_3b"] = 1200;
                hostDetail["arena_elo_2p_1b"] = 1200;
                hostDetail["arena_elo_ofk"] = 1200;
                hostDetail["arena_games_2p_3b"] = 0;
                hostDetail["arena_games_2p_1b"] = 0;
                hostDetail["arena_games_ofk"] = 0;
                hostDetail["arena_games_played"] = 0;
                hostDetail["arena_wins"] = 0;
                hostDetail["arena_losses"] = 0;
                hostDetail["arena_draws"] = 0;
            }
            hostDetail["elo_2p_3b"] = 1200;
            hostDetail["elo_2p_1b"] = 1200;
            hostDetail["elo_ofk"] = 1200;
            hostDetail["games_2p_3b"] = 0;
            hostDetail["games_2p_1b"] = 0;
            hostDetail["games_ofk"] = 0;
            hostDetail["games_played"] = 0;
            hostDetail["wins"] = 0;
            hostDetail["losses"] = 0;
            hostDetail["draws"] = 0;
            hostDetail["created_at"] = 0;
        }
        j["host_detail"] = hostDetail;
    } else {
        j["host_detail"] = nullptr;
    }
    j["created_at_ms"] = t.createdAtMs;
    j["seats_total"] = t.seatsTotal;
    j["seats_filled"] = t.seatsFilled;
    Json seatsArr = Json::array();
    for (size_t i = 0; i < t.seats.size(); ++i) {
        const TableSeatSummary &s = t.seats[i];
        Json sj;
        sj["seat"] = s.seat;
        sj["occupied"] = (s.playerId != 0);
        sj["player_uuid"] = s.playerId != 0 ? uuidOf(s.playerId) : "";
        sj["handle"] = s.handle;
        sj["rating"] = s.rating;
        sj["connected"] = s.connected;
        sj["is_robot"] = s.isRobot;
        if (s.isRobot) {
            sj["robot_uuid"] = s.robotUuid;
            sj["robot_description"] = s.robotDescription;
            sj["owner_player_id"] = s.ownerPlayerId;
            sj["owner_uuid"] = s.ownerUuid;
            sj["owner_handle"] = s.ownerHandle;
            sj["avatar_url"] = s.avatarUrl;
            sj["robot_games_played"] = s.robotGamesPlayed;
            sj["robot_wins"] = s.robotWins;
            sj["robot_losses"] = s.robotLosses;
            sj["robot_draws"] = s.robotDraws;
        }
        seatsArr.push_back(sj);
    }
    if (seatsArr.empty() && t.seatsTotal > 0) {
        for (int i = 1; i <= t.seatsTotal; ++i) {
            Json sj;
            sj["seat"] = i;
            sj["occupied"] = false;
            sj["player_uuid"] = "";
            sj["handle"] = "";
            sj["rating"] = 1500;
            sj["connected"] = false;
            sj["is_robot"] = false;
            seatsArr.push_back(sj);
        }
    }
    j["seats"] = seatsArr;
    j["players"] = t.playerHandles;
    j["observer_count"] = t.observerCount;
    j["observer_handles"] = t.observerHandles;
    j["last_move_at_ms"] = t.lastMoveAtMs;
    j["idle_ms"] = t.idleMs;
    j["ttl_remaining_ms"] = t.ttlRemainingMs;
    return j;
}

Json OctiHttpServer::Impl::lobbyEventJson(const LobbyEvent &e)
{
    Json j;
    j["seq"] = e.seq;
    j["type"] = toString(e.type);
    int64_t tsMs = (e.createdAtMs > 0) ? e.createdAtMs : e.atMs;
    j["at_ms"] = tsMs;
    j["created_at_ms"] = tsMs;
    j["is_robot"] = e.isRobot;
    if (e.isRobot) {
        j["robot_uuid"] = e.robotUuid;
        j["robot_description"] = e.robotDescription;
        j["owner_player_id"] = e.ownerPlayerId;
        j["owner_uuid"] = e.ownerUuid;
        j["owner_handle"] = e.ownerHandle;
        j["avatar_url"] = e.avatarUrl;
    }
    if (e.playerId != 0) {
        j["player_uuid"] = uuidOf(e.playerId);
        j["handle"] = e.handle;
    }
    switch (e.type) {
    case LobbyEventType::LOBBY_CHAT:
        j["text"] = e.text;
        break;
    case LobbyEventType::PRESENCE:
        j["status"] = toString(e.status);
        j["room_id"] = e.roomId;
        break;
    default:
        j["room_id"] = e.roomId;
        break;
    }
    return j;
}

Json OctiHttpServer::Impl::gameSummaryJson(const GameRecord &g)
{
    Json j;
    j["game_id"] = g.gameId;
    j["mode"] = g.gameMode;
    j["bases"] = g.gameBases;
    j["super_prongs"] = g.superProngs;
    j["edgeless"] = g.edgeless;
    j["variant"] = octiVariantCodeFromModeStr(g.gameMode, g.gameBases,
                                              g.superProngs, g.edgeless);
    j["time_control"] = g.timeControl;
    Json players = Json::array();
    const int64_t ids[4] = {g.p1PlayerId, g.p2PlayerId, g.p3PlayerId,
                            g.p4PlayerId};
    const int64_t robotIds[4] = {g.p1RobotId, g.p2RobotId, g.p3RobotId,
                                 g.p4RobotId};
    const std::string *handles[4] = {&g.p1Handle, &g.p2Handle, &g.p3Handle,
                                     &g.p4Handle};
    for (int i = 0; i < 4; ++i) {
        if (handles[i]->empty()) {
            continue;
        }
        Json p;
        p["seat"] = i + 1;
        p["handle"] = *handles[i];
        if (robotIds[i] > 0) {
            p["is_robot"] = true;
            RobotRecord rRec;
            if (store.getRobotById(robotIds[i], rRec)) {
                p["handle"] = rRec.robotName;
                if (rRec.gamesPlayed > 0) {
                    p["robot_uuid"] = rRec.robotUuid;
                }
            }
        } else if (ids[i] != 0) {
            std::string u = uuidOf(ids[i]);
            if (!u.empty()) {
                p["player_uuid"] = u;
            }
        }
        players.push_back(p);
    }
    j["players"] = players;
    j["result"] = g.result;
    j["winner_seat"] = g.winnerSeat;
    j["termination"] = g.termination;
    j["ply_count"] = g.plyCount;
    j["started_at"] = g.startedAt;
    j["ended_at"] = g.endedAt;
    j["source"] = g.source;
    j["uploaded_at"] = g.uploadedAt;
    j["winner"] = g.winner;
    j["status"] = g.status;
    j["ai_difficulty"] = g.aiDifficulty;
    j["app_version"] = g.appVersion;
    j["platform"] = g.platform;
    j["rated"] = (g.rated != 0);
    j["elo_pool"] = g.eloPool;
    return j;
}

/* ------------------------------------------------------------------ */
/* Event streaming (JSON poll and SSE)                                 */
/* ------------------------------------------------------------------ */

bool OctiHttpServer::Impl::wantsStream(const Request &req) const
{
    if (req.has_param("stream")) {
        std::string v = lower(req.get_param_value("stream"));
        if (v == "1" || v == "true") {
            return true;
        }
    }
    return lower(req.get_header_value("Accept")).find("text/event-stream") !=
           std::string::npos;
}

bool OctiHttpServer::Impl::streamStep(StreamState &st,
                                      httplib::DataSink &sink)
{
    const int64_t now = steadyMs();
    if (stopping.load()) {
        putText(sink, "event: reconnect\ndata: {}\n\n");
        sink.done();
        return true;
    }
    {
        const int64_t nowSec = store.currentEpochSec();
        BanMatch bm;
        if (!st.remoteIp.empty() &&
            banManager.matchIp(st.remoteIp, nowSec, bm)) {
            putText(sink, "event: closed\ndata: {\"error\":\"IP_BANNED\"}\n\n");
            sink.done();
            return true;
        }
        if ((!st.robotUuid.empty() &&
             banManager.matchRobot(st.robotUuid, nowSec, bm)) ||
            (!st.ownerUuid.empty() &&
             banManager.matchAccount(st.ownerUuid, nowSec, bm)) ||
            (!st.playerUuid.empty() &&
             banManager.matchAccount(st.playerUuid, nowSec, bm))) {
            putText(sink, "event: closed\ndata: {\"error\":\"ACCOUNT_BANNED\"}\n\n");
            sink.done();
            return true;
        }
    }
    if (st.first) {
        st.first = false;
        st.startedMs = now;
        st.lastWriteMs = now;
        st.lastAuthMs = now;
        if (!putText(sink, "retry: 3000\n\n")) {
            return false;
        }
    }
    const int64_t maxDurationMs =
        (st.playerId == 0 && cfg.sseAnonMaxDurationMs > 0)
            ? std::min(cfg.sseMaxDurationMs, cfg.sseAnonMaxDurationMs)
            : cfg.sseMaxDurationMs;
    if (now - st.startedMs >= maxDurationMs) {
        putText(sink, "event: reconnect\ndata: {}\n\n");
        sink.done();
        return true;
    }
    if (st.playerId != 0 && now - st.lastAuthMs >= cfg.sseSessionRecheckMs) {
        st.lastAuthMs = now;
        if (st.isRobot) {
            RobotTokenValidation val = store.validateRobotToken(st.token);
            if (val.banned) {
                putText(sink, "event: closed\ndata: {\"error\":\"ACCOUNT_BANNED\"}\n\n");
                sink.done();
                return true;
            }
            if (!val.valid || val.robot.revoked) {
                putText(sink, "event: closed\ndata: {\"error\":\"UNAUTHORIZED\"}\n\n");
                sink.done();
                return true;
            }
            if (!st.leaseId.empty()) {
                std::lock_guard<std::mutex> lock(_robotLeaseMutex);
                auto it = _activeRobotLeases.find(st.robotId);
                if (it != _activeRobotLeases.end() && !it->second.leaseId.empty() &&
                    it->second.leaseId != st.leaseId) {
                    putText(sink, "event: closed\ndata: {\"error\":\"ROBOT_TOKEN_SUPERSEDED\"}\n\n");
                    sink.done();
                    return true;
                }
                if (it != _activeRobotLeases.end()) {
                    it->second.lastSeenMs = steadyMs();
                }
            }
        } else {
            SessionValidation sv = store.validateSession(
                st.token, store.currentEpochSec());
            if (sv.banned) {
                putText(sink, "event: closed\ndata: {\"error\":\"ACCOUNT_BANNED\"}\n\n");
                sink.done();
                return true;
            }
            if (!sv.valid || sv.playerId != st.playerId) {
                putText(sink, "event: closed\ndata: {\"error\":\"UNAUTHORIZED\"}\n\n");
                sink.done();
                return true;
            }
        }
        rooms.recordClientHeartbeat(st.playerId, st.clientType, false);
    }

    PollBatch batch;
    int status = st.fetch(st.cursor, cfg.sseBatchLimit, batch);
    if (status != 200) {
        Json j;
        j["error"] = batch.errorCode;
        j["message"] = batch.message;
        putText(sink, "event: closed\ndata: " + dumpJson(j) + "\n\n");
        sink.done();
        return true;
    }
    if (batch.gap && !st.gapSent) {
        st.gapSent = true;
        if (!putText(sink, "event: gap\ndata: {}\n\n")) {
            return false;
        }
    }
    bool wrote = false;
    for (size_t i = 0; i < batch.frames.size(); ++i) {
        if (!putText(sink, sseFrame(batch.frames[i]))) {
            return false;
        }
        wrote = true;
    }
    st.cursor = std::max(st.cursor, batch.nextSeq);
    if (wrote) {
        st.lastWriteMs = steadyMs();
        return true;
    }
    waitWake(cfg.ssePollMs);
    if (stopping.load()) {
        putText(sink, "event: reconnect\ndata: {}\n\n");
        sink.done();
        return true;
    }
    const int64_t after = steadyMs();
    if (after - st.lastWriteMs >= cfg.sseHeartbeatMs) {
        if (!putText(sink, ": hb\n\n")) {
            return false;
        }
        st.lastWriteMs = after;
    }
    return true;
}

bool OctiHttpServer::Impl::tryAcquireSseSlot(
    const std::string &ip, int64_t playerId,
    std::shared_ptr<SseStreamSlotGuard> &outSlot,
    int &outStatus, std::string &outCode, std::string &outMsg)
{
    std::lock_guard<std::mutex> lock(sseQuotaMutex);
    if (sseActive.load() >= cfg.maxSseStreams) {
        outStatus = 503;
        outCode = "TOO_MANY_STREAMS";
        outMsg = "too many open event streams; use polling";
        return false;
    }
    const std::string ipKey = ip.empty()
        ? std::string()
        : ((playerId == 0 ? "anon:" : "auth:") + ip);
    int perIpLimit = 0;
    if (playerId == 0) {
        if (cfg.maxSseAnonPerIp > 0 && cfg.maxSsePerIp > 0) {
            perIpLimit = std::min(cfg.maxSseAnonPerIp, cfg.maxSsePerIp);
        } else {
            perIpLimit = (cfg.maxSseAnonPerIp > 0) ? cfg.maxSseAnonPerIp
                                                   : cfg.maxSsePerIp;
        }
    } else {
        perIpLimit = cfg.maxSsePerIp;
    }
    if (perIpLimit > 0 && !ipKey.empty()) {
        auto ipIt = sseByIp.find(ipKey);
        int curIp = (ipIt != sseByIp.end()) ? ipIt->second : 0;
        if (curIp >= perIpLimit) {
            outStatus = 429;
            outCode = "TOO_MANY_STREAMS";
            outMsg = "too many open event streams from this IP; use polling";
            return false;
        }
    }
    if (cfg.maxSsePerPlayer > 0 && playerId != 0) {
        auto plIt = sseByPlayer.find(playerId);
        int curPl = (plIt != sseByPlayer.end()) ? plIt->second : 0;
        if (curPl >= cfg.maxSsePerPlayer) {
            outStatus = 429;
            outCode = "TOO_MANY_STREAMS";
            outMsg = "too many open event streams for this player; use polling";
            return false;
        }
    }

    ++sseActive;
    if (!ipKey.empty()) {
        ++sseByIp[ipKey];
    }
    if (playerId != 0) {
        ++sseByPlayer[playerId];
    }
    outSlot = std::make_shared<SseStreamSlotGuard>(
        sseActive, sseQuotaMutex, sseByIp, sseByPlayer, ipKey, playerId);
    return true;
}

void OctiHttpServer::Impl::respondEvents(const Request &req, Response &res,
                                         const Auth &auth, PollFn fetch)
{
    int64_t since = 0;
    if (req.has_param("since_seq") &&
        !parseUnsignedClamped(req.get_param_value("since_seq"), INT64_MAX / 2,
                              since)) {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "since_seq must be a non-negative integer");
        return;
    }
    int64_t limit = 100;
    if (req.has_param("limit") &&
        (!parseUnsignedClamped(req.get_param_value("limit"),
                               static_cast<int64_t>(kMaxQueryLimit), limit) ||
         limit < 1)) {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "limit must be a positive integer");
        return;
    }
    const bool stream = wantsStream(req);
    if (stream && req.has_header("Last-Event-ID")) {
        int64_t last = 0;
        if (parseUnsignedClamped(req.get_header_value("Last-Event-ID"),
                                 INT64_MAX / 2, last)) {
            since = std::max(since, last);
        }
    }

    if (!stream) {
        int64_t waitMs = 0;
        if (req.has_param("wait_ms")) {
            if (!parseUnsignedClamped(req.get_param_value("wait_ms"),
                                      INT64_MAX / 2, waitMs)) {
                sendError(res, 400, "INVALID_ARGUMENT",
                          "wait_ms must be a non-negative integer");
                return;
            }
            int64_t maxWait = std::max<int64_t>(0, cfg.maxEventPollWaitMs);
            if (waitMs > maxWait) {
                waitMs = maxWait;
            }
        }

        uint64_t seenGen = currentEventWakeGen();
        PollBatch batch;
        int status = fetch(since, static_cast<size_t>(limit), batch);
        if (status != 200) {
            sendError(res, status, batch.errorCode, batch.message);
            return;
        }

        if (waitMs > 0 && batch.frames.empty() && !batch.gap &&
            !stopping.load()) {
            /* Long-polling ("use polling") is bounded only by the global worker
             * stream ceiling so rejected SSE clients can always fall back cleanly. */
            std::shared_ptr<SseStreamSlotGuard> slotGuard;
            int slotStatus = 503;
            std::string slotCode;
            std::string slotMsg;
            if (!tryAcquireSseSlot(std::string(), 0, slotGuard,
                                   slotStatus, slotCode, slotMsg)) {
                res.set_header("Retry-After", "5");
                sendError(res, slotStatus, slotCode, slotMsg);
                return;
            }
            const int64_t deadlineMs = steadyMs() + waitMs;
            while (!stopping.load()) {
                int64_t remMs = deadlineMs - steadyMs();
                if (remMs <= 0) {
                    break;
                }
                waitEventOrStop(seenGen, remMs);
                if (stopping.load()) {
                    break;
                }
                batch = PollBatch{};
                status = fetch(since, static_cast<size_t>(limit), batch);
                if (status != 200) {
                    sendError(res, status, batch.errorCode, batch.message);
                    return;
                }
                if (!batch.frames.empty() || batch.gap) {
                    break;
                }
            }
        }

        const std::string evEtag =
            "\"ev-" + std::to_string(batch.latestSeq) + "\"";
        res.set_header("ETag", evEtag);
        if (batch.frames.empty() && !batch.gap &&
            ifNoneMatchMatches(req, evEtag)) {
            res.set_header("Cache-Control", "no-store");
            res.status = 304;
            return;
        }

        Json j;
        Json events = Json::array();
        for (size_t i = 0; i < batch.frames.size(); ++i) {
            events.push_back(Json::parse(batch.frames[i].json));
        }
        j["events"] = events;
        j["latest_seq"] = batch.latestSeq;
        j["next_seq"] = batch.nextSeq;
        j["gap"] = batch.gap;
        j["has_more"] = batch.hasMore;
        sendJson(res, 200, j);
        return;
    }

    /* Validate access once before committing to a stream. */
    {
        PollBatch probe;
        int status = fetch(since, 1, probe);
        if (status != 200) {
            sendError(res, status, probe.errorCode, probe.message);
            return;
        }
    }
    const std::string remoteIp = clientRemoteIp(req);
    std::shared_ptr<SseStreamSlotGuard> slotGuard;
    int slotStatus = 503;
    std::string slotCode;
    std::string slotMsg;
    if (!tryAcquireSseSlot(remoteIp, auth.playerId, slotGuard,
                           slotStatus, slotCode, slotMsg)) {
        if (slotStatus == 429 && auth.playerId == 0) {
            noteRateLimit429(req, 0);
        }
        res.set_header("Retry-After", "5");
        sendError(res, slotStatus, slotCode, slotMsg);
        return;
    }
    std::shared_ptr<StreamState> st(new StreamState());
    st->slot = slotGuard;
    st->fetch = fetch;
    st->cursor = since;
    st->token = auth.token;
    st->playerId = auth.playerId;
    st->remoteIp = remoteIp;
    st->clientType = auth.clientType;
    if (auth.isRobot) {
        st->isRobot = true;
        st->robotId = auth.robotRecord.robotId;
        st->robotUuid = auth.robotRecord.robotUuid;
        st->ownerUuid = auth.robotRecord.ownerUuid;
        if (req.has_header("X-Octi-Robot-Lease")) {
            st->leaseId = req.get_header_value("X-Octi-Robot-Lease");
        } else {
            std::lock_guard<std::mutex> lock(_robotLeaseMutex);
            auto it = _activeRobotLeases.find(st->robotId);
            if (it != _activeRobotLeases.end()) {
                st->leaseId = it->second.leaseId;
            }
        }
        st->robotStreamGuard = std::make_shared<RobotStreamGuard>(
            _robotLeaseMutex, _activeRobotLeases, st->robotId);
    } else if (auth.playerId > 0) {
        st->playerUuid = uuidOf(auth.playerId);
    }
    if (auth.playerId != 0) {
        rooms.recordClientHeartbeat(auth.playerId, auth.clientType, false);
    }

    res.status = 200;
    res.set_header("Cache-Control", "no-store");
    res.set_header("X-Accel-Buffering", "no");
    res.set_chunked_content_provider(
        "text/event-stream", [this, st](size_t, httplib::DataSink &sink) {
            return streamStep(*st, sink);
        });
}

/* ------------------------------------------------------------------ */
/* Handlers: service, auth, profile                                    */
/* ------------------------------------------------------------------ */

void OctiHttpServer::Impl::handleRobotsTxt(const Request &, Response &res)
{
    res.status = 200;
    res.set_header("Cache-Control", "public, max-age=86400");
    if (cfg.disallowRobotsTxt) {
        res.set_content("User-agent: *\nDisallow: /\n",
                        "text/plain; charset=utf-8");
    } else {
        res.set_content("User-agent: *\nAllow: /\nDisallow: /api/\n",
                        "text/plain; charset=utf-8");
    }
}

void OctiHttpServer::Impl::handleHealth(const Request &, Response &res)
{
    Json j;
    j["status"] = "ok";
    j["version"] = OctiCore::versionString();
    j["protocol_version"] = cfg.protocolVersion;
    j["min_protocol_version"] = cfg.minProtocolVersion;
    j["uptime_sec"] = std::max<int64_t>(0, epochSec() - startedAtSec);
    j["active_tables"] = static_cast<int64_t>(rooms.roomCount());
    j["online_players"] = static_cast<int64_t>(rooms.onlinePlayerCount());
    sendJson(res, 200, j);
}

void OctiHttpServer::Impl::handleAvatarGet(const Request &req, Response &res)
{
    auto it = req.path_params.find("file");
    if (it == req.path_params.end() || it->second.empty()) {
        sendError(res, 404, "NOT_FOUND", "Avatar not found");
        return;
    }
    std::string bytes;
    std::string mime;
    if (!store.readAvatarFile(it->second, bytes, mime)) {
        sendError(res, 404, "NOT_FOUND", "Avatar not found");
        return;
    }
    res.status = 200;
    res.set_header("Cache-Control", "public, max-age=86400");
    res.set_header("Content-Security-Policy", "sandbox; default-src 'none'");
    res.set_header("X-Content-Type-Options", "nosniff");
    const std::string lowerFile = lower(it->second);
    if (mime == "image/svg+xml" ||
        (lowerFile.size() >= 4 &&
         lowerFile.substr(lowerFile.size() - 4) == ".svg")) {
        res.set_header("Content-Disposition", "attachment");
    }
    res.set_content(bytes, mime.c_str());
}

void OctiHttpServer::Impl::handleSoundAssetGet(const Request &req,
                                               Response &res)
{
    auto it = req.path_params.find("file");
    if (it == req.path_params.end() || it->second.empty()) {
        sendError(res, 404, "NOT_FOUND", "sound asset not found");
        return;
    }
    SoundCue cue = SoundCue::POD_PLACE;
    if (!soundCueFromAssetFileName(it->second, cue)) {
        sendError(res, 404, "NOT_FOUND", "sound asset not found");
        return;
    }
    std::size_t wavSize = 0;
    const std::uint8_t *wavBytes = soundCueWavBytes(cue, wavSize);
    if (wavBytes == nullptr || wavSize == 0) {
        sendError(res, 404, "NOT_FOUND", "sound asset not found");
        return;
    }
    res.status = 200;
    res.set_header("Cache-Control", "public, max-age=86400");
    res.set_content(reinterpret_cast<const char *>(wavBytes), wavSize,
                    "audio/wav");
}

void OctiHttpServer::Impl::handleRulebookAssetGet(const Request &req,
                                                  Response &res)
{
    auto it = req.path_params.find("file");
    if (it == req.path_params.end() || it->second.empty()) {
        sendError(res, 404, "NOT_FOUND", "rulebook asset not found");
        return;
    }
    std::size_t pngSize = 0u;
    const std::uint8_t *pngBytes = rulebookImageBytes(it->second, pngSize);
    if (pngBytes == nullptr || pngSize == 0u) {
        sendError(res, 404, "NOT_FOUND", "rulebook asset not found");
        return;
    }
    res.status = 200;
    res.set_header("Cache-Control", "public, max-age=86400");
    res.set_content(reinterpret_cast<const char *>(pngBytes), pngSize,
                    "image/png");
}

void OctiHttpServer::Impl::handlePrivacy(const Request &, Response &res)
{
    res.status = 200;
    res.set_header("Content-Type", "text/html; charset=utf-8");
    res.set_header("Cache-Control", "public, max-age=86400");
    res.set_content(
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "<meta charset=\"utf-8\">\n"
        "<title>OCTI - Privacy &amp; Data Deletion Policy</title>\n"
        "<style>body{font-family:sans-serif;max-width:800px;margin:40px auto;line-height:1.6;padding:0 20px;color:#333;}\n"
        "h1,h2{color:#111;}code{background:#f4f4f4;padding:2px 6px;border-radius:4px;}</style>\n"
        "</head>\n"
        "<body>\n"
        "<h1>OCTI Privacy &amp; Data Deletion Policy</h1>\n"
        "<p>Last updated: 2026</p>\n"
        "<h2>1. Data Collection &amp; Use</h2>\n"
        "<p>OCTI collects minimal public profile information through single sign-on (Google, Apple, Microsoft, Facebook, LINE), "
        "including your display name, email, avatar URL, and provider-specific subject identifier. Passwords are never collected or stored.</p>\n"
        "<h2>2. Game Telemetry &amp; Analytics</h2>\n"
        "<p>Online and uploaded game transcripts (board moves, game results, ELO ratings) are archived for leaderboards, replays, and AI research under CC-BY-4.0.</p>\n"
        "<h2>3. Data Deletion &amp; Account Unlinking</h2>\n"
        "<p>Players can unlink single sign-on identities via the in-app Account Settings tab or delete client-uploaded transcripts by calling the telemetry erasure endpoint with their install ID. "
        "To request complete deletion of your account and associated profile data, contact samurai@selfso.com.</p>\n"
        "</body>\n"
        "</html>\n",
        "text/html; charset=utf-8");
}

void OctiHttpServer::Impl::handleDataDeletion(const Request &req, Response &res)
{
    const size_t maxBytes = routeMaxBodyBytes(req.method, req.path);
    if (req.body.size() > maxBytes) {
        sendError(res, 413, "PAYLOAD_TOO_LARGE", "request body is too large");
        return;
    }
    if (!allowMutationAttempt(req, res, 0)) {
        return;
    }
    std::string code = AccountStore::sha256Hex(std::to_string(epochSec()) + ":" + req.body).substr(0, 12);
    Json j;
    j["url"] = "https://octi.selfso.com/privacy/?confirmation_code=" + code;
    j["confirmation_code"] = code;
    sendJson(res, 200, j);
}

bool OctiHttpServer::Impl::validateOauthHostAndCallbackUrl(
    const Request &req, std::string &outCallbackUrl) const
{
    outCallbackUrl.clear();
    size_t hostCount = req.get_header_value_count("Host");
    if (hostCount > 1) {
        return false;
    }
    std::string validatedAuthority;
    std::string hostOnly;
    if (hostCount == 1) {
        std::string rawHost = req.get_header_value("Host");
        if (!splitAuthorityHostOnly(rawHost, hostOnly)) {
            return false;
        }
        if (!normalizedAllowedHosts.empty() &&
            std::find(normalizedAllowedHosts.begin(),
                      normalizedAllowedHosts.end(),
                      hostOnly) == normalizedAllowedHosts.end()) {
            return false;
        }
        validatedAuthority = rawHost;
        while (!validatedAuthority.empty() &&
               std::isspace(static_cast<unsigned char>(validatedAuthority.front()))) {
            validatedAuthority.erase(validatedAuthority.begin());
        }
        while (!validatedAuthority.empty() &&
               std::isspace(static_cast<unsigned char>(validatedAuthority.back()))) {
            validatedAuthority.pop_back();
        }
    } else {
        std::string bindHost = cfg.bindAddress;
        if (bindHost.empty() || bindHost == "0.0.0.0") {
            bindHost = "127.0.0.1";
        } else if (bindHost == "::") {
            bindHost = "[::1]";
        } else if (bindHost.find(':') != std::string::npos && bindHost[0] != '[') {
            bindHost = "[" + bindHost + "]";
        }
        hostOnly = lower(bindHost);
        validatedAuthority = bindHost + ":" + std::to_string(boundPort.load());
    }

    if (!cfg.publicBaseUrl.empty()) {
        std::string base = cfg.publicBaseUrl;
        while (!base.empty() && base.back() == '/') {
            base.pop_back();
        }
        outCallbackUrl = base + "/api/v1/auth/callback";
        return true;
    }

    std::string proto;
    if (cfg.behindTls) {
        proto = "https";
    } else {
        std::string peer = OctiHttpServer::normalizeIpLiteral(req.remote_addr);
        bool peerTrusted = !peer.empty() &&
            std::find(normalizedTrustedProxies.begin(),
                      normalizedTrustedProxies.end(),
                      peer) != normalizedTrustedProxies.end();
        if (peerTrusted && req.has_header("X-Forwarded-Proto")) {
            std::string xfp = lower(req.get_header_value("X-Forwarded-Proto"));
            size_t comma = xfp.find(',');
            if (comma != std::string::npos) {
                xfp = xfp.substr(comma + 1);
            }
            while (!xfp.empty() &&
                   std::isspace(static_cast<unsigned char>(xfp.front()))) {
                xfp.erase(xfp.begin());
            }
            while (!xfp.empty() &&
                   std::isspace(static_cast<unsigned char>(xfp.back()))) {
                xfp.pop_back();
            }
            if (xfp == "https" || xfp == "http") {
                proto = xfp;
            } else if (!xfp.empty()) {
                return false;
            }
        }
        if (proto.empty()) {
            proto = (hostOnly == "octi.selfso.com" ||
                     hostOnly == "trumpet.selfso.com")
                ? "https"
                : "http";
        }
    }
    outCallbackUrl = proto + "://" + validatedAuthority + "/api/v1/auth/callback";
    return true;
}

void OctiHttpServer::Impl::handleAuthAuthorize(const Request &req, Response &res)
{
    if (!allowCredentialAttempt(req, res)) {
        return;
    }
    std::string callbackUrl;
    if (!validateOauthHostAndCallbackUrl(req, callbackUrl)) {
        applyBanSideEffects(banManager.recordExploitProbe(
            store, buildAbuseContext(req, 0), store.currentEpochSec()));
        sendError(res, 400, "INVALID_HOST", "untrusted or malformed Host header");
        return;
    }

    std::string provider = req.get_param_value("provider");
    if (provider != "google" && provider != "facebook") {
        sendError(res, 400, "UNSUPPORTED_PROVIDER", "unsupported provider");
        return;
    }

    if (!verifier.isProviderEnabled(provider)) {
        sendError(res, 503, "SSO_PROVIDER_NOT_CONFIGURED", "SSO provider is not enabled");
        return;
    }

    std::map<std::string, std::vector<std::string>>::const_iterator it =
        cfg.providerClientIds.find(provider);
    if (it == cfg.providerClientIds.end() || it->second.empty() || it->second[0].empty()) {
        sendError(res, 503, "SSO_PROVIDER_NOT_CONFIGURED", "SSO provider client_id not configured");
        return;
    }
    const std::string &clientId = it->second[0];

    std::string redirectUri = "/#account";
    if (req.has_param("redirect_uri")) {
        redirectUri = req.get_param_value("redirect_uri");
    }

    bool isNativeRedirect = false;
    if (!isValidOauthRedirectUri(redirectUri, isNativeRedirect)) {
        sendError(res, 400, "INVALID_REDIRECT_URI", "invalid redirect_uri");
        return;
    }

    std::string jti;
    std::string oidcNonce;
    if (!randomTokenB64Url(16, jti) || !randomTokenB64Url(16, oidcNonce)) {
        sendError(res, 500, "INTERNAL_ERROR", "failed to generate OAuth nonce");
        return;
    }

    const int64_t now = epochSec();
    const int64_t exp = now + 600;
    const size_t maxPending = cfg.maxPendingOauthStates > 0
        ? cfg.maxPendingOauthStates
        : static_cast<size_t>(16384);
    {
        std::lock_guard<std::mutex> lock(oauthStateMutex);
        while (!oauthStateOrder.empty()) {
            const uint64_t frontSeq = oauthStateOrder.front().first;
            const std::string &frontJti = oauthStateOrder.front().second;
            auto fit = pendingOauthStates.find(frontJti);
            if (fit == pendingOauthStates.end() || fit->second.seq != frontSeq) {
                oauthStateOrder.pop_front();
                continue;
            }
            if (fit->second.expiresAtSec < now) {
                pendingOauthStates.erase(fit);
                oauthStateOrder.pop_front();
                continue;
            }
            break;
        }
        while (pendingOauthStates.size() >= maxPending && !oauthStateOrder.empty()) {
            const uint64_t frontSeq = oauthStateOrder.front().first;
            const std::string frontJti = oauthStateOrder.front().second;
            oauthStateOrder.pop_front();
            auto fit = pendingOauthStates.find(frontJti);
            if (fit != pendingOauthStates.end() && fit->second.seq == frontSeq) {
                pendingOauthStates.erase(fit);
            }
        }
        if (pendingOauthStates.size() >= maxPending) {
            pendingOauthStates.clear();
            oauthStateOrder.clear();
        }
        PendingOauthState st;
        st.expiresAtSec = exp;
        st.seq = ++oauthStateSeq;
        st.oidcNonce = oidcNonce;
        pendingOauthStates[jti] = st;
        oauthStateOrder.push_back(std::make_pair(st.seq, jti));
    }

    Json stateObj;
    stateObj["provider"] = provider;
    stateObj["redirect_uri"] = redirectUri;
    stateObj["callback_url"] = callbackUrl;
    stateObj["jti"] = jti;
    stateObj["oidc_nonce"] = oidcNonce;
    stateObj["exp"] = exp;

    std::string statePayload = stateObj.dump();
    std::string stateB64 = base64UrlEncode(statePayload);
    std::string sig = hmacSha256(stateB64, oauthSecret.data(), oauthSecret.size());
    std::string sigB64 = base64UrlEncode(sig);
    std::string stateParam = stateB64 + "." + sigB64;

    std::string authUrl;
    if (provider == "google") {
        authUrl = "https://accounts.google.com/o/oauth2/v2/auth?client_id=" +
                  urlEncode(clientId) +
                  "&response_type=code&scope=openid%20email%20profile&redirect_uri=" +
                  urlEncode(callbackUrl) + "&state=" + urlEncode(stateParam) +
                  "&nonce=" + urlEncode(oidcNonce);
    } else {
        authUrl = "https://www.facebook.com/v19.0/dialog/oauth?client_id=" +
                  urlEncode(clientId) +
                  "&response_type=code&scope=public_profile,email&redirect_uri=" +
                  urlEncode(callbackUrl) + "&state=" + urlEncode(stateParam);
    }

    res.status = 302;
    res.set_header("Location", authUrl);
    res.set_header("Cache-Control", "no-store");
}

void OctiHttpServer::Impl::handleAuthCallback(const Request &req, Response &res)
{
    if (!allowCredentialAttempt(req, res)) {
        return;
    }
    std::string dummyCallbackUrl;
    if (!validateOauthHostAndCallbackUrl(req, dummyCallbackUrl)) {
        applyBanSideEffects(banManager.recordExploitProbe(
            store, buildAbuseContext(req, 0), store.currentEpochSec()));
        sendError(res, 400, "INVALID_HOST", "untrusted or malformed Host header");
        return;
    }

    std::string stateParam = req.get_param_value("state");
    size_t dot = stateParam.find('.');
    if (dot == std::string::npos || dot == 0 || dot + 1 >= stateParam.size() ||
        stateParam.find('.', dot + 1) != std::string::npos) {
        applyBanSideEffects(banManager.recordExploitProbe(
            store, buildAbuseContext(req, 0), store.currentEpochSec()));
        sendError(res, 400, "INVALID_OAUTH_STATE", "invalid state parameter");
        return;
    }

    std::string stateB64 = stateParam.substr(0, dot);
    std::string sigB64 = stateParam.substr(dot + 1);
    std::string sig;
    if (!base64UrlDecode(sigB64, sig)) {
        applyBanSideEffects(banManager.recordExploitProbe(
            store, buildAbuseContext(req, 0), store.currentEpochSec()));
        sendError(res, 400, "INVALID_OAUTH_STATE", "malformed state signature");
        return;
    }
    std::string expectedSig = hmacSha256(stateB64, oauthSecret.data(),
                                         oauthSecret.size());
    if (expectedSig.empty() || sig.size() != expectedSig.size() ||
        CRYPTO_memcmp(sig.data(), expectedSig.data(), expectedSig.size()) != 0) {
        applyBanSideEffects(banManager.recordExploitProbe(
            store, buildAbuseContext(req, 0), store.currentEpochSec()));
        sendError(res, 400, "INVALID_OAUTH_STATE", "state signature mismatch");
        return;
    }

    std::string payloadJson;
    if (!base64UrlDecode(stateB64, payloadJson)) {
        applyBanSideEffects(banManager.recordExploitProbe(
            store, buildAbuseContext(req, 0), store.currentEpochSec()));
        sendError(res, 400, "INVALID_OAUTH_STATE", "malformed state payload");
        return;
    }
    Json stateObj = Json::parse(payloadJson, nullptr, false);
    if (stateObj.is_discarded() || !stateObj.is_object()) {
        applyBanSideEffects(banManager.recordExploitProbe(
            store, buildAbuseContext(req, 0), store.currentEpochSec()));
        sendError(res, 400, "INVALID_OAUTH_STATE", "invalid state JSON");
        return;
    }

    const int64_t now = epochSec();
    int64_t exp = stateObj.value("exp", 0LL);
    if (exp < now) {
        sendError(res, 400, "INVALID_OAUTH_STATE", "state has expired");
        return;
    }

    std::string jti = stateObj.value("jti", "");
    if (jti.empty()) {
        sendError(res, 400, "INVALID_OAUTH_STATE", "missing state jti");
        return;
    }
    PendingOauthState consumedState;
    {
        std::lock_guard<std::mutex> lock(oauthStateMutex);
        auto it = pendingOauthStates.find(jti);
        if (it == pendingOauthStates.end()) {
            sendError(res, 400, "INVALID_OAUTH_STATE",
                      "state has already been used or expired");
            return;
        }
        consumedState = it->second;
        pendingOauthStates.erase(it);
    }
    if (consumedState.expiresAtSec < now) {
        sendError(res, 400, "INVALID_OAUTH_STATE", "state has expired");
        return;
    }

    std::string provider = stateObj.value("provider", "");
    if (provider != "google" && provider != "facebook") {
        sendError(res, 400, "INVALID_OAUTH_STATE", "unsupported provider in state");
        return;
    }
    std::string redirectUri = stateObj.value("redirect_uri", "/#account");
    bool isNativeRedirect = false;
    if (!isValidOauthRedirectUri(redirectUri, isNativeRedirect)) {
        sendError(res, 400, "INVALID_REDIRECT_URI", "invalid redirect_uri in state");
        return;
    }
    std::string callbackUrl = stateObj.value("callback_url", "");
    if (callbackUrl.empty()) {
        sendError(res, 400, "INVALID_OAUTH_STATE", "missing callback_url in state");
        return;
    }

    std::string clientId;
    std::map<std::string, std::vector<std::string>>::const_iterator cit =
        cfg.providerClientIds.find(provider);
    if (cit != cfg.providerClientIds.end() && !cit->second.empty()) {
        clientId = cit->second[0];
    }
    std::string clientSecret;
    std::map<std::string, std::string>::const_iterator sit =
        cfg.providerClientSecrets.find(provider);
    if (sit != cfg.providerClientSecrets.end()) {
        clientSecret = sit->second;
    }

    const std::string rawIdTokenParam = req.get_param_value("id_token");
    const std::string rawAccessTokenParam = req.get_param_value("access_token");
    const std::string code = req.get_param_value("code");

    const bool strictCodeExchange =
        !clientSecret.empty() || !cfg.allowSyntheticOauthCallbackForTesting;

    std::string idToken;
    std::string accessToken;

    if (strictCodeExchange) {
        if (!rawIdTokenParam.empty() || !rawAccessTokenParam.empty()) {
            applyBanSideEffects(banManager.recordExploitProbe(
                store, buildAbuseContext(req, 0), store.currentEpochSec()));
            sendError(res, 400, "INVALID_OAUTH_CALLBACK",
                      "direct token parameters are not accepted");
            return;
        }
        if (code.empty() || code.size() > 2048 || hasControlChars(code)) {
            sendError(res, 400, "INVALID_OAUTH_CALLBACK",
                      "missing or invalid authorization code");
            return;
        }
        int codeDots = 0;
        for (size_t i = 0; i < code.size(); ++i) {
            if (code[i] == '.') {
                ++codeDots;
            }
        }
        if (codeDots >= 2) {
            applyBanSideEffects(banManager.recordExploitProbe(
                store, buildAbuseContext(req, 0), store.currentEpochSec()));
            sendError(res, 400, "INVALID_OAUTH_CALLBACK",
                      "authorization code must not be a JWT");
            return;
        }
        if (clientId.empty() || clientSecret.empty()) {
            sendError(res, 503, "SSO_PROVIDER_NOT_CONFIGURED",
                      "SSO provider client_secret not configured");
            return;
        }

        if (provider == "google") {
            httplib::SSLClient gcli("oauth2.googleapis.com", 443);
            gcli.enable_server_certificate_verification(true);
            gcli.set_connection_timeout(5, 0);
            gcli.set_read_timeout(5, 0);
            gcli.set_write_timeout(5, 0);
            httplib::Params params{
                {"code", code},
                {"client_id", clientId},
                {"client_secret", clientSecret},
                {"redirect_uri", callbackUrl},
                {"grant_type", "authorization_code"}
            };
            httplib::Result gres = gcli.Post("/token", params);
            if (!gres || gres->status != 200 || gres->body.size() > 65536) {
                int stCode = (gres && gres->status >= 400 && gres->status < 500)
                    ? 401
                    : 502;
                sendError(res, stCode, "OAUTH_TOKEN_EXCHANGE_FAILED",
                          "failed to exchange authorization code");
                return;
            }
            Json tj = Json::parse(gres->body, nullptr, false);
            if (!tj.is_object() || !tj.contains("id_token") ||
                !tj["id_token"].is_string()) {
                sendError(res, 401, "OAUTH_TOKEN_EXCHANGE_FAILED",
                          "token endpoint response missing id_token");
                return;
            }
            idToken = tj["id_token"].get<std::string>();
        } else {
            httplib::SSLClient fbcli("graph.facebook.com", 443);
            fbcli.enable_server_certificate_verification(true);
            fbcli.set_connection_timeout(5, 0);
            fbcli.set_read_timeout(5, 0);
            fbcli.set_write_timeout(5, 0);
            std::string fbPath = "/v19.0/oauth/access_token?client_id=" +
                                 urlEncode(clientId) +
                                 "&redirect_uri=" + urlEncode(callbackUrl) +
                                 "&client_secret=" + urlEncode(clientSecret) +
                                 "&code=" + urlEncode(code);
            httplib::Result fbres = fbcli.Get(fbPath.c_str());
            if (!fbres || fbres->status != 200 || fbres->body.size() > 65536) {
                int stCode = (fbres && fbres->status >= 400 && fbres->status < 500)
                    ? 401
                    : 502;
                sendError(res, stCode, "OAUTH_TOKEN_EXCHANGE_FAILED",
                          "failed to exchange authorization code");
                return;
            }
            Json tj = Json::parse(fbres->body, nullptr, false);
            if (!tj.is_object() || !tj.contains("access_token") ||
                !tj["access_token"].is_string()) {
                sendError(res, 401, "OAUTH_TOKEN_EXCHANGE_FAILED",
                          "token endpoint response missing access_token");
                return;
            }
            accessToken = tj["access_token"].get<std::string>();
        }
    } else {
        idToken = rawIdTokenParam;
        accessToken = rawAccessTokenParam;
        if (idToken.empty() && accessToken.empty() && !code.empty()) {
            idToken = code;
        }
        if (provider == "facebook" && accessToken.empty() && !idToken.empty()) {
            int dots = 0;
            for (size_t i = 0; i < idToken.size(); ++i) {
                if (idToken[i] == '.') {
                    ++dots;
                }
            }
            if (dots < 2) {
                accessToken = idToken;
                idToken.clear();
            }
        }
    }

    VerifyResult vr;
    if (!accessToken.empty()) {
        if (provider != "facebook") {
            sendError(res, 400, "INVALID_ARGUMENT", "access_token is only accepted for facebook");
            return;
        }
        vr = verifier.verifyFacebookAccessToken(accessToken);
    } else if (!idToken.empty()) {
        vr = verifier.verifyIdToken(provider, idToken);
    } else {
        sendError(res, 400, "INVALID_ARGUMENT", "missing id_token or access_token");
        return;
    }

    if (!vr.ok()) {
        const int stCode = statusForVerify(vr.status);
        if (stCode == 401) {
            applyBanSideEffects(banManager.recordAuthFailure(
                store, buildAbuseContext(req, 0), store.currentEpochSec()));
        }
        sendError(res, stCode, verifyStatusName(vr.status), vr.detail);
        return;
    }

    if (provider == "google") {
        const std::string &expectedNonce = consumedState.oidcNonce;
        if (expectedNonce.empty() ||
            vr.claims.nonce.size() != expectedNonce.size() ||
            CRYPTO_memcmp(vr.claims.nonce.data(), expectedNonce.data(),
                          expectedNonce.size()) != 0) {
            applyBanSideEffects(banManager.recordAuthFailure(
                store, buildAbuseContext(req, 0), store.currentEpochSec()));
            sendError(res, 401, "TOKEN_REJECTED", "OIDC nonce mismatch");
            return;
        }
    }

    LoginResult lr = store.loginWithSso(vr.claims, "octi-oauth-broker/1.0");
    const int64_t cbNowSec = store.currentEpochSec();
    if (lr.banned) {
        BanMatch bm;
        const BanMatch *bmp =
            (!lr.profile.uuid.empty() &&
             banManager.matchAccount(lr.profile.uuid, cbNowSec, bm))
                ? &bm
                : nullptr;
        sendAccountBanned(res, bmp, cbNowSec);
        return;
    }
    if (!lr.ok) {
        sendError(res, 500, "INTERNAL_ERROR", "sign-in failed");
        return;
    }
    BanMatch cbBm;
    if (banManager.matchAccount(lr.profile.uuid, cbNowSec, cbBm)) {
        store.revokeAllSessions(lr.profile.playerId);
        sendAccountBanned(res, &cbBm, cbNowSec);
        return;
    }
    if (lr.profile.isAdmin) {
        banManager.noteVerifiedAdminIp(clientRemoteIp(req), cbNowSec);
    }
    rooms.enterLobby(lr.profile.playerId);
    rooms.recordClientHeartbeat(lr.profile.playerId, "web", true);

    std::string targetLoc;
    std::string isNewStr = lr.created ? "true" : "false";
    if (isNativeRedirect) {
        const std::string &rawTok = !idToken.empty() ? idToken : accessToken;
        targetLoc = redirectUri + (redirectUri.find('?') != std::string::npos ? "&" : "?") +
                    "id_token=" + urlEncode(rawTok) +
                    "&auth_token=" + urlEncode(lr.sessionToken) +
                    "&provider=" + urlEncode(provider) +
                    "&is_new_account=" + isNewStr +
                    "&handle=" + urlEncode(lr.profile.handle) +
                    "&player_uuid=" + urlEncode(lr.profile.uuid);
    } else {
        targetLoc = "/#auth_token=" + urlEncode(lr.sessionToken) +
                    "&provider=" + urlEncode(provider) +
                    "&is_new_account=" + isNewStr;
    }

    res.status = 302;
    res.set_header("Location", targetLoc);
    res.set_header("Cache-Control", "no-store");
}

void OctiHttpServer::Impl::handleProviders(const Request &, Response &res)
{
    Json arr = Json::array();
    const std::vector<ProviderDescriptor> &reg =
        OidcVerifier::providerRegistry();
    for (size_t i = 0; i < reg.size(); ++i) {
        if (!verifier.isProviderEnabled(reg[i].name)) {
            continue;
        }
        Json p;
        p["name"] = reg[i].name;
        p["display_name"] = reg[i].displayName;
        Json ids = Json::array();
        std::map<std::string, std::vector<std::string> >::const_iterator it =
            cfg.providerClientIds.find(reg[i].name);
        if (it != cfg.providerClientIds.end()) {
            for (size_t k = 0; k < it->second.size(); ++k) {
                ids.push_back(it->second[k]);
            }
        }
        p["client_ids"] = ids;
        arr.push_back(p);
    }
    Json j;
    j["providers"] = arr;
    sendJson(res, 200, j);
}

/*
 * Fixed-window and token-bucket rate limiters keyed per canonical client IP
 * (and per authenticated actor where applicable).
 */
bool OctiHttpServer::Impl::allowCredentialAttempt(const Request &req,
                                                  Response &res)
{
    int64_t retrySec = 1;
    if (credentialLimiter.tryAcquire("cred:" + clientRemoteIp(req),
                                     cfg.ssoRateLimitMax,
                                     cfg.ssoRateLimitWindowMs, retrySec)) {
        return true;
    }
    noteRateLimit429(req, 0);
    sendError(res, 429, "RATE_LIMITED",
              "too many sign-in attempts; retry later");
    res.set_header("Retry-After", std::to_string(retrySec));
    return false;
}

bool OctiHttpServer::Impl::allowUploadAttempt(const Request &req, Response &res)
{
    int64_t retrySec = 1;
    if (uploadLimiter.tryAcquire("upload:" + clientRemoteIp(req),
                                 cfg.uploadRateLimitMax,
                                 cfg.uploadRateLimitWindowMs, retrySec)) {
        return true;
    }
    noteRateLimit429(req, 0);
    sendError(res, 429, "RATE_LIMITED",
              "too many upload attempts; retry later");
    res.set_header("Retry-After", std::to_string(retrySec));
    return false;
}

bool OctiHttpServer::Impl::allowExportAttempt(const Request &req, Response &res,
                                              int64_t actorId)
{
    int64_t retrySec = 1;
    const std::string ipKey = "export_ip:" + clientRemoteIp(req);
    const std::string actorKey =
        actorId != 0 ? ("export_actor:" + std::to_string(actorId))
                     : std::string();
    if (exportLimiter.tryAcquireDual(actorKey, cfg.exportRateLimitMax,
                                     ipKey, cfg.exportRateLimitMax,
                                     cfg.exportRateLimitWindowMs, retrySec)) {
        return true;
    }
    noteRateLimit429(req, actorId);
    sendError(res, 429, "RATE_LIMITED",
              "too many export attempts; retry later");
    res.set_header("Retry-After", std::to_string(retrySec));
    return false;
}

bool OctiHttpServer::Impl::allowRobotExportConfAttempt(const Request &req,
                                                       Response &res,
                                                       int64_t actorId)
{
    int64_t retrySec = 1;
    const std::string ipKey = "botconf_ip:" + clientRemoteIp(req);
    const std::string actorKey =
        actorId != 0 ? ("botconf_actor:" + std::to_string(actorId))
                     : std::string();
    if (robotExportConfLimiter.tryAcquireDual(
            actorKey, cfg.robotExportConfRateLimitMax,
            ipKey, cfg.robotExportConfRateLimitMax,
            cfg.robotExportConfRateLimitWindowMs, retrySec)) {
        return true;
    }
    noteRateLimit429(req, actorId);
    sendError(res, 429, "RATE_LIMITED",
              "too many robot config export attempts; retry later");
    res.set_header("Retry-After", std::to_string(retrySec));
    return false;
}

bool OctiHttpServer::Impl::allowPliesAttempt(const Request &req, Response &res)
{
    int64_t retrySec = 1;
    if (pliesLimiter.tryAcquire("plies:" + clientRemoteIp(req),
                                cfg.pliesRateLimitMax,
                                cfg.pliesRateLimitWindowMs, retrySec)) {
        return true;
    }
    noteRateLimit429(req, 0);
    if (!hasValidBearerAuth(req)) {
        applyBanSideEffects(banManager.recordAnonymousEnumeration(
            store, clientRemoteIp(req), req.path, 429,
            store.currentEpochSec()));
    }
    sendError(res, 429, "RATE_LIMITED",
              "too many ply replay requests; retry later");
    res.set_header("Retry-After", std::to_string(retrySec));
    return false;
}

bool OctiHttpServer::Impl::allowProbeAttempt(const Request &req, Response &res,
                                             int64_t actorId)
{
    int64_t retrySec = 1;
    const std::string ipKey = "probe_ip:" + clientRemoteIp(req);
    const std::string actorKey =
        actorId != 0 ? ("probe_actor:" + std::to_string(actorId))
                     : std::string();
    if (probeLimiter.tryAcquireDual(actorKey, cfg.probeRateLimitMax,
                                    ipKey, cfg.probeRateLimitMax,
                                    cfg.probeRateLimitWindowMs, retrySec)) {
        return true;
    }
    noteRateLimit429(req, actorId);
    sendError(res, 429, "RATE_LIMITED",
              "too many robot URL verification probes; retry later");
    res.set_header("Retry-After", std::to_string(retrySec));
    return false;
}

bool OctiHttpServer::Impl::allowMutationAttempt(const Request &req,
                                                Response &res,
                                                int64_t actorId)
{
    if (cfg.mutationRateLimitMax <= 0 || cfg.mutationRateLimitWindowMs <= 0) {
        return true;
    }
    int64_t retrySec = 1;
    const std::string ip = clientRemoteIp(req);
    bool ok = false;
    if (actorId != 0) {
        const int sharedNatCap = std::max(cfg.mutationRateLimitMax,
                                          cfg.mutationRateLimitMax * 4);
        ok = mutationLimiter.tryAcquireDual(
            "mut_actor:" + std::to_string(actorId), cfg.mutationRateLimitMax,
            "mut_authed_ip:" + ip, sharedNatCap,
            cfg.mutationRateLimitWindowMs, retrySec);
    } else {
        ok = mutationLimiter.tryAcquire(
            "mut_anon_ip:" + ip, cfg.mutationRateLimitMax,
            cfg.mutationRateLimitWindowMs, retrySec);
    }
    if (ok) {
        return true;
    }
    noteRateLimit429(req, actorId);
    sendError(res, 429, "RATE_LIMITED",
              "too many write requests; retry later");
    res.set_header("Retry-After", std::to_string(retrySec));
    return false;
}

bool OctiHttpServer::Impl::checkPasscodeBlocked(const Request &req,
                                                Response &res,
                                                int64_t actorId)
{
    int64_t retrySec = 1;
    const std::string ipKey = "pass_ip:" + clientRemoteIp(req);
    const std::string actorKey =
        actorId != 0 ? ("pass_actor:" + std::to_string(actorId))
                     : std::string();
    if (passcodeFailureLimiter.isBlocked(ipKey, cfg.passcodeFailureRateLimitMax,
                                         cfg.passcodeFailureRateLimitWindowMs,
                                         retrySec) ||
        (!actorKey.empty() &&
         passcodeFailureLimiter.isBlocked(actorKey,
                                          cfg.passcodeFailureRateLimitMax,
                                          cfg.passcodeFailureRateLimitWindowMs,
                                          retrySec))) {
        noteRateLimit429(req, actorId);
        sendError(res, 429, "RATE_LIMITED",
                  "too many failed passcode attempts; retry later");
        res.set_header("Retry-After", std::to_string(retrySec));
        return true;
    }
    return false;
}

void OctiHttpServer::Impl::recordPasscodeFailure(const Request &req,
                                                 Response &res,
                                                 int64_t actorId,
                                                 const RoomResult &r)
{
    if (r.code != RoomCode::FORBIDDEN_PASSCODE) {
        return;
    }
    applyBanSideEffects(banManager.recordPasscodeFailure(
        store, buildAbuseContext(req, actorId), store.currentEpochSec()));
    int64_t retrySec = 1;
    const std::string ipKey = "pass_ip:" + clientRemoteIp(req);
    const std::string actorKey =
        actorId != 0 ? ("pass_actor:" + std::to_string(actorId))
                     : std::string();
    if (!passcodeFailureLimiter.tryAcquireDual(
            actorKey, cfg.passcodeFailureRateLimitMax,
            ipKey, cfg.passcodeFailureRateLimitMax,
            cfg.passcodeFailureRateLimitWindowMs, retrySec)) {
        noteRateLimit429(req, actorId);
        sendError(res, 429, "RATE_LIMITED",
                  "too many failed passcode attempts; retry later");
        res.set_header("Retry-After", std::to_string(retrySec));
    }
}

bool OctiHttpServer::Impl::allowLivePoll(const Request &req, Response &res,
                                         const Auth &auth)
{
    int64_t retrySec = 1;
    const std::string ip = clientRemoteIp(req);
    bool ok = false;
    if (auth.playerId != 0) {
        ok = livePollLimiter.tryAcquireDual(
            "actor:" + std::to_string(auth.playerId),
            cfg.livePollActorRefillPerSec, cfg.livePollActorBurst,
            "authed_ip:" + ip,
            cfg.livePollAuthedIpRefillPerSec, cfg.livePollAuthedIpBurst,
            retrySec);
    } else {
        ok = livePollLimiter.tryAcquire(
            "anon_ip:" + ip,
            cfg.livePollAnonIpRefillPerSec, cfg.livePollAnonIpBurst,
            retrySec);
    }
    if (ok) {
        return true;
    }
    noteRateLimit429(req, auth.playerId);
    sendError(res, 429, "RATE_LIMITED",
              "too many poll requests; retry later");
    res.set_header("Retry-After", std::to_string(retrySec));
    return false;
}

bool OctiHttpServer::Impl::allowLobbySnap(const Request &req, Response &res,
                                          const Auth &auth)
{
    int64_t retrySec = 1;
    const std::string ip = clientRemoteIp(req);
    bool ok = false;
    if (auth.playerId != 0) {
        ok = lobbySnapLimiter.tryAcquireDual(
            "actor:" + std::to_string(auth.playerId),
            cfg.lobbySnapActorRefillPerSec, cfg.lobbySnapActorBurst,
            "authed_ip:" + ip,
            cfg.lobbySnapAuthedIpRefillPerSec, cfg.lobbySnapAuthedIpBurst,
            retrySec);
    } else {
        ok = lobbySnapLimiter.tryAcquire(
            "anon_ip:" + ip,
            cfg.lobbySnapAnonIpRefillPerSec, cfg.lobbySnapAnonIpBurst,
            retrySec);
    }
    if (ok) {
        return true;
    }
    noteRateLimit429(req, auth.playerId);
    sendError(res, 429, "RATE_LIMITED",
              "too many lobby snapshot requests; retry later");
    res.set_header("Retry-After", std::to_string(retrySec));
    return false;
}

bool OctiHttpServer::Impl::allowBrowseRead(const Request &req, Response &res,
                                           const Auth &auth)
{
    int64_t retrySec = 1;
    const std::string ip = clientRemoteIp(req);
    bool ok = false;
    if (auth.playerId != 0) {
        ok = browseLimiter.tryAcquireDual(
            "actor:" + std::to_string(auth.playerId),
            cfg.browseActorRefillPerSec, cfg.browseActorBurst,
            "authed_ip:" + ip,
            cfg.browseAuthedIpRefillPerSec, cfg.browseAuthedIpBurst,
            retrySec);
    } else {
        ok = browseLimiter.tryAcquire(
            "anon_ip:" + ip,
            cfg.browseAnonIpRefillPerSec, cfg.browseAnonIpBurst,
            retrySec);
    }
    if (ok) {
        return true;
    }
    noteRateLimit429(req, auth.playerId);
    if (auth.playerId == 0 &&
        (req.path.rfind("/api/v1/archive/games/", 0) == 0 ||
         req.path.rfind("/api/v1/users/", 0) == 0 ||
         req.path.rfind("/api/v1/profile/", 0) == 0 ||
         req.path.rfind("/api/v1/players/", 0) == 0)) {
        applyBanSideEffects(banManager.recordAnonymousEnumeration(
            store, ip, req.path, 429, store.currentEpochSec()));
    }
    sendError(res, 429, "RATE_LIMITED",
              "too many directory or archive read requests; retry later");
    res.set_header("Retry-After", std::to_string(retrySec));
    return false;
}

void OctiHttpServer::Impl::handleSso(const Request &req, Response &res)
{
    if (!allowCredentialAttempt(req, res)) {
        return;
    }
    Json body;
    if (!readJsonBody(req, res, body, false)) {
        return;
    }
    std::string agent;
    std::string err;
    if (!fieldString(body, "client_agent", false, agent, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    VerifiedClaims claims;
    if (!verifyCredential(req, body, res, claims)) {
        return;
    }
    const int64_t nowSec = store.currentEpochSec();
    LoginResult lr = store.loginWithSso(claims, cleanAgent(agent));
    if (lr.banned) {
        BanMatch bm;
        const BanMatch *bmp =
            (!lr.profile.uuid.empty() &&
             banManager.matchAccount(lr.profile.uuid, nowSec, bm))
                ? &bm
                : nullptr;
        sendAccountBanned(res, bmp, nowSec);
        return;
    }
    if (!lr.ok) {
        sendError(res, 500, "INTERNAL_ERROR", "sign-in failed");
        return;
    }
    BanMatch bm;
    if (!lr.profile.uuid.empty() &&
        banManager.matchAccount(lr.profile.uuid, nowSec, bm)) {
        store.revokeAllSessions(lr.profile.playerId);
        sendAccountBanned(res, &bm, nowSec);
        return;
    }
    if (lr.profile.isAdmin) {
        banManager.noteVerifiedAdminIp(clientRemoteIp(req), nowSec);
    }
    std::string bodyInstallId;
    if (body.contains("install_id") && body["install_id"].is_string()) {
        bodyInstallId = body["install_id"].get<std::string>();
    }
    maybeBindClientInstallId(req, lr.profile.playerId, bodyInstallId);
    rooms.enterLobby(lr.profile.playerId);
    std::string cType = detectClientType(req);
    if (cType.empty()) {
        std::string lAgent = lower(agent);
        if (lAgent.find("android") != std::string::npos) {
            cType = "android";
        } else if (lAgent.find("desktop") != std::string::npos) {
            cType = "desktop";
        } else if (lAgent.find("web") != std::string::npos ||
                   lAgent.find("mozilla") != std::string::npos) {
            cType = "web";
        }
    }
    rooms.recordClientHeartbeat(lr.profile.playerId, cType, true);
    Json j;
    j["session_token"] = lr.sessionToken;
    j["expires_at"] = lr.sessionExpiresAt;
    j["created"] = lr.created;
    j["profile"] = profileJson(lr.profile, true);
    j["linked_identities"] = identitiesJson(lr.identities);
    sendJson(res, lr.created ? 201 : 200, j);
}

void OctiHttpServer::Impl::handleLink(const Request &req, Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth) || !allowCredentialAttempt(req, res)) {
        return;
    }
    Json body;
    if (!readJsonBody(req, res, body, false)) {
        return;
    }
    VerifiedClaims claims;
    if (!verifyCredential(req, body, res, claims)) {
        return;
    }
    bool forceRelink = false;
    std::string err;
    fieldBool(body, "force_relink", false, forceRelink, err);
    LinkResult lr = store.linkIdentity(auth.playerId, claims, forceRelink);
    switch (lr.status) {
    case LinkStatus::LINKED:
    case LinkStatus::ALREADY_LINKED_SELF: {
        std::string bodyInstallId;
        if (body.contains("install_id") && body["install_id"].is_string()) {
            bodyInstallId = body["install_id"].get<std::string>();
        }
        maybeBindClientInstallId(req, auth.playerId, bodyInstallId);
        std::vector<LinkedIdentity> ids;
        store.listLinkedIdentities(auth.playerId, ids);
        Json j;
        j["ok"] = true;
        j["linked"] = lr.status == LinkStatus::LINKED;
        j["status"] = lr.status == LinkStatus::LINKED ? "linked"
                                                       : "already_linked";
        j["linked_identities"] = identitiesJson(ids);
        sendJson(res, 200, j);
        return;
    }
    case LinkStatus::CONFLICT_ALREADY_LINKED: {
        Json j;
        j["error"] = "CONFLICT_ALREADY_LINKED";
        j["message"] = "this identity is already linked to another profile";
        Json other;
        other["uuid"] = lr.conflictProfile.uuid;
        other["handle"] = lr.conflictProfile.handle;
        other["elo_2p"] = lr.conflictProfile.elo2p;
        other["games_played"] = lr.conflictProfile.gamesPlayed;
        j["existing_profile"] = other;
        sendJson(res, 409, j);
        return;
    }
    case LinkStatus::PLAYER_NOT_FOUND:
        sendError(res, 401, "UNAUTHORIZED", "unknown player");
        return;
    case LinkStatus::INVALID_CLAIMS:
        sendError(res, 400, "INVALID_ARGUMENT", "invalid identity claims");
        return;
    case LinkStatus::ERROR:
        break;
    }
    sendError(res, 500, "INTERNAL_ERROR", "link failed");
}

void OctiHttpServer::Impl::handleUnlink(const Request &req, Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth) ||
        !allowMutationAttempt(req, res, auth.playerId)) {
        return;
    }
    std::unordered_map<std::string, std::string>::const_iterator it =
        req.path_params.find("provider");
    if (it == req.path_params.end() ||
        !OidcVerifier::isSupportedProvider(it->second)) {
        sendError(res, 400, "UNSUPPORTED_PROVIDER", "unsupported provider");
        return;
    }
    switch (store.unlinkIdentity(auth.playerId, it->second)) {
    case UnlinkStatus::UNLINKED: {
        std::vector<LinkedIdentity> ids;
        store.listLinkedIdentities(auth.playerId, ids);
        Json j;
        j["status"] = "unlinked";
        j["linked_identities"] = identitiesJson(ids);
        sendJson(res, 200, j);
        return;
    }
    case UnlinkStatus::LAST_IDENTITY_FORBIDDEN:
        sendError(res, 400, "LAST_IDENTITY_FORBIDDEN",
                  "cannot unlink the only remaining sign-in identity");
        return;
    case UnlinkStatus::NOT_LINKED:
        sendError(res, 404, "NOT_LINKED", "provider is not linked");
        return;
    case UnlinkStatus::ERROR:
        break;
    }
    sendError(res, 500, "INTERNAL_ERROR", "unlink failed");
}

void OctiHttpServer::Impl::handleAuthListIdentities(const Request &req,
                                                   Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth)) {
        return;
    }
    std::vector<LinkedIdentity> ids;
    if (!store.listLinkedIdentities(auth.playerId, ids)) {
        sendError(res, 500, "INTERNAL_ERROR", "failed to list identities");
        return;
    }
    Json j;
    j["ok"] = true;
    j["identities"] = identitiesJson(ids);
    sendJson(res, 200, j);
}

void OctiHttpServer::Impl::handleAuthUnlinkProvider(const Request &req,
                                                   Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth) ||
        !allowMutationAttempt(req, res, auth.playerId)) {
        return;
    }
    std::unordered_map<std::string, std::string>::const_iterator it =
        req.path_params.find("provider");
    if (it == req.path_params.end() ||
        !OidcVerifier::isSupportedProvider(it->second)) {
        sendError(res, 400, "UNSUPPORTED_PROVIDER", "unsupported provider");
        return;
    }
    switch (store.unlinkIdentity(auth.playerId, it->second)) {
    case UnlinkStatus::UNLINKED: {
        std::vector<LinkedIdentity> ids;
        store.listLinkedIdentities(auth.playerId, ids);
        Json j;
        j["ok"] = true;
        j["status"] = "unlinked";
        j["linked_identities"] = identitiesJson(ids);
        sendJson(res, 200, j);
        return;
    }
    case UnlinkStatus::LAST_IDENTITY_FORBIDDEN:
        sendError(res, 409, "cannot_unlink_last_identity",
                  "cannot unlink the only remaining sign-in identity");
        return;
    case UnlinkStatus::NOT_LINKED:
        sendError(res, 404, "NOT_LINKED", "provider is not linked");
        return;
    case UnlinkStatus::ERROR:
        break;
    }
    sendError(res, 500, "INTERNAL_ERROR", "unlink failed");
}

void OctiHttpServer::Impl::handleLogout(const Request &req, Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth) ||
        !allowMutationAttempt(req, res, auth.playerId)) {
        return;
    }
    store.revokeSession(auth.token);
    if (!auth.clientType.empty()) {
        rooms.clearClientPresence(auth.playerId, auth.clientType);
    }
    if (store.countActiveSessions(auth.playerId) == 0) {
        rooms.leaveLobby(auth.playerId);
    }
    Json j;
    j["status"] = "logged_out";
    sendJson(res, 200, j);
}

void OctiHttpServer::Impl::handleProfileGet(const Request &req, Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth)) {
        return;
    }
    if (auth.isRobot) {
        Json j;
        j["uuid"] = auth.robotRecord.robotUuid;
        j["handle"] = auth.robotRecord.robotName;
        j["display_name"] = auth.robotRecord.robotName;
        j["avatar_url"] = auth.robotRecord.ownerAvatarUrl;
        j["bio"] = auth.robotRecord.description;
        j["is_robot"] = true;
        j["robot_uuid"] = auth.robotRecord.robotUuid;
        j["robot_name"] = auth.robotRecord.robotName;
        j["description"] = auth.robotRecord.description;
        j["owner_uuid"] = auth.robotRecord.ownerUuid;
        j["owner_handle"] = auth.robotRecord.ownerHandle;
        j["owner_avatar_url"] = auth.robotRecord.ownerAvatarUrl;
        j["elo"] = auth.robotRecord.elo2p3b;
        j["elo_2p"] = auth.robotRecord.elo2p3b;
        j["elo_2p_3b"] = auth.robotRecord.elo2p3b;
        j["elo_2p_1b"] = auth.robotRecord.elo2p1b;
        j["elo_ofk"] = auth.robotRecord.eloOfk;
        j["games_played"] = auth.robotRecord.gamesPlayed;
        j["wins"] = auth.robotRecord.wins;
        j["losses"] = auth.robotRecord.losses;
        j["draws"] = auth.robotRecord.draws;
        j["created_at"] = auth.robotRecord.createdAt;
        j["linked_identities"] = Json::array();
        sendJson(res, 200, j);
        return;
    }
    PlayerProfile p;
    if (!store.getProfileById(auth.playerId, p)) {
        sendError(res, 401, "UNAUTHORIZED", "unknown player");
        return;
    }
    std::vector<LinkedIdentity> ids;
    store.listLinkedIdentities(auth.playerId, ids);
    Json j = profileJson(p, true);
    j["linked_identities"] = identitiesJson(ids);
    sendJson(res, 200, j);
}

void OctiHttpServer::Impl::handleProfilePatch(const Request &req,
                                              Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth) ||
        !allowMutationAttempt(req, res, auth.playerId)) {
        return;
    }
    Json body;
    if (!readJsonBody(req, res, body, false)) {
        return;
    }
    ProfileUpdate up;
    std::string err;
    if (fieldPresent(body, "handle")) {
        up.setHandle = true;
        if (!fieldString(body, "handle", true, up.handle, err)) {
            sendError(res, 400, "INVALID_ARGUMENT", err);
            return;
        }
    }
    if (fieldPresent(body, "display_name")) {
        up.setDisplayName = true;
        if (!fieldString(body, "display_name", true, up.displayName, err)) {
            sendError(res, 400, "INVALID_ARGUMENT", err);
            return;
        }
    }
    if (fieldPresent(body, "avatar_url")) {
        up.setAvatarUrl = true;
        if (!fieldString(body, "avatar_url", true, up.avatarUrl, err)) {
            sendError(res, 400, "INVALID_ARGUMENT", err);
            return;
        }
    }
    if (fieldPresent(body, "bio")) {
        up.setBio = true;
        if (!fieldString(body, "bio", true, up.bio, err)) {
            sendError(res, 400, "INVALID_ARGUMENT", err);
            return;
        }
    }
    if (fieldPresent(body, "location")) {
        up.setLocation = true;
        if (!fieldString(body, "location", true, up.location, err)) {
            sendError(res, 400, "INVALID_ARGUMENT", err);
            return;
        }
    }
    if (!up.setHandle && !up.setDisplayName && !up.setAvatarUrl && !up.setBio &&
        !up.setLocation) {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "nothing to update: send handle, display_name, avatar_url, bio or location");
        return;
    }
    switch (store.updateProfile(auth.playerId, up)) {
    case UpdateStatus::OK: {
        PlayerProfile p;
        if (!store.getProfileById(auth.playerId, p)) {
            sendError(res, 500, "INTERNAL_ERROR", "profile lookup failed");
            return;
        }
        rooms.enterLobby(auth.playerId);    // refresh the cached handle
        std::vector<LinkedIdentity> ids;
        store.listLinkedIdentities(auth.playerId, ids);
        Json j = profileJson(p, true);
        j["linked_identities"] = identitiesJson(ids);
        sendJson(res, 200, j);
        return;
    }
    case UpdateStatus::NOT_FOUND:
        sendError(res, 401, "UNAUTHORIZED", "unknown player");
        return;
    case UpdateStatus::INVALID_HANDLE:
        sendError(res, 400, "INVALID_HANDLE",
                  "Handle must be 3 to 24 characters using only letters (A-Z, a-z), digits (0-9), underscores (_), or hyphens (-).");
        return;
    case UpdateStatus::HANDLE_TAKEN:
        sendError(res, 409, "HANDLE_TAKEN",
                  "That handle is already taken by another player. Please choose a different handle.");
        return;
    case UpdateStatus::HANDLE_RATE_LIMITED:
        noteRateLimit429(req, auth.playerId);
        sendError(res, 429, "HANDLE_CHANGE_RATE_LIMITED",
                  "Handle can only be changed once every 24 hours after initial setup. Please try again later.");
        return;
    case UpdateStatus::INVALID_DISPLAY_NAME:
        sendError(res, 400, "INVALID_DISPLAY_NAME",
                  "Display name must be 1 to 64 printable characters and cannot be blank.");
        return;
    case UpdateStatus::INVALID_AVATAR_URL:
        if (up.setAvatarUrl && lower(up.avatarUrl).rfind("data:", 0) == 0) {
            applyBanSideEffects(banManager.recordExploitProbe(
                store, buildAbuseContext(req, auth.playerId),
                store.currentEpochSec()));
        }
        sendError(res, 400, "INVALID_AVATAR_URL",
                  "Avatar must be an uploaded image file or a valid https:// URL (up to 512 characters, no spaces).");
        return;
    case UpdateStatus::INVALID_BIO:
        sendError(res, 400, "INVALID_BIO",
                  "Bio must be 500 characters or fewer.");
        return;
    case UpdateStatus::INVALID_LOCATION:
        sendError(res, 400, "INVALID_LOCATION",
                  "Location must be 64 printable characters or fewer on a single line.");
        return;
    case UpdateStatus::ERROR:
        break;
    }
    sendError(res, 500, "INTERNAL_ERROR", "profile update failed");
}

void OctiHttpServer::Impl::handlePlayer(const Request &req, Response &res)
{
    handleUserGet(req, res);
}

void OctiHttpServer::Impl::handleUsersList(const Request &req, Response &res)
{
    Auth auth;
    if (!optionalAuth(req, res, auth) || !allowBrowseRead(req, res, auth)) {
        return;
    }
    UserDirectoryFilter filter;
    if (req.has_param("q")) {
        filter.query = req.get_param_value("q");
    }
    if (req.has_param("sort")) {
        filter.sortField = req.get_param_value("sort");
    }
    if (filter.sortField.empty()) {
        filter.sortField = "elo_2p_3b";
    }
    if (req.has_param("order")) {
        std::string ord = lower(req.get_param_value("order"));
        if (ord == "asc") {
            filter.sortAsc = true;
        } else if (ord == "desc") {
            filter.sortAsc = false;
        }
    }
    int64_t limit = 50;
    if (req.has_param("limit")) {
        if (!parseUnsignedClamped(req.get_param_value("limit"), 100, limit) || limit < 1) {
            sendError(res, 400, "INVALID_ARGUMENT", "limit must be a positive integer");
            return;
        }
    }
    filter.limit = static_cast<int>(std::max<int64_t>(1, std::min<int64_t>(limit, 100)));

    int64_t offset = 0;
    if (req.has_param("offset")) {
        if (!parseUnsignedClamped(req.get_param_value("offset"), INT64_MAX, offset) || offset < 0) {
            sendError(res, 400, "INVALID_ARGUMENT", "offset must be a non-negative integer");
            return;
        }
    }
    filter.offset = static_cast<int>(offset);

    std::vector<UserDirectoryEntry> entries;
    int64_t total = 0;
    if (!store.listUsers(filter, entries, &total)) {
        sendError(res, 500, "INTERNAL_ERROR", "failed to list users");
        return;
    }

    Json usersArr = Json::array();
    for (size_t i = 0; i < entries.size(); ++i) {
        const auto &e = entries[i];
        Json uj;
        uj["uuid"] = e.profile.uuid;
        uj["handle"] = e.profile.handle;
        uj["display_name"] = e.profile.displayName;
        uj["avatar_url"] = e.profile.avatarUrl;
        uj["location"] = e.profile.location;
        uj["bio"] = e.profile.bio;
        uj["elo_2p"] = e.profile.elo2p3b;
        uj["elo_2p_3b"] = e.profile.elo2p3b;
        uj["elo_2p_1b"] = e.profile.elo2p1b;
        uj["elo_4p"] = e.profile.elo4p;
        uj["elo_ofk"] = e.profile.eloOfk;
        uj["games_2p_3b"] = e.profile.games2p3b;
        uj["games_2p_1b"] = e.profile.games2p1b;
        uj["games_ofk"] = e.profile.gamesOfk;
        uj["games_played"] = e.profile.gamesPlayed;
        uj["wins"] = e.profile.wins;
        uj["losses"] = e.profile.losses;
        uj["draws"] = e.profile.draws;
        uj["arena_elo_2p_3b"] = e.profile.arenaElo2p3b;
        uj["arena_elo_2p_1b"] = e.profile.arenaElo2p1b;
        uj["arena_elo_ofk"] = e.profile.arenaEloOfk;
        uj["arena_games_2p_3b"] = e.profile.arenaGames2p3b;
        uj["arena_games_2p_1b"] = e.profile.arenaGames2p1b;
        uj["arena_games_ofk"] = e.profile.arenaGamesOfk;
        uj["arena_games_played"] = e.profile.arenaGamesPlayed;
        uj["arena_wins"] = e.profile.arenaWins;
        uj["arena_losses"] = e.profile.arenaLosses;
        uj["arena_draws"] = e.profile.arenaDraws;
        uj["active_robots_count"] = e.activeRobotsCount;
        uj["created_at"] = e.profile.createdAt;
        uj["last_seen_at"] = e.profile.lastSeenAt;
        usersArr.push_back(uj);
    }

    Json j;
    j["total"] = total;
    j["limit"] = filter.limit;
    j["offset"] = filter.offset;
    j["users"] = usersArr;
    sendJson(res, 200, j);
}

Json OctiHttpServer::Impl::buildRobotProfilePayload(const RobotRecord &r)
{
    Json rj;
    rj["is_robot"] = true;
    rj["uuid"] = r.robotUuid;
    rj["robot_uuid"] = r.robotUuid;
    rj["name"] = r.robotName;
    rj["handle"] = r.robotName;
    rj["robot_name"] = r.robotName;
    rj["display_name"] = r.robotName;
    rj["description"] = r.description;
    rj["bio"] = r.description;
    rj["robot_url"] = r.robotUrl;
    rj["name_changed_at"] = r.nameChangedAt;
    rj["avatar_url"] = r.ownerAvatarUrl;
    rj["owner_uuid"] = r.ownerUuid;
    rj["owner_handle"] = r.ownerHandle;
    rj["owner_avatar_url"] = r.ownerAvatarUrl;
    rj["elo"] = r.elo2p3b;
    rj["elo_2p"] = r.elo2p3b;
    rj["elo_2p_3b"] = r.elo2p3b;
    rj["elo_2p_1b"] = r.elo2p1b;
    rj["elo_ofk"] = r.eloOfk;
    rj["games_2p_3b"] = r.games2p3b;
    rj["games_2p_1b"] = r.games2p1b;
    rj["games_ofk"] = r.gamesOfk;
    rj["games_played"] = r.gamesPlayed;
    rj["wins"] = r.wins;
    rj["losses"] = r.losses;
    rj["draws"] = r.draws;
    double wr = (r.gamesPlayed > 0)
                    ? std::round((static_cast<double>(r.wins) / r.gamesPlayed) *
                                 1000.0) /
                          10.0
                    : 0.0;
    rj["win_rate"] = wr;
    rj["revoked"] = r.revoked;
    rj["token_deleted"] = r.tokenDeleted;
    bool retired = (r.revoked || r.tokenDeleted);
    rj["retired"] = retired;
    rj["status"] = (!retired) ? "active" : "retired";
    rj["created_at"] = r.createdAt;
    rj["last_seen_at"] = 0;
    rj["robots"] = Json::array();

    ArchiveFilter af;
    af.robotId = r.robotId;
    af.robotUuid = r.robotUuid;
    af.source = "online";
    af.limit = 50;
    af.offset = 0;
    std::vector<GameRecord> games;
    store.listArchivedGames(af, games, nullptr);

    Json gamesArr = Json::array();
    for (size_t i = 0; i < games.size(); ++i) {
        const auto &g = games[i];
        Json gj = gameSummaryJson(g);
        gj["p1_handle"] = g.p1Handle;
        gj["p2_handle"] = g.p2Handle;
        gj["p3_handle"] = g.p3Handle;
        gj["p4_handle"] = g.p4Handle;
        if (gj.contains("players") && gj["players"].is_array()) {
            for (const auto &pl : gj["players"]) {
                int seat = pl.value("seat", 0);
                std::string h = pl.value("handle", "");
                if (seat == 1) gj["p1_handle"] = h;
                else if (seat == 2) gj["p2_handle"] = h;
                else if (seat == 3) gj["p3_handle"] = h;
                else if (seat == 4) gj["p4_handle"] = h;
            }
        }

        int mySeat = 0;
        if (r.robotId > 0) {
            if (g.p1RobotId == r.robotId) mySeat = 1;
            else if (g.p2RobotId == r.robotId) mySeat = 2;
            else if (g.p3RobotId == r.robotId) mySeat = 3;
            else if (g.p4RobotId == r.robotId) mySeat = 4;
        }
        if (mySeat == 0 && !r.robotName.empty()) {
            std::string rn = lower(r.robotName);
            if (lower(g.p1Handle) == rn) mySeat = 1;
            else if (lower(g.p2Handle) == rn) mySeat = 2;
            else if (lower(g.p3Handle) == rn) mySeat = 3;
            else if (lower(g.p4Handle) == rn) mySeat = 4;
        }

        std::string outcome = "DRAW";
        if (g.result == "DRAW" || (g.winnerSeat == 0 && g.winner == "NONE")) {
            outcome = "DRAW";
        } else if (g.result == "TEAM_13_WIN") {
            outcome = (mySeat == 1 || mySeat == 3) ? "WIN" : "LOSS";
        } else if (g.result == "TEAM_24_WIN") {
            outcome = (mySeat == 2 || mySeat == 4) ? "WIN" : "LOSS";
        } else if (g.winnerSeat > 0) {
            outcome = (mySeat == g.winnerSeat) ? "WIN" : "LOSS";
        } else if (g.winner == "A") {
            outcome = (mySeat == 1) ? "WIN" : "LOSS";
        } else if (g.winner == "B") {
            outcome = (mySeat == 2) ? "WIN" : "LOSS";
        } else if (g.winner == "C") {
            outcome = (mySeat == 3) ? "WIN" : "LOSS";
        } else if (g.winner == "D") {
            outcome = (mySeat == 4) ? "WIN" : "LOSS";
        }
        gj["player_outcome"] = outcome;
        gamesArr.push_back(gj);
    }
    rj["recent_games"] = gamesArr;
    return rj;
}

void OctiHttpServer::Impl::handleUserGet(const Request &req, Response &res)
{
    Auth auth;
    if (!optionalAuth(req, res, auth) || !allowBrowseRead(req, res, auth)) {
        return;
    }
    std::string id;
    std::unordered_map<std::string, std::string>::const_iterator it =
        req.path_params.find("id");
    if (it == req.path_params.end()) {
        it = req.path_params.find("uuid");
    }
    if (it == req.path_params.end() || !isSafeId(it->second)) {
        sendError(res, 404, "NOT_FOUND", "no such resource");
        return;
    }
    id = it->second;

    PlayerProfile prof;
    bool found = store.getProfileByUuid(id, prof);
    if (!found) {
        std::string h = id;
        if (!h.empty() && h[0] == '@') {
            h = h.substr(1);
        }
        found = store.getProfileByHandle(h, prof);
    }
    if (!found) {
        RobotRecord robotRec;
        bool rFound = store.getRobotByUuid(id, robotRec);
        if (!rFound) {
            std::string h = id;
            if (!h.empty() && h[0] == '@') {
                h = h.substr(1);
            }
            rFound = store.getRobotByName(h, robotRec);
        }
        if (rFound && robotRec.gamesPlayed > 0) {
            if (auth.playerId == 0) {
                applyBanSideEffects(banManager.recordAnonymousEnumeration(
                    store, clientRemoteIp(req), req.path, 200,
                    store.currentEpochSec()));
            }
            sendJson(res, 200, buildRobotProfilePayload(robotRec));
            return;
        }
    }
    if (!found || prof.banned) {
        if (auth.playerId == 0) {
            applyBanSideEffects(banManager.recordAnonymousEnumeration(
                store, clientRemoteIp(req), req.path, 404,
                store.currentEpochSec()));
        }
        sendError(res, 404, "NOT_FOUND", "no such user");
        return;
    }
    if (auth.playerId == 0) {
        applyBanSideEffects(banManager.recordAnonymousEnumeration(
            store, clientRemoteIp(req), req.path, 200,
            store.currentEpochSec()));
    }

    std::vector<RobotRecord> robots;
    store.listPublicRobotsByOwner(prof.playerId, robots);
    Json robotsArr = Json::array();
    for (size_t i = 0; i < robots.size(); ++i) {
        const auto &r = robots[i];
        Json rj;
        rj["robot_uuid"] = r.robotUuid;
        rj["robot_name"] = r.robotName;
        rj["description"] = r.description;
        rj["robot_url"] = r.robotUrl;
        rj["name_changed_at"] = r.nameChangedAt;
        rj["elo"] = r.elo2p3b;
        rj["elo_2p"] = r.elo2p3b;
        rj["elo_2p_3b"] = r.elo2p3b;
        rj["elo_2p_1b"] = r.elo2p1b;
        rj["elo_ofk"] = r.eloOfk;
        rj["games_2p_3b"] = r.games2p3b;
        rj["games_2p_1b"] = r.games2p1b;
        rj["games_ofk"] = r.gamesOfk;
        rj["games_played"] = r.gamesPlayed;
        rj["wins"] = r.wins;
        rj["losses"] = r.losses;
        rj["draws"] = r.draws;
        rj["created_at"] = r.createdAt;
        robotsArr.push_back(rj);
    }

    ArchiveFilter af;
    af.playerUuid = prof.uuid;
    af.limit = 50;
    af.offset = 0;
    std::vector<GameRecord> games;
    store.listArchivedGames(af, games, nullptr);
    Json gamesArr = Json::array();
    for (size_t i = 0; i < games.size(); ++i) {
        const auto &g = games[i];
        Json gj = gameSummaryJson(g);
        gj["p1_handle"] = g.p1Handle;
        gj["p2_handle"] = g.p2Handle;
        gj["p3_handle"] = g.p3Handle;
        gj["p4_handle"] = g.p4Handle;
        if (gj.contains("players") && gj["players"].is_array()) {
            for (const auto &pl : gj["players"]) {
                int seat = pl.value("seat", 0);
                std::string h = pl.value("handle", "");
                if (seat == 1) gj["p1_handle"] = h;
                else if (seat == 2) gj["p2_handle"] = h;
                else if (seat == 3) gj["p3_handle"] = h;
                else if (seat == 4) gj["p4_handle"] = h;
            }
        }

        int mySeat = 0;
        if (prof.playerId > 0 && prof.playerId == g.p1PlayerId) mySeat = 1;
        else if (prof.playerId > 0 && prof.playerId == g.p2PlayerId) mySeat = 2;
        else if (prof.playerId > 0 && prof.playerId == g.p3PlayerId) mySeat = 3;
        else if (prof.playerId > 0 && prof.playerId == g.p4PlayerId) mySeat = 4;
        else if (!prof.handle.empty() && prof.handle == g.p1Handle) mySeat = 1;
        else if (!prof.handle.empty() && prof.handle == g.p2Handle) mySeat = 2;
        else if (!prof.handle.empty() && prof.handle == g.p3Handle) mySeat = 3;
        else if (!prof.handle.empty() && prof.handle == g.p4Handle) mySeat = 4;

        std::string outcome = "DRAW";
        if (g.result == "DRAW" || (g.winnerSeat == 0 && g.winner == "NONE")) {
            outcome = "DRAW";
        } else if (g.result == "TEAM_13_WIN") {
            outcome = (mySeat == 1 || mySeat == 3) ? "WIN" : "LOSS";
        } else if (g.result == "TEAM_24_WIN") {
            outcome = (mySeat == 2 || mySeat == 4) ? "WIN" : "LOSS";
        } else if (g.winnerSeat > 0) {
            outcome = (mySeat == g.winnerSeat) ? "WIN" : "LOSS";
        } else if (g.winner == "A") {
            outcome = (mySeat == 1) ? "WIN" : "LOSS";
        } else if (g.winner == "B") {
            outcome = (mySeat == 2) ? "WIN" : "LOSS";
        } else if (g.winner == "C") {
            outcome = (mySeat == 3) ? "WIN" : "LOSS";
        } else if (g.winner == "D") {
            outcome = (mySeat == 4) ? "WIN" : "LOSS";
        }
        gj["player_outcome"] = outcome;
        gamesArr.push_back(gj);
    }

    Json j = profileJson(prof, true);
    j["profile"] = profileJson(prof, true);
    j["robots"] = robotsArr;
    j["recent_games"] = gamesArr;
    sendJson(res, 200, j);
}

void OctiHttpServer::Impl::handleRobotsDirectory(const Request &req, Response &res)
{
    Auth auth;
    if (!optionalAuth(req, res, auth) || !allowBrowseRead(req, res, auth)) {
        return;
    }
    ArenaStandingsFilter filter;
    filter.participantType = "all";
    if (req.has_param("participant_type")) {
        std::string pt = lower(req.get_param_value("participant_type"));
        if (pt != "all" && pt != "robots" && pt != "humans") {
            sendError(res, 400, "INVALID_ARGUMENT",
                      "participant_type must be all, robots or humans");
            return;
        }
        filter.participantType = pt;
    }
    if (req.has_param("q")) {
        filter.query = req.get_param_value("q");
    }
    if (req.has_param("sort")) {
        filter.sortField = req.get_param_value("sort");
    } else {
        filter.sortField = "elo_2p_3b";
    }
    if (req.has_param("order")) {
        std::string ord = lower(req.get_param_value("order"));
        filter.sortAsc = (ord == "asc");
    } else {
        filter.sortAsc = false;
    }
    int limit = 50;
    if (req.has_param("limit")) {
        try {
            limit = std::stoi(req.get_param_value("limit"));
        } catch (...) {
            limit = 50;
        }
    }
    if (limit < 1) limit = 1;
    if (limit > 100) limit = 100;
    filter.limit = limit;

    int offset = 0;
    if (req.has_param("offset")) {
        try {
            offset = std::stoi(req.get_param_value("offset"));
        } catch (...) {
            offset = 0;
        }
    }
    if (offset < 0) offset = 0;
    filter.offset = offset;

    const std::string cacheKey =
        filter.participantType + "|" + filter.query + "|" + filter.sortField +
        "|" + (filter.sortAsc ? "1" : "0") + "|" + std::to_string(limit) +
        "|" + std::to_string(offset);
    const uint64_t storeRev = store.mutationRevision();
    const int64_t nowMs = steadyMs();
    CachedPayload cached;
    if (robotsDirCache.get(cacheKey, storeRev, nowMs, cached)) {
        sendCachedSnapshot(req, res, cached, "no-store");
        return;
    }

    std::vector<ArenaStandingEntry> list;
    int64_t total = 0;
    if (!store.listArenaStandings(filter, list, &total)) {
        sendError(res, 500, "INTERNAL_ERROR", "failed to list arena directory");
        return;
    }

    Json arr = Json::array();
    for (size_t i = 0; i < list.size(); ++i) {
        const auto &e = list[i];
        const bool isRobot = (e.entryType == "robot");
        Json rj;
        rj["entry_type"] = e.entryType;
        rj["is_robot"] = isRobot;
        rj["uuid"] = e.uuid;
        rj["robot_uuid"] = isRobot ? e.robotUuid : "";
        rj["user_uuid"] = isRobot ? "" : e.uuid;
        rj["handle"] = e.name;
        rj["name"] = e.name;
        rj["robot_name"] = isRobot ? e.name : "";
        rj["display_name"] = e.displayName;
        rj["avatar_url"] = e.avatarUrl;
        rj["description"] = e.description;
        rj["bio"] = e.description;
        rj["robot_url"] = e.robotUrl;
        rj["name_changed_at"] = e.nameChangedAt;
        rj["elo"] = e.elo2p3b;
        rj["elo_2p"] = e.elo2p3b;
        rj["elo_2p_3b"] = e.elo2p3b;
        rj["elo_2p_1b"] = e.elo2p1b;
        rj["elo_ofk"] = e.eloOfk;
        rj["games_2p_3b"] = e.games2p3b;
        rj["games_2p_1b"] = e.games2p1b;
        rj["games_ofk"] = e.gamesOfk;
        rj["games_played"] = e.gamesPlayed;
        rj["wins"] = e.wins;
        rj["losses"] = e.losses;
        rj["draws"] = e.draws;
        rj["win_rate"] = e.winRate;
        rj["revoked"] = e.revoked;
        rj["token_deleted"] = e.tokenDeleted;
        bool retired = isRobot && (e.revoked || e.tokenDeleted);
        rj["retired"] = retired;
        rj["status"] = (!retired) ? "active" : "retired";
        rj["created_at"] = e.createdAt;
        rj["last_seen_at"] = e.lastSeenAt;
        rj["owner_uuid"] = e.ownerUuid;
        rj["owner_handle"] = e.ownerHandle;
        rj["owner_avatar_url"] = e.ownerAvatarUrl;
        arr.push_back(rj);
    }

    Json resp;
    resp["participant_type"] = filter.participantType;
    resp["total"] = total;
    resp["limit"] = limit;
    resp["offset"] = offset;
    resp["entries"] = arr;
    resp["robots"] = arr;
    CachedPayload stored = robotsDirCache.put(
        cacheKey, storeRev, 2000, nowMs, "rdir-", dumpJson(resp));
    sendCachedSnapshot(req, res, stored, "no-store");
}

void OctiHttpServer::Impl::handleRobotProfileGet(const Request &req, Response &res)
{
    Auth auth;
    if (!optionalAuth(req, res, auth) || !allowBrowseRead(req, res, auth)) {
        return;
    }
    std::string id;
    std::unordered_map<std::string, std::string>::const_iterator it =
        req.path_params.find("id");
    if (it == req.path_params.end()) {
        it = req.path_params.find("uuid");
    }
    if (it == req.path_params.end() || !isSafeId(it->second)) {
        sendError(res, 404, "ERR_NOT_FOUND", "robot profile not found");
        return;
    }
    id = it->second;

    RobotRecord robotRec;
    bool found = store.getRobotByUuid(id, robotRec);
    if (!found) {
        std::string h = id;
        if (!h.empty() && h[0] == '@') {
            h = h.substr(1);
        }
        found = store.getRobotByName(h, robotRec);
    }
    if (!found || robotRec.gamesPlayed <= 0) {
        sendError(res, 404, "ERR_NOT_FOUND", "robot profile not found");
        return;
    }

    sendJson(res, 200, buildRobotProfilePayload(robotRec));
}

void OctiHttpServer::Impl::handleRobotsList(const Request &req, Response &res)
{
    res.set_header("Cache-Control", "no-store");
    Auth auth;
    if (!requireAuth(req, res, auth)) {
        return;
    }
    std::vector<RobotRecord> robots;
    if (!store.listRobots(auth.playerId, robots)) {
        sendError(res, 500, "INTERNAL_ERROR", "failed to list robots");
        return;
    }
    int activeCount = 0;
    Json arr = Json::array();
    for (size_t i = 0; i < robots.size(); ++i) {
        const auto &r = robots[i];
        if (!r.revoked) {
            ++activeCount;
        }
        Json rj;
        rj["robot_uuid"] = r.robotUuid;
        rj["robot_name"] = r.robotName;
        rj["description"] = r.description;
        rj["robot_url"] = r.robotUrl;
        rj["name_changed_at"] = r.nameChangedAt;
        rj["owner_uuid"] = r.ownerUuid;
        rj["owner_handle"] = r.ownerHandle;
        rj["robot_token"] = r.token;
        rj["token_prefix"] = r.tokenPrefix;
        rj["created_at"] = r.createdAt;
        rj["elo"] = r.elo2p3b;
        rj["elo_2p"] = r.elo2p3b;
        rj["elo_2p_3b"] = r.elo2p3b;
        rj["elo_2p_1b"] = r.elo2p1b;
        rj["elo_ofk"] = r.eloOfk;
        rj["games_2p_3b"] = r.games2p3b;
        rj["games_2p_1b"] = r.games2p1b;
        rj["games_ofk"] = r.gamesOfk;
        rj["games_played"] = r.gamesPlayed;
        rj["wins"] = r.wins;
        rj["losses"] = r.losses;
        rj["draws"] = r.draws;
        rj["revoked"] = r.revoked;
        rj["revoked_at"] = r.revokedAt;
        arr.push_back(rj);
    }
    Json j;
    j["max_robots"] = kMaxRobotsPerPlayer;
    j["active_count"] = activeCount;
    j["robots"] = arr;
    sendJson(res, 200, j);
}

void OctiHttpServer::Impl::handleRobotCreate(const Request &req, Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth) ||
        !allowMutationAttempt(req, res, auth.playerId)) {
        return;
    }
    Json body;
    if (!readJsonBody(req, res, body, false)) {
        return;
    }
    std::string name;
    std::string err;
    if (fieldPresent(body, "robot_name")) {
        fieldString(body, "robot_name", false, name, err);
    } else if (fieldPresent(body, "name")) {
        fieldString(body, "name", false, name, err);
    }
    if (name.empty()) {
        sendError(res, 400, "INVALID_ROBOT_NAME", "missing robot_name");
        return;
    }
    std::string desc;
    fieldString(body, "description", false, desc, err);

    std::string robotUrl;
    if (fieldPresent(body, "robot_url")) {
        fieldString(body, "robot_url", false, robotUrl, err);
        while (!robotUrl.empty() && std::isspace(static_cast<unsigned char>(robotUrl.front()))) {
            robotUrl.erase(robotUrl.begin());
        }
        while (!robotUrl.empty() && std::isspace(static_cast<unsigned char>(robotUrl.back()))) {
            robotUrl.pop_back();
        }
        if (!robotUrl.empty()) {
            if (!AccountStore::validRobotUrlSyntax(robotUrl)) {
                sendError(res, 400, "INVALID_ROBOT_URL", "invalid robot URL");
                return;
            }
            if (!allowProbeAttempt(req, res, auth.playerId)) {
                return;
            }
            UrlProbeSlotGuard probeSlot(activeUrlProbes, cfg.maxConcurrentUrlProbes);
            if (!probeSlot.allowed()) {
                sendError(res, 503, "SERVICE_BUSY",
                          "too many concurrent robot URL verification probes; retry later");
                res.set_header("Retry-After", "3");
                return;
            }
            std::string reachErr;
            bool disallowedTarget = false;
            if (!verifyRobotUrlReachable(robotUrl, reachErr, &disallowedTarget)) {
                if (disallowedTarget) {
                    applyBanSideEffects(banManager.recordExploitProbe(
                        store, buildAbuseContext(req, auth.playerId),
                        store.currentEpochSec()));
                }
                sendError(res, 400, "INVALID_ROBOT_URL", reachErr);
                return;
            }
        }
    }

    CreateRobotResult cr = store.createRobot(auth.playerId, name, desc, robotUrl);
    switch (cr.status) {
    case RobotStatus::OK: {
        Json j;
        j["robot_uuid"] = cr.robot.robotUuid;
        j["robot_name"] = cr.robot.robotName;
        j["description"] = cr.robot.description;
        j["robot_url"] = cr.robot.robotUrl;
        j["name_changed_at"] = cr.robot.nameChangedAt;
        j["owner_uuid"] = cr.robot.ownerUuid;
        j["owner_handle"] = cr.robot.ownerHandle;
        j["robot_token"] = cr.rawToken;
        j["token_prefix"] = cr.robot.tokenPrefix;
        j["created_at"] = cr.robot.createdAt;
        j["elo"] = cr.robot.elo2p3b;
        j["elo_2p"] = cr.robot.elo2p3b;
        j["elo_2p_3b"] = cr.robot.elo2p3b;
        j["elo_2p_1b"] = cr.robot.elo2p1b;
        j["elo_ofk"] = cr.robot.eloOfk;
        j["games_2p_3b"] = cr.robot.games2p3b;
        j["games_2p_1b"] = cr.robot.games2p1b;
        j["games_ofk"] = cr.robot.gamesOfk;
        j["revoked"] = cr.robot.revoked;
        sendJson(res, 201, j);
        return;
    }
    case RobotStatus::INVALID_ROBOT_NAME:
        sendError(res, 400, "INVALID_ROBOT_NAME",
                  cr.errorMessage.empty() ? "invalid robot name" : cr.errorMessage);
        return;
    case RobotStatus::INVALID_ROBOT_DESCRIPTION:
        sendError(res, 400, "INVALID_ROBOT_DESCRIPTION",
                  cr.errorMessage.empty() ? "invalid robot description" : cr.errorMessage);
        return;
    case RobotStatus::INVALID_ROBOT_URL:
        sendError(res, 400, "INVALID_ROBOT_URL",
                  cr.errorMessage.empty() ? "invalid robot URL" : cr.errorMessage);
        return;
    case RobotStatus::ROBOT_NAME_TAKEN:
        sendError(res, 409, "ROBOT_NAME_TAKEN", "robot name already taken");
        return;
    case RobotStatus::ROBOT_LIMIT_REACHED:
        sendError(res, 409, "ROBOT_LIMIT_REACHED",
                  "robot limit reached (max " + std::to_string(kMaxRobotsPerPlayer) + ")");
        return;
    default:
        break;
    }
    sendError(res, 500, "INTERNAL_ERROR", "failed to create robot");
}

void OctiHttpServer::Impl::handleRobotPatch(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!requireAuth(req, res, auth) || !pathId(req, "id", res, id) ||
        !allowMutationAttempt(req, res, auth.playerId)) {
        return;
    }
    Json body;
    if (!readJsonBody(req, res, body, false)) {
        return;
    }

    bool hasDesc = false;
    std::string desc;
    std::string err;
    if (fieldPresent(body, "description")) {
        hasDesc = true;
        if (!fieldString(body, "description", false, desc, err)) {
            sendError(res, 400, "INVALID_ARGUMENT", err);
            return;
        }
    }

    bool hasName = false;
    std::string name;
    if (fieldPresent(body, "robot_name")) {
        hasName = true;
        if (!fieldString(body, "robot_name", false, name, err)) {
            sendError(res, 400, "INVALID_ARGUMENT", err);
            return;
        }
    } else if (fieldPresent(body, "name")) {
        hasName = true;
        if (!fieldString(body, "name", false, name, err)) {
            sendError(res, 400, "INVALID_ARGUMENT", err);
            return;
        }
    }

    bool hasUrl = false;
    std::string robotUrl;
    if (fieldPresent(body, "robot_url")) {
        hasUrl = true;
        if (!fieldString(body, "robot_url", false, robotUrl, err)) {
            sendError(res, 400, "INVALID_ARGUMENT", err);
            return;
        }
        while (!robotUrl.empty() && std::isspace(static_cast<unsigned char>(robotUrl.front()))) {
            robotUrl.erase(robotUrl.begin());
        }
        while (!robotUrl.empty() && std::isspace(static_cast<unsigned char>(robotUrl.back()))) {
            robotUrl.pop_back();
        }
        if (!robotUrl.empty()) {
            if (!AccountStore::validRobotUrlSyntax(robotUrl)) {
                sendError(res, 400, "INVALID_ROBOT_URL", "invalid robot URL");
                return;
            }
            if (!allowProbeAttempt(req, res, auth.playerId)) {
                return;
            }
            UrlProbeSlotGuard probeSlot(activeUrlProbes, cfg.maxConcurrentUrlProbes);
            if (!probeSlot.allowed()) {
                sendError(res, 503, "SERVICE_BUSY",
                          "too many concurrent robot URL verification probes; retry later");
                res.set_header("Retry-After", "3");
                return;
            }
            std::string reachErr;
            bool disallowedTarget = false;
            if (!verifyRobotUrlReachable(robotUrl, reachErr, &disallowedTarget)) {
                if (disallowedTarget) {
                    applyBanSideEffects(banManager.recordExploitProbe(
                        store, buildAbuseContext(req, auth.playerId),
                        store.currentEpochSec()));
                }
                sendError(res, 400, "INVALID_ROBOT_URL", reachErr);
                return;
            }
        }
    }

    if (!hasDesc && !hasName && !hasUrl) {
        sendError(res, 400, "INVALID_ARGUMENT", "no update fields provided");
        return;
    }

    RobotRecord updated;
    RobotStatus st = store.updateRobot(auth.playerId, id, hasName, name, hasDesc, desc, hasUrl, robotUrl, &updated);
    switch (st) {
    case RobotStatus::OK: {
        rooms.refreshCachedPlayerStats();
        Json j;
        j["updated"] = true;
        j["robot_uuid"] = updated.robotUuid;
        j["robot_name"] = updated.robotName;
        j["description"] = updated.description;
        j["robot_url"] = updated.robotUrl;
        j["name_changed_at"] = updated.nameChangedAt;
        sendJson(res, 200, j);
        return;
    }
    case RobotStatus::INVALID_ROBOT_NAME:
        sendError(res, 400, "INVALID_ROBOT_NAME", "invalid robot name");
        return;
    case RobotStatus::INVALID_ROBOT_DESCRIPTION:
        sendError(res, 400, "INVALID_ROBOT_DESCRIPTION", "invalid robot description");
        return;
    case RobotStatus::INVALID_ROBOT_URL:
        sendError(res, 400, "INVALID_ROBOT_URL", "invalid robot URL");
        return;
    case RobotStatus::ROBOT_NAME_TAKEN:
        sendError(res, 409, "ROBOT_NAME_TAKEN", "robot name already taken");
        return;
    case RobotStatus::ROBOT_NAME_RATE_LIMITED:
        noteRateLimit429(req, auth.playerId);
        sendError(res, 429, "ROBOT_NAME_RATE_LIMITED", "Robot name can only be changed once every 24 hours.");
        return;
    case RobotStatus::NOT_FOUND:
        sendError(res, 404, "NOT_FOUND", "robot not found");
        return;
    case RobotStatus::ALREADY_REVOKED:
        sendError(res, 409, "ALREADY_REVOKED", "robot is revoked");
        return;
    default:
        break;
    }
    sendError(res, 500, "INTERNAL_ERROR", "failed to update robot");
}

void OctiHttpServer::Impl::handleRobotDelete(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!requireAuth(req, res, auth) || !pathId(req, "id", res, id) ||
        !allowMutationAttempt(req, res, auth.playerId)) {
        return;
    }
    bool permanent = req.has_param("permanent") &&
                     (req.get_param_value("permanent") == "1" ||
                      req.get_param_value("permanent") == "true");
    if (!permanent && req.has_param("delete") &&
        (req.get_param_value("delete") == "1" ||
         req.get_param_value("delete") == "true")) {
        permanent = true;
    }

    if (permanent) {
        RobotStatus st = store.deleteRobot(auth.playerId, id);
        switch (st) {
        case RobotStatus::OK: {
            Json j;
            j["ok"] = true;
            j["deleted"] = true;
            j["id"] = id;
            j["robot_uuid"] = id;
            sendJson(res, 200, j);
            return;
        }
        case RobotStatus::NOT_FOUND:
            sendError(res, 404, "NOT_FOUND", "robot not found");
            return;
        default:
            break;
        }
        sendError(res, 500, "INTERNAL_ERROR", "failed to delete robot");
        return;
    }

    RobotStatus st = store.revokeRobot(auth.playerId, id);
    switch (st) {
    case RobotStatus::OK: {
        Json j;
        j["revoked"] = true;
        j["id"] = id;
        j["robot_uuid"] = id;
        sendJson(res, 200, j);
        return;
    }
    case RobotStatus::NOT_FOUND:
        sendError(res, 404, "NOT_FOUND", "robot not found");
        return;
    case RobotStatus::ALREADY_REVOKED:
        sendError(res, 409, "ALREADY_REVOKED", "robot is already revoked");
        return;
    default:
        break;
    }
    sendError(res, 500, "INTERNAL_ERROR", "failed to revoke robot");
}

void OctiHttpServer::Impl::handleRobotsConnect(const Request &req, Response &res)
{
    std::string token;
    Json body;
    bool bodyRead = readJsonBody(req, res, body, true);
    if (!bodyRead) {
        return;
    }
    if (!extractBearer(req, token)) {
        std::string err;
        if (fieldPresent(body, "robot_token")) {
            fieldString(body, "robot_token", false, token, err);
        } else if (fieldPresent(body, "token")) {
            fieldString(body, "token", false, token, err);
        }
    }
    if (token.empty()) {
        sendAuthUnauthorized(req, res, "Bearer", "missing robot token",
                             req.has_header("Authorization"));
        return;
    }
    const int64_t nowSec = store.currentEpochSec();
    RobotTokenValidation val = store.validateRobotToken(token);
    if (val.banned) {
        BanMatch bm;
        const BanMatch *bmp =
            (!val.owner.uuid.empty() &&
             banManager.matchAccount(val.owner.uuid, nowSec, bm))
                ? &bm
                : nullptr;
        sendAccountBanned(res, bmp, nowSec);
        return;
    }
    if (!val.valid || val.robot.revoked) {
        sendAuthUnauthorized(req, res, "Bearer error=\"invalid_token\"",
                             "invalid or revoked robot token", true);
        return;
    }
    {
        BanMatch bm;
        if (banManager.matchRobot(val.robot.robotUuid, nowSec, bm) ||
            (!val.owner.uuid.empty() &&
             banManager.matchAccount(val.owner.uuid, nowSec, bm))) {
            sendAccountBanned(res, &bm, nowSec);
            return;
        }
    }
    if (val.owner.isAdmin) {
        banManager.noteVerifiedAdminIp(clientRemoteIp(req), nowSec);
    }
    RobotRecord robotRec = val.robot;

    bool ifUnused = false;
    std::string err;
    fieldBool(body, "if_unused", false, ifUnused, err);

    int64_t actorId = makeRobotActorId(robotRec.robotId);
    if (!allowMutationAttempt(req, res, actorId)) {
        return;
    }
    const int64_t now = steadyMs();

    std::string leaseId;
    {
        std::lock_guard<std::mutex> lock(_robotLeaseMutex);
        auto it = _activeRobotLeases.find(robotRec.robotId);
        if (ifUnused && it != _activeRobotLeases.end() && !it->second.leaseId.empty()) {
            if (it->second.activeStreams > 0 || (now - it->second.lastSeenMs < 30000)) {
                sendError(res, 409, "ROBOT_TOKEN_IN_USE",
                          "robot token is currently active on another connection");
                return;
            }
        }
        leaseId = generateLeaseId();
        ActiveRobotLease &lease = _activeRobotLeases[robotRec.robotId];
        lease.leaseId = leaseId;
        lease.connectedAtMs = now;
        lease.lastSeenMs = now;
        lease.clientIp = clientRemoteIp(req);
        lease.clientType = "robot";
    }

    rooms.enterLobby(actorId);
    rooms.recordClientHeartbeat(actorId, "robot", true);

    std::string activeRoomId;
    bool hasActiveRoom = rooms.findActiveRoomForActor(actorId, activeRoomId);

    Json j;
    j["ok"] = true;
    j["lease_id"] = leaseId;
    j["robot_id"] = robotRec.robotId;
    j["robot_uuid"] = robotRec.robotUuid;
    j["robot_name"] = robotRec.robotName;
    j["description"] = robotRec.description;
    j["owner_uuid"] = robotRec.ownerUuid;
    j["owner_handle"] = robotRec.ownerHandle;
    j["owner_avatar_url"] = robotRec.ownerAvatarUrl;
    j["active_room_id"] = hasActiveRoom ? activeRoomId : "";
    j["in_game"] = hasActiveRoom;
    sendJson(res, 200, j);
}

void OctiHttpServer::Impl::handleRobotsDisconnect(const Request &req, Response &res)
{
    std::string token;
    Json body;
    bool bodyRead = readJsonBody(req, res, body, true);
    if (!bodyRead) {
        return;
    }
    if (!extractBearer(req, token)) {
        std::string err;
        if (fieldPresent(body, "robot_token")) {
            fieldString(body, "robot_token", false, token, err);
        } else if (fieldPresent(body, "token")) {
            fieldString(body, "token", false, token, err);
        }
    }
    if (token.empty()) {
        sendAuthUnauthorized(req, res, "Bearer", "missing robot token",
                             req.has_header("Authorization"));
        return;
    }
    const int64_t nowSec = store.currentEpochSec();
    RobotTokenValidation val = store.validateRobotToken(token);
    if (val.banned) {
        BanMatch bm;
        const BanMatch *bmp =
            (!val.owner.uuid.empty() &&
             banManager.matchAccount(val.owner.uuid, nowSec, bm))
                ? &bm
                : nullptr;
        sendAccountBanned(res, bmp, nowSec);
        return;
    }
    if (!val.valid || val.robot.revoked) {
        sendAuthUnauthorized(req, res, "Bearer error=\"invalid_token\"",
                             "invalid or revoked robot token", true);
        return;
    }
    {
        BanMatch bm;
        if (banManager.matchRobot(val.robot.robotUuid, nowSec, bm) ||
            (!val.owner.uuid.empty() &&
             banManager.matchAccount(val.owner.uuid, nowSec, bm))) {
            sendAccountBanned(res, &bm, nowSec);
            return;
        }
    }
    if (val.owner.isAdmin) {
        banManager.noteVerifiedAdminIp(clientRemoteIp(req), nowSec);
    }
    RobotRecord robotRec = val.robot;
    int64_t actorId = makeRobotActorId(robotRec.robotId);
    if (!allowMutationAttempt(req, res, actorId)) {
        return;
    }

    std::string leaseHeader;
    if (req.has_header("X-Octi-Robot-Lease")) {
        leaseHeader = req.get_header_value("X-Octi-Robot-Lease");
    } else if (fieldPresent(body, "lease_id")) {
        std::string err;
        fieldString(body, "lease_id", false, leaseHeader, err);
    }

    {
        std::lock_guard<std::mutex> lock(_robotLeaseMutex);
        auto it = _activeRobotLeases.find(robotRec.robotId);
        if (it != _activeRobotLeases.end()) {
            if (leaseHeader.empty() || it->second.leaseId == leaseHeader) {
                _activeRobotLeases.erase(it);
            }
        }
    }

    rooms.clearClientPresence(actorId, "robot");
    rooms.leaveLobby(actorId);

    Json j;
    j["ok"] = true;
    sendJson(res, 200, j);
}

void OctiHttpServer::Impl::handleRobotsExportConf(const Request &req, Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth)) {
        return;
    }
    if (auth.isRobot || auth.playerId <= 0) {
        sendError(res, 403, "FORBIDDEN_ROBOT", "robots cannot export token configs");
        return;
    }
    if (!allowRobotExportConfAttempt(req, res, auth.playerId)) {
        return;
    }
    PlayerProfile prof;
    if (!store.getProfileById(auth.playerId, prof)) {
        sendError(res, 401, "UNAUTHORIZED", "unknown player");
        return;
    }
    std::vector<RobotRecord> robots;
    if (!store.listRobots(auth.playerId, robots)) {
        sendError(res, 500, "INTERNAL_ERROR", "failed to list robots");
        return;
    }
    std::ostringstream ss;
    ss << "# octi tokens.conf\n";
    ss << "# Owner: " << prof.handle << " (" << prof.uuid << ")\n";
    ss << "# Format: <robot_name>=<robot_token>\n\n";
    for (size_t i = 0; i < robots.size(); ++i) {
        const auto &r = robots[i];
        if (!r.revoked && !r.token.empty()) {
            ss << r.robotName << "=" << r.token << "\n";
        }
    }
    std::string conf = ss.str();
    res.status = 200;
    res.set_header("Cache-Control", "no-store");
    res.set_header("Content-Disposition", "attachment; filename=\"tokens.conf\"");
    res.set_content(conf, "text/plain; charset=utf-8");
}

void OctiHttpServer::Impl::handleLeaderboard(const Request &req,
                                             Response &res)
{
    Auth auth;
    if (!optionalAuth(req, res, auth) || !allowBrowseRead(req, res, auth)) {
        return;
    }
    std::string rawMode = "2P_3B";
    if (req.has_param("ladder")) {
        rawMode = req.get_param_value("ladder");
    } else if (req.has_param("mode")) {
        rawMode = req.get_param_value("mode");
    }
    const std::string modeUpper = upper(rawMode);
    if (modeUpper != "OVERALL" && modeUpper != "2P" &&
        modeUpper != "2P_3B" && modeUpper != "2P_1B" &&
        modeUpper != "OFK") {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "mode must be overall, 2P_3B, 2P_1B or OFK");
        return;
    }
    const bool isOverall = (modeUpper == "OVERALL");
    const std::string mode = isOverall ? "overall" : modeUpper;
    const std::string ladder =
        isOverall ? "OVERALL" : ((modeUpper == "2P") ? "2P_3B" : modeUpper);

    std::string pool = "human";
    if (req.has_param("pool")) {
        pool = lower(req.get_param_value("pool"));
    }
    if (pool != "human" && pool != "arena") {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "pool must be human or arena");
        return;
    }

    std::string participantType = "all";
    if (req.has_param("participant_type")) {
        participantType = lower(req.get_param_value("participant_type"));
    }
    if (participantType != "all" && participantType != "robots" &&
        participantType != "humans") {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "participant_type must be all, robots or humans");
        return;
    }
    if (pool == "human" && participantType == "robots") {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "participant_type=robots is not supported for pool=human");
        return;
    }

    std::string sortField;
    if (req.has_param("sort") && !req.get_param_value("sort").empty()) {
        sortField = lower(req.get_param_value("sort"));
        if (sortField != "elo_2p_3b" && sortField != "elo_2p_1b" &&
            sortField != "elo_ofk" && sortField != "wins" &&
            sortField != "games_played" && sortField != "win_rate" &&
            sortField != "name" && sortField != "handle" &&
            sortField != "robot_name" && sortField != "created_at") {
            sendError(res, 400, "INVALID_ARGUMENT",
                      "sort must be a valid leaderboard sort field");
            return;
        }
    } else if (ladder == "2P_1B") {
        sortField = "elo_2p_1b";
    } else if (ladder == "OFK") {
        sortField = "elo_ofk";
    } else {
        sortField = "elo_2p_3b";
    }

    bool sortAsc = false;
    if (req.has_param("order") && !req.get_param_value("order").empty()) {
        const std::string ord = lower(req.get_param_value("order"));
        if (ord == "asc") {
            sortAsc = true;
        } else if (ord == "desc") {
            sortAsc = false;
        } else {
            sendError(res, 400, "INVALID_ARGUMENT",
                      "order must be desc or asc");
            return;
        }
    }

    bool playedOnly = isOverall;
    if (req.has_param("played_only") &&
        !req.get_param_value("played_only").empty()) {
        const std::string po = lower(req.get_param_value("played_only"));
        if (po == "1" || po == "true") {
            playedOnly = true;
        } else if (po == "0" || po == "false") {
            playedOnly = isOverall ? true : false;
        } else {
            sendError(res, 400, "INVALID_ARGUMENT",
                      "played_only must be 0, 1, false or true");
            return;
        }
    }

    int64_t limit = 50;
    if (req.has_param("limit") &&
        (!parseUnsignedClamped(req.get_param_value("limit"),
                               AccountStore::kMaxLeaderboardLimit, limit) ||
         limit < 1)) {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "limit must be a positive integer");
        return;
    }

    const std::string lbKey =
        pool + "|" + participantType + "|" + mode + "|" + ladder + "|" +
        sortField + "|" + (sortAsc ? "1" : "0") + "|" +
        (playedOnly ? "1" : "0") + "|" + std::to_string(limit);
    const uint64_t lbRev = store.mutationRevision();
    const int64_t lbNowMs = steadyMs();
    CachedPayload lbCached;
    if (leaderboardCache.get(lbKey, lbRev, lbNowMs, lbCached)) {
        sendCachedSnapshot(req, res, lbCached, "no-cache");
        return;
    }

    int64_t total = 0;
    Json arr = Json::array();
    if (pool == "arena") {
        ArenaStandingsFilter af;
        af.participantType = participantType;
        af.limit = static_cast<int>(limit);
        af.offset = 0;
        af.sortAsc = sortAsc;
        af.sortField = sortField;
        af.ladder = isOverall ? "" : ladder;
        af.playedOnly = playedOnly;
        std::vector<ArenaStandingEntry> arenaRows;
        if (!store.listArenaStandings(af, arenaRows, &total)) {
            sendError(res, 500, "INTERNAL_ERROR", "arena leaderboard query failed");
            return;
        }
        int rankedCount = 0;
        for (size_t i = 0; i < arenaRows.size(); ++i) {
            const ArenaStandingEntry &row = arenaRows[i];
            int ladderGames = row.gamesPlayed;
            int ladderElo = row.elo2p3b;
            if (isOverall) {
                if (sortField == "elo_2p_1b") {
                    ladderElo = row.elo2p1b;
                } else if (sortField == "elo_ofk") {
                    ladderElo = row.eloOfk;
                }
            } else if (ladder == "2P_1B") {
                ladderGames = row.games2p1b;
                ladderElo = row.elo2p1b;
            } else if (ladder == "OFK") {
                ladderGames = row.gamesOfk;
                ladderElo = row.eloOfk;
            } else {
                ladderGames = row.games2p3b;
                ladderElo = row.elo2p3b;
            }
            const bool ranked = (ladderGames > 0);
            const bool isRobot = (row.entryType == "robot");
            const bool retired = isRobot && (row.revoked || row.tokenDeleted);
            Json e;
            e["rank"] = ranked ? (++rankedCount) : 0;
            e["ranked"] = ranked;
            e["entry_type"] = row.entryType;
            e["is_robot"] = isRobot;
            e["uuid"] = row.uuid;
            e["robot_uuid"] = isRobot ? row.robotUuid : "";
            e["user_uuid"] = isRobot ? "" : row.uuid;
            e["handle"] = row.name;
            e["name"] = row.name;
            e["robot_name"] = isRobot ? row.name : "";
            e["display_name"] = row.displayName;
            e["avatar_url"] = row.avatarUrl;
            e["description"] = row.description;
            e["robot_url"] = row.robotUrl;
            e["owner_uuid"] = row.ownerUuid;
            e["owner_handle"] = row.ownerHandle;
            e["owner_avatar_url"] = row.ownerAvatarUrl;
            e["elo"] = ranked ? ladderElo : 1200;
            e["elo_2p_3b"] = row.elo2p3b;
            e["elo_2p_1b"] = row.elo2p1b;
            e["elo_ofk"] = row.eloOfk;
            e["ladder_games"] = ladderGames;
            e["games_2p_3b"] = row.games2p3b;
            e["games_2p_1b"] = row.games2p1b;
            e["games_ofk"] = row.gamesOfk;
            e["wins_2p_3b"] = row.wins2p3b;
            e["losses_2p_3b"] = row.losses2p3b;
            e["draws_2p_3b"] = row.draws2p3b;
            e["wins_2p_1b"] = row.wins2p1b;
            e["losses_2p_1b"] = row.losses2p1b;
            e["draws_2p_1b"] = row.draws2p1b;
            e["wins_ofk"] = row.winsOfk;
            e["losses_ofk"] = row.lossesOfk;
            e["draws_ofk"] = row.drawsOfk;
            e["games_played"] = row.gamesPlayed;
            e["wins"] = row.wins;
            e["losses"] = row.losses;
            e["draws"] = row.draws;
            e["win_rate"] = row.winRate;
            e["revoked"] = row.revoked;
            e["token_deleted"] = row.tokenDeleted;
            e["retired"] = retired;
            e["status"] = (!retired) ? "active" : "retired";
            arr.push_back(e);
        }
    } else {
        std::vector<LeaderboardEntry> rows;
        bool ok = false;
        if (isOverall) {
            ok = store.leaderboardOverall(sortField, sortAsc,
                                          static_cast<int>(limit), rows,
                                          &total);
        } else {
            ok = store.leaderboard(modeUpper, static_cast<int>(limit), rows,
                                   playedOnly, &total);
        }
        if (!ok) {
            sendError(res, 500, "INTERNAL_ERROR", "leaderboard query failed");
            return;
        }
        for (size_t i = 0; i < rows.size(); ++i) {
            const bool ranked = (rows[i].ladderGames > 0);
            Json e;
            e["rank"] = ranked ? rows[i].rank : 0;
            e["ranked"] = ranked;
            e["entry_type"] = "human";
            e["is_robot"] = false;
            e["uuid"] = rows[i].uuid;
            e["user_uuid"] = rows[i].uuid;
            e["handle"] = rows[i].handle;
            e["display_name"] = rows[i].displayName;
            e["avatar_url"] = rows[i].avatarUrl;
            e["elo"] = ranked ? rows[i].elo : 1200;
            e["elo_2p_3b"] = rows[i].elo2p3b;
            e["elo_2p_1b"] = rows[i].elo2p1b;
            e["elo_ofk"] = rows[i].eloOfk;
            e["ladder_games"] = rows[i].ladderGames;
            e["games_2p_3b"] = rows[i].games2p3b;
            e["games_2p_1b"] = rows[i].games2p1b;
            e["games_ofk"] = rows[i].gamesOfk;
            e["games_played"] = rows[i].gamesPlayed;
            e["wins"] = rows[i].wins;
            e["losses"] = rows[i].losses;
            e["draws"] = rows[i].draws;
            e["win_rate"] = rows[i].winRate;
            arr.push_back(e);
        }
    }
    Json j;
    j["pool"] = pool;
    j["participant_type"] = participantType;
    j["mode"] = mode;
    j["ladder"] = ladder;
    j["total"] = total;
    j["entries"] = arr;
    CachedPayload lbStored = leaderboardCache.put(
        lbKey, lbRev, 2000, lbNowMs, "lb-", dumpJson(j));
    sendCachedSnapshot(req, res, lbStored, "no-cache");
}

/* ------------------------------------------------------------------ */
/* Handlers: lobby, tables, matchmaking                                */
/* ------------------------------------------------------------------ */

void OctiHttpServer::Impl::finishRoom(const Request &req, int64_t actorId,
                                      Response &res, const RoomResult &r,
                                      Json body)
{
    if (!r.ok() && r.code == RoomCode::RATE_LIMITED) {
        noteRateLimit429(req, actorId);
    }
    finishRoom(res, r, std::move(body));
}

void OctiHttpServer::Impl::finishRoom(Response &res, const RoomResult &r,
                                      Json body)
{
    if (!r.ok()) {
        sendRoomError(res, r);
        return;
    }
    body["ok"] = true;
    if (r.seq != 0) {
        body["seq"] = r.seq;
    }
    sendJson(res, 200, body);
}

void OctiHttpServer::Impl::handleLobbyState(const Request &req, Response &res)
{
    Auth auth;
    if (!optionalAuth(req, res, auth) || !allowLobbySnap(req, res, auth)) {
        return;
    }
    if (auth.playerId == 0) {
        std::string remoteIp = clientRemoteIp(req);
        if (!isLoopbackIp(remoteIp) && !auth.clientType.empty()) {
            int64_t viewerId = rooms.ensureAnonymousObserver(remoteIp);
            rooms.enterLobby(viewerId);
            rooms.recordClientHeartbeat(viewerId, auth.clientType, false);
        }
    } else {
        rooms.enterLobby(auth.playerId);
        rooms.recordClientHeartbeat(auth.playerId, auth.clientType, false);
    }
    const uint64_t snapRev =
        rooms.lobbySnapshotRevision() ^
        (store.mutationRevision() * 0x9e3779b97f4a7c15ULL);
    const int64_t snapNowMs = rooms.currentClockMs();
    CachedPayload lobbyCached;
    if (lobbyStateCache.get("lobby", snapRev, snapNowMs, lobbyCached)) {
        sendCachedSnapshot(req, res, lobbyCached, "no-cache");
        return;
    }
    LobbySnapshot snap = rooms.lobbyState();
    Json j;
    Json players = Json::array();
    for (size_t i = 0; i < snap.players.size(); ++i) {
        PlayerProfile pProf;
        bool hasHumanProf = false;
        std::string avatarUrl;
        if (snap.players[i].isRobot) {
            avatarUrl = snap.players[i].avatarUrl;
        } else if (snap.players[i].playerId > 0 &&
                   store.getProfileById(snap.players[i].playerId, pProf)) {
            hasHumanProf = true;
            avatarUrl = pProf.avatarUrl;
        }
        Json clientsArr = Json::array();
        for (size_t ci = 0; ci < snap.players[i].clients.size(); ++ci) {
            clientsArr.push_back(snap.players[i].clients[ci]);
        }
        const std::string resolvedPlayerUuid = uuidOf(snap.players[i].playerId);
        Json pj = {
            {"player_uuid", resolvedPlayerUuid},
            {"handle", snap.players[i].handle},
            {"avatar_url", avatarUrl},
            {"status", toString(snap.players[i].status)},
            {"room_id", snap.players[i].roomId},
            {"clients", clientsArr},
            {"online_sec", snap.players[i].onlineSec},
            {"idle_sec", snap.players[i].idleSec},
            {"is_robot", snap.players[i].isRobot}
        };
        if (snap.players[i].isRobot ||
            isRobotActorId(snap.players[i].playerId)) {
            pj["robot_uuid"] = snap.players[i].robotUuid;
            pj["robot_name"] = snap.players[i].handle;
            pj["robot_url"] = "";
            pj["robot_description"] = snap.players[i].robotDescription;
            pj["owner_player_id"] = snap.players[i].ownerPlayerId;
            pj["owner_uuid"] = snap.players[i].ownerUuid;
            pj["owner_handle"] = snap.players[i].ownerHandle;
            pj["elo_2p_3b"] = 1200;
            pj["elo_2p_1b"] = 1200;
            pj["elo_ofk"] = 1200;
            pj["games_2p_3b"] = 0;
            pj["games_2p_1b"] = 0;
            pj["games_ofk"] = 0;
            pj["robot_games_played"] = snap.players[i].robotGamesPlayed;
            pj["games_played"] = snap.players[i].robotGamesPlayed;
            pj["robot_wins"] = snap.players[i].robotWins;
            pj["wins"] = snap.players[i].robotWins;
            pj["robot_losses"] = snap.players[i].robotLosses;
            pj["losses"] = snap.players[i].robotLosses;
            pj["robot_draws"] = snap.players[i].robotDraws;
            pj["draws"] = snap.players[i].robotDraws;
            pj["created_at"] = 0;
            populateActorEntitySummaryJson(snap.players[i].playerId,
                                           snap.players[i].handle,
                                           snap.players[i].robotUuid,
                                           snap.players[i].avatarUrl, pj);
            pj["player_uuid"] = resolvedPlayerUuid;
        } else if (hasHumanProf) {
            populateHumanEntitySummaryJson(pj, pProf);
            pj["player_uuid"] = resolvedPlayerUuid;
            pj["handle"] = snap.players[i].handle;
        } else {
            pj["display_name"] = "";
            pj["bio"] = "";
            pj["location"] = "";
            pj["elo_2p_3b"] = 1200;
            pj["elo_2p_1b"] = 1200;
            pj["elo_ofk"] = 1200;
            pj["games_2p_3b"] = 0;
            pj["games_2p_1b"] = 0;
            pj["games_ofk"] = 0;
            pj["games_played"] = 0;
            pj["wins"] = 0;
            pj["losses"] = 0;
            pj["draws"] = 0;
            pj["arena_elo_2p_3b"] = 1200;
            pj["arena_elo_2p_1b"] = 1200;
            pj["arena_elo_ofk"] = 1200;
            pj["arena_games_2p_3b"] = 0;
            pj["arena_games_2p_1b"] = 0;
            pj["arena_games_ofk"] = 0;
            pj["arena_games_played"] = 0;
            pj["arena_wins"] = 0;
            pj["arena_losses"] = 0;
            pj["arena_draws"] = 0;
            pj["created_at"] = 0;
        }
        players.push_back(pj);
    }
    j["players"] = players;
    Json tables = Json::array();
    for (size_t i = 0; i < snap.tables.size(); ++i) {
        tables.push_back(tableJson(snap.tables[i]));
    }
    j["tables"] = tables;
    Json chat = Json::array();
    for (size_t i = 0; i < snap.recentChat.size(); ++i) {
        chat.push_back(lobbyEventJson(snap.recentChat[i]));
    }
    j["recent_chat"] = chat;
    j["latest_seq"] = snap.latestSeq;
    CachedPayload lobbyStored = lobbyStateCache.put(
        "lobby", snapRev, 500, snapNowMs, "ls-", dumpJson(j));
    sendCachedSnapshot(req, res, lobbyStored, "no-cache");
}

void OctiHttpServer::Impl::handleLobbyChat(const Request &req, Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth) ||
        !allowMutationAttempt(req, res, auth.playerId)) {
        return;
    }
    Json body;
    if (!readJsonBody(req, res, body, false)) {
        return;
    }
    std::string text;
    std::string err;
    if (!fieldString(body, "text", true, text, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    finishRoom(req, auth.playerId, res,
               rooms.postLobbyChat(auth.playerId, text), Json::object());
}

void OctiHttpServer::Impl::handleLobbyEvents(const Request &req,
                                             Response &res)
{
    Auth auth;
    if (!optionalAuth(req, res, auth)) {
        return;
    }
    const bool stream = wantsStream(req);
    if (!stream && !allowLivePoll(req, res, auth)) {
        return;
    }
    std::string remoteIp = clientRemoteIp(req);
    const int64_t viewer =
        (auth.playerId != 0)
            ? auth.playerId
            : (stream ? rooms.ensureAnonymousObserver(remoteIp)
                      : rooms.findAnonymousObserver(remoteIp));
    if (auth.playerId == 0 && stream && !isLoopbackIp(remoteIp)) {
        rooms.recordClientHeartbeat(viewer, auth.clientType, false);
    }
    PollFn fetch = [this, viewer](int64_t since, size_t limit,
                                  PollBatch &out) -> int {
        LobbyPoll poll = rooms.pollLobbyEvents(viewer, since, limit);
        out.latestSeq = poll.latestSeq;
        out.gap = poll.gap;
        out.hasMore = poll.hasMore;
        for (size_t i = 0; i < poll.events.size(); ++i) {
            Frame f;
            f.seq = poll.events[i].seq;
            f.type = toString(poll.events[i].type);
            f.json = dumpJson(lobbyEventJson(poll.events[i]));
            out.frames.push_back(f);
        }
        out.nextSeq = poll.hasMore && !out.frames.empty()
                          ? out.frames.back().seq
                          : std::max(since, poll.latestSeq);
        return 200;
    };
    respondEvents(req, res, auth, fetch);
}

void OctiHttpServer::Impl::handleTables(const Request &req, Response &res)
{
    Auth auth;
    if (!optionalAuth(req, res, auth) || !allowLobbySnap(req, res, auth)) {
        return;
    }
    bool includeFinished = false;
    if (req.has_param("include_finished")) {
        std::string v = lower(req.get_param_value("include_finished"));
        includeFinished = (v == "1" || v == "true");
    }
    const std::string tblKey = includeFinished ? "all" : "open";
    const uint64_t tblRev =
        rooms.lobbySnapshotRevision() ^
        (store.mutationRevision() * 0x9e3779b97f4a7c15ULL);
    const int64_t tblNowMs = rooms.currentClockMs();
    CachedPayload tblCached;
    if (tablesCache.get(tblKey, tblRev, tblNowMs, tblCached)) {
        sendCachedSnapshot(req, res, tblCached, "no-cache");
        return;
    }
    std::vector<TableSummary> tables = rooms.listTables(includeFinished);
    Json arr = Json::array();
    for (size_t i = 0; i < tables.size(); ++i) {
        arr.push_back(tableJson(tables[i]));
    }
    Json j;
    j["tables"] = arr;
    CachedPayload tblStored = tablesCache.put(
        tblKey, tblRev, 500, tblNowMs, "tbl-", dumpJson(j));
    sendCachedSnapshot(req, res, tblStored, "no-cache");
}

void OctiHttpServer::Impl::handleTableCreate(const Request &req,
                                             Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth) ||
        !allowMutationAttempt(req, res, auth.playerId)) {
        return;
    }
    Json body;
    if (!readJsonBody(req, res, body, true)) {
        return;
    }
    GameTableConfig cfgIn;
    std::string err;
    if (!parseTableConfig(body, cfgIn, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    int64_t seat = 1;
    if (!fieldInt(body, "seat", false, seat, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    if (seat < 0 || seat > 4) {
        sendError(res, 400, "INVALID_ARGUMENT", "seat must be 0..4");
        return;
    }
    RoomResult r = rooms.createTable(auth.playerId, cfgIn, static_cast<int>(seat));
    if (!r.ok()) {
        sendRoomError(res, r);
        return;
    }
    Json j;
    j["ok"] = true;
    j["room_id"] = r.roomId;
    j["seat"] = r.seat;
    sendJson(res, 201, j);
}

void OctiHttpServer::Impl::handleTableJoin(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!requireAuth(req, res, auth) || !pathId(req, "id", res, id) ||
        !allowMutationAttempt(req, res, auth.playerId)) {
        return;
    }
    Json body;
    if (!readJsonBody(req, res, body, true)) {
        return;
    }
    int64_t seat = 0;
    std::string passcode;
    std::string err;
    if (!fieldInt(body, "seat", false, seat, err) ||
        !fieldString(body, "passcode", false, passcode, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    if (seat < 0 || seat > 4) {
        sendError(res, 400, "INVALID_ARGUMENT", "seat must be 0..4");
        return;
    }
    if (!passcode.empty() && checkPasscodeBlocked(req, res, auth.playerId)) {
        return;
    }
    RoomResult r =
        rooms.joinTable(id, auth.playerId, static_cast<int>(seat), passcode);
    if (r.code == RoomCode::FORBIDDEN_PASSCODE) {
        recordPasscodeFailure(req, res, auth.playerId, r);
        if (res.status == 429) {
            return;
        }
    }
    Json j;
    j["room_id"] = r.roomId;
    j["seat"] = r.seat;
    j["game_started"] = r.gameStarted;
    finishRoom(res, r, j);
}

void OctiHttpServer::Impl::handleTableLeave(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!requireAuth(req, res, auth) || !pathId(req, "id", res, id) ||
        !allowMutationAttempt(req, res, auth.playerId)) {
        return;
    }
    Json body;
    if (!readJsonBody(req, res, body, true)) {
        return;
    }
    bool abortIfPly0 = false;
    std::string err;
    if (!fieldBool(body, "abort_if_ply0", false, abortIfPly0, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    RoomResult r = rooms.leaveTable(id, auth.playerId, abortIfPly0);
    Json j;
    j["stood_up_ply0"] = (r.detail == "ply0_standup");
    j["detail"] = r.detail;
    j["seq"] = r.seq;
    finishRoom(res, r, j);
}

void OctiHttpServer::Impl::handleTablePurge(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!requireAuth(req, res, auth) || !pathId(req, "id", res, id) ||
        !allowMutationAttempt(req, res, auth.playerId)) {
        return;
    }
    bool isHost = !auth.isRobot && rooms.isTableHost(id, auth.playerId);
    bool isAdmin = !auth.isRobot && store.isPlayerAdmin(auth.playerId);
    if (!isHost && !isAdmin) {
        sendError(res, 403, "ERR_FORBIDDEN",
                  "only the table host or an administrator may delete a table");
        return;
    }
    RoomResult r = rooms.purgeRoom(id);
    if (!r.ok()) {
        if (r.code == RoomCode::NOT_FOUND) {
            sendError(res, 404, "NOT_FOUND", "room not found");
        } else {
            sendError(res, 400, "BAD_REQUEST", r.message);
        }
        return;
    }
    Json j;
    j["ok"] = true;
    j["status"] = "purged";
    j["room_id"] = id;
    sendJson(res, 200, j);
}

void OctiHttpServer::Impl::handleTableKick(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!requireAuth(req, res, auth) || !pathId(req, "id", res, id) ||
        !allowMutationAttempt(req, res, auth.playerId)) {
        return;
    }
    Json body;
    int64_t target = 0;
    if (!readJsonBody(req, res, body, false) ||
        !resolveTarget(body, "target", res, target)) {
        return;
    }
    finishRoom(res, rooms.kickSeat(id, auth.playerId, target), Json::object());
}

void OctiHttpServer::Impl::handleTableObserve(const Request &req,
                                              Response &res)
{
    Auth auth;
    std::string id;
    if (!pathId(req, "id", res, id) || !optionalAuth(req, res, auth) ||
        !allowMutationAttempt(req, res, auth.playerId)) {
        return;
    }
    int64_t viewerId = (auth.playerId > 0)
                           ? auth.playerId
                           : rooms.ensureAnonymousObserver(clientRemoteIp(req));
    RoomResult r = rooms.observeRoom(id, viewerId);
    if (!r.ok()) {
        if (r.code == RoomCode::FORBIDDEN_PASSCODE) {
            recordPasscodeFailure(req, res, auth.playerId, r);
            if (res.status == 429) {
                return;
            }
        }
        sendRoomError(res, r);
        return;
    }
    RoomView view;
    rooms.getRoomView(id, viewerId, view);
    sendJson(res, 200, roomViewJson(view));
}

void OctiHttpServer::Impl::handleTableUnobserve(const Request &req,
                                                Response &res)
{
    Auth auth;
    std::string id;
    if (!pathId(req, "id", res, id) || !optionalAuth(req, res, auth) ||
        !allowMutationAttempt(req, res, auth.playerId)) {
        return;
    }
    int64_t viewerId = (auth.playerId > 0)
                           ? auth.playerId
                           : rooms.ensureAnonymousObserver(clientRemoteIp(req));
    finishRoom(res, rooms.stopObserving(id, viewerId), Json::object());
}

void OctiHttpServer::Impl::handleEnqueue(const Request &req, Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth) ||
        !allowMutationAttempt(req, res, auth.playerId)) {
        return;
    }
    Json body;
    if (!readJsonBody(req, res, body, true)) {
        return;
    }
    GameTableConfig c;
    std::string err;
    if (!parseTableConfig(body, c, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    MatchRuleset rs;
    rs.mode = c.mode;
    rs.bases = c.bases;
    rs.useSuperProngs = c.useSuperProngs;
    rs.useEdgelessBoard = c.useEdgelessBoard;
    rs.initialMs = c.timeControl.enabled ? c.timeControl.initialMs : 0;
    rs.incrementMs = c.timeControl.enabled ? c.timeControl.incrementMs : 0;
    rs.allowRobots = c.allowRobots;
    Json j;
    j["queued"] = true;
    finishRoom(res, rooms.enqueueMatchmaking(auth.playerId, rs), j);
}

void OctiHttpServer::Impl::handleDequeue(const Request &req, Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth) ||
        !allowMutationAttempt(req, res, auth.playerId)) {
        return;
    }
    Json j;
    j["queued"] = false;
    finishRoom(res, rooms.dequeueMatchmaking(auth.playerId), j);
}

void OctiHttpServer::Impl::handleMatchStatus(const Request &req,
                                             Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth) || !allowLivePoll(req, res, auth)) {
        return;
    }
    MatchmakingStatus st = rooms.matchmakingStatus(auth.playerId);
    Json j;
    j["queued"] = st.queued;
    j["waited_ms"] = st.waitedMs;
    j["window"] = st.window;
    j["rating"] = st.rating;
    j["matched_room_id"] = st.matchedRoomId;
    sendJson(res, 200, j);
}

/* ------------------------------------------------------------------ */
/* Handlers: games                                                     */
/* ------------------------------------------------------------------ */

void OctiHttpServer::Impl::handleGameGet(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!pathId(req, "id", res, id) || !optionalAuth(req, res, auth) ||
        !allowLivePoll(req, res, auth)) {
        return;
    }
    const int64_t viewerId = (auth.playerId != 0)
                                 ? auth.playerId
                                 : rooms.findAnonymousObserver(clientRemoteIp(req));
    RoomView view;
    RoomResult r = rooms.getRoomView(id, viewerId, view);
    if (r.code == RoomCode::NOT_FOUND) {
        GameRecord rec;
        if (store.getArchivedGame(id, rec)) {
            Json j = gameSummaryJson(rec);
            j["live"] = false;
            j["room_id"] = rec.gameId;
            j["phase"] = "FINISHED";
            j["fen"] = rec.finalFen;
            j["archived"] = true;
            sendJson(res, 200, j);
            return;
        }
    }
    if (!r.ok()) {
        sendRoomError(res, r);
        return;
    }
    sendJson(res, 200, roomViewJson(view));
}

void OctiHttpServer::Impl::handleMove(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!requireAuth(req, res, auth) || !pathId(req, "id", res, id)) {
        return;
    }
    Json body;
    std::string move;
    std::string err;
    if (!readJsonBody(req, res, body, false)) {
        return;
    }
    if (!fieldString(body, "move", true, move, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    GameRoomManager::MoveTelemetry telemetry;
    if (body.contains("ai") && body["ai"].is_object()) {
        const auto &ai = body["ai"];
        telemetry.present = true;
        if (ai.contains("depth") && ai["depth"].is_number_integer()) {
            telemetry.depth = ai["depth"].get<int>();
        }
        if (ai.contains("nodes") && ai["nodes"].is_number_integer()) {
            telemetry.nodes = ai["nodes"].get<int64_t>();
        }
        if (ai.contains("score") && ai["score"].is_number_integer()) {
            telemetry.score = ai["score"].get<int>();
        }
        if (ai.contains("elapsed_ms") && ai["elapsed_ms"].is_number_integer()) {
            telemetry.elapsedMs = ai["elapsed_ms"].get<int64_t>();
        }
    }
    RoomResult r = rooms.submitMove(id, auth.playerId, move, telemetry);
    Json j;
    j["move"] = r.move;
    j["clock_ms"] = r.clockMs;
    j["elapsed_ms"] = r.elapsedMs;
    j["game_over"] = r.gameOver;
    finishRoom(res, r, j);
}

void OctiHttpServer::Impl::handleResign(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!requireAuth(req, res, auth) || !pathId(req, "id", res, id) ||
        !allowMutationAttempt(req, res, auth.playerId)) {
        return;
    }
    RoomResult r = rooms.resign(id, auth.playerId);
    Json j;
    j["game_over"] = r.gameOver;
    finishRoom(res, r, j);
}

void OctiHttpServer::Impl::handleAction(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!requireAuth(req, res, auth) || !pathId(req, "id", res, id) ||
        !allowMutationAttempt(req, res, auth.playerId)) {
        return;
    }
    Json body;
    std::string action;
    std::string err;
    if (!readJsonBody(req, res, body, false)) {
        return;
    }
    if (!fieldString(body, "action", true, action, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }

    if (action == "DRAW_OFFER") {
        action = "offer_draw";
    } else if (action == "ACCEPT_DRAW") {
        action = "accept_draw";
    } else if (action == "DECLINE_DRAW") {
        action = "decline_draw";
    } else if (action == "PAUSE_REQUEST") {
        action = "request_pause";
    } else if (action == "ACCEPT_PAUSE") {
        action = "accept_pause";
    } else if (action == "DECLINE_PAUSE") {
        action = "decline_pause";
    } else if (action == "RESUME") {
        action = "resume";
    } else if (action == "UNDO_REQUEST") {
        action = "request_undo";
    } else if (action == "ACCEPT_UNDO") {
        action = "accept_undo";
    } else if (action == "DECLINE_UNDO") {
        action = "decline_undo";
    } else if (action == "REMATCH_REQUEST") {
        action = "request_rematch";
    } else if (action == "ACCEPT_REMATCH") {
        action = "accept_rematch";
    } else if (action == "DECLINE_REMATCH") {
        action = "decline_rematch";
    } else if (action == "ABORT") {
        action = "abort";
    } else if (action == "STAND_UP") {
        action = "stand_up";
    }

    const int64_t pid = auth.playerId;
    RoomResult r;
    if (action == "offer_draw") {
        r = rooms.submitProposal(id, pid, ProposalType::DRAW_OFFER);
    } else if (action == "accept_draw") {
        r = rooms.respondProposal(id, pid, ProposalType::DRAW_OFFER, true);
    } else if (action == "decline_draw") {
        r = rooms.respondProposal(id, pid, ProposalType::DRAW_OFFER, false);
    } else if (action == "request_pause") {
        r = rooms.submitProposal(id, pid, ProposalType::PAUSE_REQUEST);
    } else if (action == "accept_pause") {
        r = rooms.respondProposal(id, pid, ProposalType::PAUSE_REQUEST, true);
    } else if (action == "decline_pause") {
        r = rooms.respondProposal(id, pid, ProposalType::PAUSE_REQUEST, false);
    } else if (action == "resume") {
        r = rooms.resumeGame(id, pid);
    } else if (action == "request_undo") {
        r = rooms.submitProposal(id, pid, ProposalType::UNDO_REQUEST);
    } else if (action == "accept_undo") {
        r = rooms.respondProposal(id, pid, ProposalType::UNDO_REQUEST, true);
    } else if (action == "decline_undo") {
        r = rooms.respondProposal(id, pid, ProposalType::UNDO_REQUEST, false);
    } else if (action == "request_rematch") {
        r = rooms.submitProposal(id, pid, ProposalType::REMATCH_REQUEST);
    } else if (action == "accept_rematch") {
        r = rooms.respondProposal(id, pid, ProposalType::REMATCH_REQUEST, true);
    } else if (action == "decline_rematch") {
        r = rooms.respondProposal(id, pid, ProposalType::REMATCH_REQUEST, false);
    } else if (action == "claim_forfeit") {
        r = rooms.claimDisconnectForfeit(id, pid);
    } else if (action == "abort" || action == "stand_up") {
        r = rooms.standUpAtPly0(id, pid);
    } else {
        sendError(res, 400, "INVALID_ARGUMENT", "unknown action");
        return;
    }
    Json j;
    j["action"] = action;
    j["room_id"] = r.roomId.empty() ? id : r.roomId;
    j["game_over"] = r.gameOver;
    if (action == "abort" || action == "stand_up") {
        j["stood_up_ply0"] = (r.detail == "ply0_standup");
        j["detail"] = r.detail;
        j["seq"] = r.seq;
    }
    finishRoom(res, r, j);
}

void OctiHttpServer::Impl::handleChat(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!requireAuth(req, res, auth) || !pathId(req, "id", res, id) ||
        !allowMutationAttempt(req, res, auth.playerId)) {
        return;
    }
    Json body;
    std::string text;
    std::string scopeText = "all";
    std::string err;
    if (!readJsonBody(req, res, body, false)) {
        return;
    }
    if (!fieldString(body, "text", true, text, err) ||
        !fieldString(body, "scope", false, scopeText, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    ChatScope scope = ChatScope::ALL;
    if (!parseChatScope(scopeText, scope)) {
        sendError(res, 400, "INVALID_ARGUMENT", "scope must be 'all' or 'team'");
        return;
    }
    finishRoom(req, auth.playerId, res,
               rooms.postBoardChat(id, auth.playerId, scope, text),
               Json::object());
}

void OctiHttpServer::Impl::handleBan(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!requireAuth(req, res, auth) || !pathId(req, "id", res, id) ||
        !allowMutationAttempt(req, res, auth.playerId)) {
        return;
    }
    Json body;
    int64_t target = 0;
    std::string reason;
    std::string err;
    if (!readJsonBody(req, res, body, false) ||
        !resolveTarget(body, "target", res, target)) {
        return;
    }
    if (!fieldString(body, "reason", false, reason, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    RoomResult r = rooms.banUser(id, auth.playerId, target, reason);
    Json j;
    if (r.ok() && !r.message.empty()) {
        j["note"] = r.message;
    }
    finishRoom(res, r, j);
}

/*
 * Sets the chat mute state of the target.
 */
void OctiHttpServer::Impl::handleMute(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!requireAuth(req, res, auth) || !pathId(req, "id", res, id) ||
        !allowMutationAttempt(req, res, auth.playerId)) {
        return;
    }
    Json body;
    int64_t target = 0;
    if (!readJsonBody(req, res, body, false) ||
        !resolveTarget(body, "target", res, target)) {
        return;
    }
    if (!fieldPresent(body, "mute_chat")) {
        sendError(res, 400, "INVALID_ARGUMENT", "send mute_chat");
        return;
    }
    bool muteChat = false;
    std::string err;
    if (!fieldBool(body, "mute_chat", false, muteChat, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    finishRoom(res,
               rooms.muteUser(id, auth.playerId, target, muteChat),
               Json::object());
}

void OctiHttpServer::Impl::handleSettings(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!requireAuth(req, res, auth) || !pathId(req, "id", res, id) ||
        !allowMutationAttempt(req, res, auth.playerId)) {
        return;
    }
    Json body;
    if (!readJsonBody(req, res, body, false)) {
        return;
    }
    std::string observerText;
    std::string err;
    if (!fieldString(body, "observer_policy", false, observerText, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    if (observerText.empty()) {
        sendError(res, 400, "INVALID_ARGUMENT", "send observer_policy");
        return;
    }
    std::optional<ObserverPolicy> op;
    ObserverPolicy parsed = ObserverPolicy::OPEN;
    if (!parseObserverPolicy(observerText, parsed)) {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "observer_policy must be OPEN, SILENT_OBSERVERS or "
                  "NO_OBSERVERS");
        return;
    }
    op = parsed;
    RoomResult r = rooms.updateRoomPolicies(id, auth.playerId, op);
    if (!r.ok()) {
        sendRoomError(res, r);
        return;
    }
    RoomView view;
    RoomResult cur = rooms.getRoomView(id, auth.playerId, view);
    if (!cur.ok()) {
        sendRoomError(res, cur);
        return;
    }
    Json j;
    j["observer_policy"] = toString(view.config.observerPolicy);
    finishRoom(res, r, j);
}

void OctiHttpServer::Impl::handleGameEvents(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!pathId(req, "id", res, id) || !optionalAuth(req, res, auth)) {
        return;
    }
    const bool stream = wantsStream(req);
    if (!stream && !allowLivePoll(req, res, auth)) {
        return;
    }
    const std::string remoteIp = clientRemoteIp(req);
    const int64_t viewer =
        (auth.playerId != 0)
            ? auth.playerId
            : (stream ? rooms.ensureAnonymousObserver(remoteIp)
                      : rooms.findAnonymousObserver(remoteIp));
    PollFn fetch = [this, id, viewer](int64_t since, size_t limit,
                                      PollBatch &out) -> int {
        RoomPoll poll;
        RoomResult r = rooms.pollRoomEvents(id, viewer, since, limit, poll);
        if (!r.ok()) {
            out.errorCode = toString(r.code);
            out.message = r.message;
            return statusFor(r.code);
        }
        out.latestSeq = poll.latestSeq;
        out.gap = poll.gap;
        out.hasMore = poll.hasMore;
        for (size_t i = 0; i < poll.events.size(); ++i) {
            Frame f;
            f.seq = poll.events[i].seq;
            f.type = toString(poll.events[i].type);
            f.json = dumpJson(roomEventJson(poll.events[i]));
            out.frames.push_back(f);
        }
        out.nextSeq = poll.hasMore && !out.frames.empty()
                          ? out.frames.back().seq
                          : std::max(since, poll.latestSeq);
        return 200;
    };
    respondEvents(req, res, auth, fetch);
}

/* ------------------------------------------------------------------ */
/* Handlers: archive and UI                                            */
/* ------------------------------------------------------------------ */

static std::string sanitizeTranscriptJson(const std::string &rawJsonStr, bool pretty = false)
{
    try {
        auto j = nlohmann::json::parse(rawJsonStr);
        if (j.is_object()) {
            if (j.contains("client") && j["client"].is_object()) {
                j["client"].erase("extended");
            }
            j["license"] = "CC-BY-4.0";
            j["attribution"] = OCTI_DATA_ATTRIBUTION;
            return pretty ? j.dump(2) : j.dump();
        }
    } catch (...) {}
    return rawJsonStr;
}

static int countPods(const OctiBoard &b, OctiPlayer player)
{
    int count = 0;
    for (int x = 0; x < b.getBoardWidth(); ++x) {
        for (int y = 0; y < b.getBoardHeight(); ++y) {
            const OctiSquare &sq = b.squareAt(x, y);
            for (int i = 0; i < sq.getNumPods(); ++i) {
                if (podTypeToPlayer(sq.getPodAt(i).getType()) == player) {
                    ++count;
                }
            }
        }
    }
    return count;
}

static std::string classifyRuleTermination(const OctiBoard &b, const OctiGame &g)
{
    OctiPlayer w = g.getWinner();
    if (w == PLAYER_UNDEF) {
        return "NO_PROGRESS_DRAW";
    }
    if (g.getMode() == GAME_MODE_2) {
        const int xs[3] = {2, 4, 6};
        int y = (w == PLAYER_A) ? 6 : 2;
        int captured = 0;
        for (int i = 0; i < 3; ++i) {
            if (b.squareAt(xs[i], y).isCapturedBy(w)) {
                ++captured;
            }
        }
        int needed = (g.getBases() == GAME_BASES_1) ? 1 : 3;
        if (captured >= needed) {
            return "BASE_CAPTURE";
        }
        OctiPlayer opp = (w == PLAYER_A) ? PLAYER_B : PLAYER_A;
        return (countPods(b, opp) == 0 && g.getFreePods(opp) == 0) ? "TOTAL_CAPTURE" : "STALEMATE";
    }
    if (g.getMode() == GAME_MODE_OFK) {
        int y = (w == PLAYER_A) ? 5 : 1;
        for (int x = 1; x <= 4; ++x) {
            if (b.squareAt(x, y).isCapturedBy(w)) {
                return "BASE_CAPTURE";
            }
        }
        OctiPlayer opp = (w == PLAYER_A) ? PLAYER_B : PLAYER_A;
        return countPods(b, opp) == 0 ? "TOTAL_CAPTURE" : "STALEMATE";
    }
    bool bases;
    if (w == PLAYER_A || w == PLAYER_C) {
        bases = b.squareAt(2, 6).isCapturedByTeamOf(PLAYER_A) &&
                b.squareAt(6, 2).isCapturedByTeamOf(PLAYER_A);
    } else {
        bases = b.squareAt(2, 2).isCapturedByTeamOf(PLAYER_B) &&
                b.squareAt(6, 6).isCapturedByTeamOf(PLAYER_B);
    }
    if (bases) {
        return "BASE_CAPTURE";
    }
    int bdPods = countPods(b, PLAYER_B) + countPods(b, PLAYER_D);
    int acPods = countPods(b, PLAYER_A) + countPods(b, PLAYER_C);
    int bdFree = (g.isPlayerEliminated(PLAYER_B) ? 0 : g.getFreePods(PLAYER_B)) +
                 (g.isPlayerEliminated(PLAYER_D) ? 0 : g.getFreePods(PLAYER_D));
    int acFree = (g.isPlayerEliminated(PLAYER_A) ? 0 : g.getFreePods(PLAYER_A)) +
                 (g.isPlayerEliminated(PLAYER_C) ? 0 : g.getFreePods(PLAYER_C));
    if ((w == PLAYER_A || w == PLAYER_C) && bdPods == 0 && bdFree == 0) {
        return "TOTAL_CAPTURE";
    }
    if ((w == PLAYER_B || w == PLAYER_D) && acPods == 0 && acFree == 0) {
        return "TOTAL_CAPTURE";
    }
    return "STALEMATE";
}

void OctiHttpServer::Impl::handleArchiveUpload(const Request &req, Response &res)
{
    const size_t maxBytes = routeMaxBodyBytes(req.method, req.path);
    if (req.body.empty() || req.body.size() > maxBytes) {
        sendError(res, req.body.size() > maxBytes ? 413 : 400,
                  req.body.size() > maxBytes ? "PAYLOAD_TOO_LARGE" : "MALFORMED_UPLOAD",
                  "invalid payload size");
        return;
    }
    if (!allowUploadAttempt(req, res)) {
        return;
    }
    Json root = Json::parse(req.body, nullptr, false);
    if (root.is_discarded() || !root.is_object()) {
        sendError(res, 400, "MALFORMED_UPLOAD", "invalid JSON payload");
        return;
    }

    Json transcriptObj;
    std::string clientId;

    if (root.contains("transcript")) {
        if (root["transcript"].is_string()) {
            std::string tStr = root["transcript"].get<std::string>();
            if (tStr.size() > 49152) {
                sendError(res, 400, "MALFORMED_UPLOAD", "transcript exceeds 48 KiB");
                return;
            }
            transcriptObj = Json::parse(tStr, nullptr, false);
            if (transcriptObj.is_discarded() || !transcriptObj.is_object()) {
                sendError(res, 400, "MALFORMED_UPLOAD", "transcript is not valid JSON");
                return;
            }
        } else if (root["transcript"].is_object()) {
            transcriptObj = root["transcript"];
        } else {
            sendError(res, 400, "MALFORMED_UPLOAD", "transcript must be a string or object");
            return;
        }
        if (root.contains("clientId")) {
            if (!root["clientId"].is_string() || root["clientId"].get<std::string>().size() > 128) {
                sendError(res, 400, "MALFORMED_UPLOAD", "invalid clientId");
                return;
            }
            clientId = root["clientId"].get<std::string>();
        }
    } else if (root.contains("format")) {
        transcriptObj = root;
    } else {
        sendError(res, 400, "MALFORMED_UPLOAD", "missing transcript or format field");
        return;
    }

    if (transcriptObj.dump().size() > 49152) {
        sendError(res, 400, "MALFORMED_UPLOAD", "transcript exceeds 48 KiB");
        return;
    }

    if (!transcriptObj.contains("format") || !transcriptObj["format"].is_string() ||
        transcriptObj["format"].get<std::string>() != "octi.transcript.v3") {
        sendError(res, 400, "MALFORMED_UPLOAD", "unsupported transcript format");
        return;
    }

    if (!transcriptObj.contains("mode") || !transcriptObj["mode"].is_string()) {
        sendError(res, 400, "MALFORMED_UPLOAD", "missing mode");
        return;
    }
    std::string modeStr = transcriptObj["mode"].get<std::string>();
    OctiGameMode mode;
    if (modeStr == "2P" || modeStr == "2") {
        mode = GAME_MODE_2;
        modeStr = "2P";
    } else if (modeStr == "4P" || modeStr == "4") {
        mode = GAME_MODE_4;
        modeStr = "4P";
    } else if (modeStr == "OFK") {
        mode = GAME_MODE_OFK;
    } else {
        sendError(res, 400, "MALFORMED_UPLOAD", "invalid mode");
        return;
    }

    int basesVal = 1;
    OctiGameBases bases = GAME_BASES_1;
    bool superProngs = false;
    bool edgeless = false;

    if (mode == GAME_MODE_OFK) {
        basesVal = 1;
        bases = GAME_BASES_1;
        superProngs = false;
        edgeless = false;
    } else if (mode == GAME_MODE_4) {
        basesVal = 3;
        int claimedBases = transcriptObj.value("bases", 3);
        bases = (claimedBases == 1) ? GAME_BASES_1 : GAME_BASES_3;
        if (transcriptObj.contains("superProngs") && transcriptObj["superProngs"].is_boolean()) {
            superProngs = transcriptObj["superProngs"].get<bool>();
        }
        if (transcriptObj.contains("edgeless") && transcriptObj["edgeless"].is_boolean()) {
            edgeless = transcriptObj["edgeless"].get<bool>();
        }
    } else {
        if (!transcriptObj.contains("bases") || !transcriptObj["bases"].is_number_integer()) {
            sendError(res, 400, "MALFORMED_UPLOAD", "missing bases");
            return;
        }
        basesVal = transcriptObj["bases"].get<int>();
        if (basesVal != 1 && basesVal != 3) {
            sendError(res, 400, "MALFORMED_UPLOAD", "bases must be 1 or 3");
            return;
        }
        bases = (basesVal == 3) ? GAME_BASES_3 : GAME_BASES_1;

        if (!transcriptObj.contains("superProngs") || !transcriptObj["superProngs"].is_boolean() ||
            !transcriptObj.contains("edgeless") || !transcriptObj["edgeless"].is_boolean()) {
            sendError(res, 400, "MALFORMED_UPLOAD", "missing superProngs or edgeless");
            return;
        }
        superProngs = transcriptObj["superProngs"].get<bool>();
        edgeless = transcriptObj["edgeless"].get<bool>();
    }

    std::string startPlayerStr = transcriptObj.value("startPlayer", "A");
    OctiPlayer startPlayer = PLAYER_A;
    if (startPlayerStr == "A") startPlayer = PLAYER_A;
    else if (startPlayerStr == "B") startPlayer = PLAYER_B;
    else if (startPlayerStr == "C") startPlayer = PLAYER_C;
    else if (startPlayerStr == "D") startPlayer = PLAYER_D;
    else {
        sendError(res, 400, "MALFORMED_UPLOAD", "invalid startPlayer");
        return;
    }

    if (!transcriptObj.contains("status") || !transcriptObj["status"].is_string()) {
        sendError(res, 400, "MALFORMED_UPLOAD", "missing status");
        return;
    }
    std::string statusStr = transcriptObj["status"].get<std::string>();
    if (statusStr == "IN_PROGRESS" || statusStr == "NOT_STARTED") {
        sendError(res, 400, "GAME_NOT_COMPLETED", "game is not completed");
        return;
    }
    if (statusStr != "GAME_OVER" && statusStr != "RESIGNED") {
        sendError(res, 400, "MALFORMED_UPLOAD", "invalid status");
        return;
    }

    if (!transcriptObj.contains("winner") || !transcriptObj["winner"].is_string()) {
        sendError(res, 400, "MALFORMED_UPLOAD", "missing winner");
        return;
    }
    std::string claimedWinner = transcriptObj["winner"].get<std::string>();
    std::string normalizedClaimedWinner = claimedWinner;
    if (mode == GAME_MODE_4) {
        if (claimedWinner == "TEAM_13_WIN" || claimedWinner == "A_WIN" || claimedWinner == "C_WIN") {
            normalizedClaimedWinner = "A";
        } else if (claimedWinner == "TEAM_24_WIN" || claimedWinner == "B_WIN" || claimedWinner == "D_WIN") {
            normalizedClaimedWinner = "B";
        }
    }
    if (normalizedClaimedWinner != "A" && normalizedClaimedWinner != "B" &&
        normalizedClaimedWinner != "C" && normalizedClaimedWinner != "D" &&
        normalizedClaimedWinner != "NONE") {
        sendError(res, 400, "MALFORMED_UPLOAD", "invalid winner value");
        return;
    }
    std::string claimedFinalFen;
    if (transcriptObj.contains("finalFen") && transcriptObj["finalFen"].is_string()) {
        claimedFinalFen = transcriptObj["finalFen"].get<std::string>();
    } else if (transcriptObj.contains("final_fen") && transcriptObj["final_fen"].is_string()) {
        claimedFinalFen = transcriptObj["final_fen"].get<std::string>();
    } else {
        sendError(res, 400, "MALFORMED_UPLOAD", "missing finalFen");
        return;
    }

    std::string appVersion;
    std::string platform;
    std::string installId = clientId;
    if (transcriptObj.contains("client")) {
        if (!transcriptObj["client"].is_object()) {
            sendError(res, 400, "MALFORMED_UPLOAD", "client must be an object");
            return;
        }
        const auto &clientObj = transcriptObj["client"];
        for (auto it = clientObj.begin(); it != clientObj.end(); ++it) {
            const std::string &k = it.key();
            if (k != "appVersion" && k != "buildType" && k != "platform" && k != "extended") {
                if (!it.value().is_string() && !it.value().is_number()) {
                    sendError(res, 400, "MALFORMED_UPLOAD", "invalid unknown client field type");
                    return;
                }
                if (it.value().is_string() && it.value().get<std::string>().size() > 128) {
                    sendError(res, 400, "MALFORMED_UPLOAD", "oversized unknown client field");
                    return;
                }
            }
        }
        if (clientObj.contains("appVersion")) {
            if (!clientObj["appVersion"].is_string() || clientObj["appVersion"].get<std::string>().size() > 128) {
                sendError(res, 400, "MALFORMED_UPLOAD", "invalid appVersion");
                return;
            }
            appVersion = clientObj["appVersion"].get<std::string>();
            if (hasControlChars(appVersion)) {
                sendError(res, 400, "MALFORMED_UPLOAD", "appVersion has control chars");
                return;
            }
        }
        if (clientObj.contains("platform")) {
            if (!clientObj["platform"].is_string() || clientObj["platform"].get<std::string>().size() > 128) {
                sendError(res, 400, "MALFORMED_UPLOAD", "invalid platform");
                return;
            }
            platform = clientObj["platform"].get<std::string>();
            if (hasControlChars(platform)) {
                sendError(res, 400, "MALFORMED_UPLOAD", "platform has control chars");
                return;
            }
        }
        if (clientObj.contains("extended")) {
            if (!clientObj["extended"].is_object()) {
                sendError(res, 400, "MALFORMED_UPLOAD", "extended must be an object");
                return;
            }
            const auto &ext = clientObj["extended"];
            for (auto it = ext.begin(); it != ext.end(); ++it) {
                const std::string &k = it.key();
                if (k != "startedAt" && k != "endedAt" && k != "utcOffsetMin" &&
                    k != "locale" && k != "osVersion" && k != "installId") {
                    if (it.value().is_string() && it.value().get<std::string>().size() > 128) {
                        sendError(res, 400, "MALFORMED_UPLOAD", "oversized extended field");
                        return;
                    }
                }
            }
            if (ext.contains("installId") && ext["installId"].is_string()) {
                std::string iid = ext["installId"].get<std::string>();
                if (iid.size() > 128 || hasControlChars(iid)) {
                    sendError(res, 400, "MALFORMED_UPLOAD", "invalid installId in extended");
                    return;
                }
                if (!iid.empty()) {
                    installId = iid;
                }
            }
            if (ext.contains("locale") && ext["locale"].is_string()) {
                if (ext["locale"].get<std::string>().size() > 128 || hasControlChars(ext["locale"].get<std::string>())) {
                    sendError(res, 400, "MALFORMED_UPLOAD", "invalid locale");
                    return;
                }
            }
            if (ext.contains("osVersion") && ext["osVersion"].is_string()) {
                if (ext["osVersion"].get<std::string>().size() > 128 || hasControlChars(ext["osVersion"].get<std::string>())) {
                    sendError(res, 400, "MALFORMED_UPLOAD", "invalid osVersion");
                    return;
                }
            }
        }
    }

    PlayerProfile authedPlayer;
    bool hasAuthedPlayer = false;
    if (req.has_header("Authorization")) {
        std::string token;
        if (extractBearer(req, token)) {
            SessionValidation sv = store.validateSession(token);
            if (sv.valid && sv.playerId > 0 &&
                store.getProfileById(sv.playerId, authedPlayer)) {
                hasAuthedPlayer = true;
            }
        }
    }

    if (req.has_header("X-Octi-Install-Token")) {
        std::string installTok = req.get_header_value("X-Octi-Install-Token");
        if (!AccountStore::isValidInstallToken(installTok)) {
            sendError(res, 400, "MALFORMED_UPLOAD", "invalid X-Octi-Install-Token");
            return;
        }
        if (!installId.empty() && isSafeId(installId)) {
            if (!store.registerOrVerifyInstallToken(installId, installTok)) {
                sendError(res, 403, "FORBIDDEN", "install token mismatch");
                return;
            }
        }
    } else if (!installId.empty() && isSafeId(installId) &&
               store.hasInstallToken(installId)) {
        sendError(res, 403, "FORBIDDEN",
                  "missing X-Octi-Install-Token for registered installId");
        return;
    }

    if (!transcriptObj.contains("plies") || !transcriptObj["plies"].is_array()) {
        sendError(res, 400, "MALFORMED_UPLOAD", "missing plies array");
        return;
    }
    const auto &plies = transcriptObj["plies"];
    if (plies.empty()) {
        sendError(res, 400, "GAME_NOT_COMPLETED", "plies array cannot be empty");
        return;
    }
    if (plies.size() > 1000) {
        sendError(res, 400, "MALFORMED_UPLOAD", "too many plies (> 1000)");
        return;
    }

    int detectedAiDifficulty = 0;
    std::string seatHandle[5] = {"", "Player", "Player", "", ""};
    int64_t seatPlayerId[5] = {0, 0, 0, 0, 0};
    int64_t claimedSeatPlayerId[5] = {0, 0, 0, 0, 0};
    bool seatIsAi[5] = {false, false, false, false, false};
    auto canonicalSeatOf = [](const Json &p) -> int {
        if (!p.is_object()) {
            return 0;
        }
        std::string pStr = p.value("player", "");
        if (pStr == "A") return 1;
        if (pStr == "B") return 2;
        if (pStr == "C") return 3;
        if (pStr == "D") return 4;
        int seat = p.value("seat", 0);
        return (seat >= 1 && seat <= 4) ? seat : 0;
    };
    auto canonicalSideOf = [](int seat) -> const char * {
        switch (seat) {
        case 1: return "A";
        case 2: return "B";
        case 3: return "C";
        case 4: return "D";
        default: return "";
        }
    };
    if (transcriptObj.contains("players") && transcriptObj["players"].is_array()) {
        for (const auto &p : transcriptObj["players"]) {
            if (p.is_object()) {
                int seat = canonicalSeatOf(p);
                std::string h = p.value("handle", "");
                std::string t = p.value("type", "");
                int64_t pid = p.value("playerId", p.value("id", static_cast<int64_t>(0)));
                bool isAi = (t == "AI" || h.rfind("AI_", 0) == 0);
                if (seat >= 1 && seat <= 4) {
                    if (!h.empty()) seatHandle[seat] = h;
                    if (pid > 0) claimedSeatPlayerId[seat] = pid;
                    seatIsAi[seat] = isAi;
                }
                if (t == "AI" || p.value("aiDifficulty", 0) > 0) {
                    int d = p.value("aiDifficulty", 0);
                    if (d >= 1 && d <= 5) detectedAiDifficulty = d;
                }
            }
        }
    }

    for (size_t i = 0; i < plies.size(); ++i) {
        const auto &p = plies[i];
        if (!p.is_object()) {
            sendError(res, 400, "MALFORMED_UPLOAD", "ply must be an object");
            return;
        }
        if (!p.contains("move") || !p["move"].is_string()) {
            sendError(res, 400, "MALFORMED_UPLOAD", "ply missing move");
            return;
        }
        std::string mv = p["move"].get<std::string>();
        if (mv.empty() || mv.size() > 128 || hasControlChars(mv)) {
            sendError(res, 400, "MALFORMED_UPLOAD", "invalid move in ply");
            return;
        }
        if (p.contains("elapsedMs")) {
            if (!p["elapsedMs"].is_number_integer() || p["elapsedMs"].get<int64_t>() < 0) {
                sendError(res, 400, "MALFORMED_UPLOAD", "negative elapsedMs");
                return;
            }
        }
        if (p.contains("clockAfterMs")) {
            if (!p["clockAfterMs"].is_number_integer() || p["clockAfterMs"].get<int64_t>() < -1) {
                sendError(res, 400, "MALFORMED_UPLOAD", "invalid clockAfterMs");
                return;
            }
        }
        if (p.contains("ai") && p["ai"].is_object()) {
            const auto &ai = p["ai"];
            if (ai.contains("difficulty") && ai["difficulty"].is_number_integer()) {
                int d = ai["difficulty"].get<int>();
                if (d < 0 || d > 5) {
                    sendError(res, 400, "MALFORMED_UPLOAD", "invalid ai difficulty");
                    return;
                }
                if (d > 0 && detectedAiDifficulty == 0) detectedAiDifficulty = d;
            }
            if (ai.contains("budgetMs") && (!ai["budgetMs"].is_number_integer() || ai["budgetMs"].get<int64_t>() < 0)) {
                sendError(res, 400, "MALFORMED_UPLOAD", "negative budgetMs");
                return;
            }
            if (ai.contains("depthReached") && (!ai["depthReached"].is_number_integer() || ai["depthReached"].get<int>() < 0)) {
                sendError(res, 400, "MALFORMED_UPLOAD", "negative depthReached");
                return;
            }
            if (ai.contains("nodesVisited") && (!ai["nodesVisited"].is_number_unsigned() && !ai["nodesVisited"].is_number_integer())) {
                sendError(res, 400, "MALFORMED_UPLOAD", "invalid nodesVisited");
                return;
            }
        }
    }

    OctiBoard board(mode, superProngs, edgeless, bases);
    OctiGame *game = board.getGame();
    if (!game) {
        sendError(res, 500, "INTERNAL_ERROR", "failed to initialize game engine");
        return;
    }
    game->start(startPlayer);

    for (size_t i = 0; i < plies.size(); ++i) {
        const auto &p = plies[i];
        std::string plyPlayerStr = p.value("player", "");
        std::string mv = p["move"].get<std::string>();

        OctiPlayer expectedPlayer = game->getTurn();
        std::string expectedPlayerStr;
        switch (expectedPlayer) {
        case PLAYER_A: expectedPlayerStr = "A"; break;
        case PLAYER_B: expectedPlayerStr = "B"; break;
        case PLAYER_C: expectedPlayerStr = "C"; break;
        case PLAYER_D: expectedPlayerStr = "D"; break;
        default: break;
        }
        if (!plyPlayerStr.empty() && plyPlayerStr != expectedPlayerStr) {
            sendError(res, 400, "INVALID_GAME", "turn mismatch in ply " + std::to_string(i + 1));
            return;
        }

        try {
            game->move(expectedPlayer, mv);
        } catch (const IllegalMoveException &e) {
            sendError(res, 400, "INVALID_GAME", std::string("illegal move: ") + e.what());
            return;
        } catch (const IllegalMoveSyntaxException &e) {
            sendError(res, 400, "INVALID_GAME", std::string("illegal syntax: ") + e.what());
            return;
        } catch (const std::exception &e) {
            sendError(res, 400, "INVALID_GAME", std::string("move failed: ") + e.what());
            return;
        }
    }

    if (statusStr == "GAME_OVER") {
        if (!game->isGameOver()) {
            sendError(res, 400, "GAME_NOT_COMPLETED", "game did not reach game over state");
            return;
        }
    } else if (statusStr == "RESIGNED") {
        if (claimedWinner == "NONE") {
            sendError(res, 400, "INVALID_GAME", "resigned game must have a winner");
            return;
        }
        OctiPlayer resWinner = PLAYER_UNDEF;
        if (claimedWinner == "A") resWinner = PLAYER_A;
        else if (claimedWinner == "B") resWinner = PLAYER_B;
        else if (claimedWinner == "C") resWinner = PLAYER_C;
        else if (claimedWinner == "D") resWinner = PLAYER_D;
        game->setResignedWinner(resWinner);
    }

    std::string replayedWinner;
    switch (game->getWinner()) {
    case PLAYER_A: replayedWinner = "A"; break;
    case PLAYER_B: replayedWinner = "B"; break;
    case PLAYER_C: replayedWinner = "C"; break;
    case PLAYER_D: replayedWinner = "D"; break;
    default: replayedWinner = "NONE"; break;
    }
    bool winnerMatches = false;
    if (mode == GAME_MODE_4) {
        if ((replayedWinner == "A" || replayedWinner == "C") &&
            (normalizedClaimedWinner == "A" || normalizedClaimedWinner == "C")) {
            winnerMatches = true;
        } else if ((replayedWinner == "B" || replayedWinner == "D") &&
                   (normalizedClaimedWinner == "B" || normalizedClaimedWinner == "D")) {
            winnerMatches = true;
        } else if (replayedWinner == "NONE" && normalizedClaimedWinner == "NONE") {
            winnerMatches = true;
        }
    } else {
        winnerMatches = (claimedWinner == replayedWinner);
    }
    if (!winnerMatches) {
        sendError(res, 400, "INVALID_GAME", "claimed winner does not match replay");
        return;
    }

    std::string replayedFinalFen = board.toPositionString();
    if (claimedFinalFen != replayedFinalFen) {
        sendError(res, 400, "INVALID_GAME", "claimed finalFen does not match replay");
        return;
    }

    std::string serverGameId = board.computeCanonicalGameId();

    Json storedJson = transcriptObj;
    if (storedJson.contains("client") && storedJson["client"].is_object()) {
        storedJson["client"].erase("extended");
    }
    auto normalizePlayersArr = [&](Json &arr) {
        if (!arr.is_array()) {
            return;
        }
        for (auto &pEntry : arr) {
            if (!pEntry.is_object()) {
                continue;
            }
            int s = canonicalSeatOf(pEntry);
            if (s >= 1 && s <= 4) {
                pEntry["seat"] = s;
                const char *side = canonicalSideOf(s);
                if (side[0] != '\0') {
                    pEntry["player"] = side;
                }
            }
        }
        std::vector<Json> sorted(arr.begin(), arr.end());
        std::stable_sort(sorted.begin(), sorted.end(),
                         [](const Json &a, const Json &b) {
                             int sa = a.is_object() ? a.value("seat", 99) : 99;
                             int sb = b.is_object() ? b.value("seat", 99) : 99;
                             return sa < sb;
                         });
        arr = Json::array();
        for (auto &item : sorted) {
            arr.push_back(std::move(item));
        }
    };
    if (storedJson.contains("players")) {
        normalizePlayersArr(storedJson["players"]);
    }
    if (storedJson.contains("game") && storedJson["game"].is_object() &&
        storedJson["game"].contains("players")) {
        normalizePlayersArr(storedJson["game"]["players"]);
    }
    storedJson["gameId"] = serverGameId;
    storedJson["finalFen"] = replayedFinalFen;
    storedJson["license"] = "CC-BY-4.0";
    storedJson["attribution"] = OCTI_DATA_ATTRIBUTION;
    storedJson["winner"] = replayedWinner;
    storedJson["status"] = statusStr;

    int64_t now = epochSec();
    int64_t startedAt = now;
    int64_t endedAt = now;
    if (transcriptObj.contains("client") && transcriptObj["client"].is_object() &&
        transcriptObj["client"].contains("extended") && transcriptObj["client"]["extended"].is_object()) {
        const auto &ext = transcriptObj["client"]["extended"];
        if (ext.contains("startedAt") && ext["startedAt"].is_number_integer() && ext["startedAt"].get<int64_t>() > 0) {
            startedAt = ext["startedAt"].get<int64_t>();
        }
        if (ext.contains("endedAt") && ext["endedAt"].is_number_integer() && ext["endedAt"].get<int64_t>() > 0) {
            endedAt = ext["endedAt"].get<int64_t>();
        }
    }
    if (endedAt < startedAt) {
        endedAt = startedAt;
    }

    GameRecord rec;
    rec.gameId = serverGameId;
    rec.gameMode = modeStr;
    rec.gameBases = basesVal;
    rec.superProngs = superProngs;
    rec.edgeless = edgeless;
    rec.timeControl = transcriptObj.value("timeControl", "untimed");
    rec.p1PlayerId = seatPlayerId[1];
    rec.p2PlayerId = seatPlayerId[2];
    rec.p1Handle = seatHandle[1];
    rec.p2Handle = seatHandle[2];
    if (mode == GAME_MODE_4) {
        rec.p3PlayerId = seatPlayerId[3];
        rec.p3Handle = seatHandle[3].empty() ? "Player" : seatHandle[3];
        rec.p4PlayerId = seatPlayerId[4];
        rec.p4Handle = seatHandle[4].empty() ? "Player" : seatHandle[4];
    } else {
        rec.p3PlayerId = 0;
        rec.p4PlayerId = 0;
        rec.p3Handle = "";
        rec.p4Handle = "";
    }
    if (mode == GAME_MODE_4) {
        if (replayedWinner == "A" || replayedWinner == "C") {
            rec.result = "TEAM_13_WIN";
            rec.winnerSeat = (replayedWinner == "C") ? 3 : 1;
        } else if (replayedWinner == "B" || replayedWinner == "D") {
            rec.result = "TEAM_24_WIN";
            rec.winnerSeat = (replayedWinner == "D") ? 4 : 2;
        } else {
            rec.result = "DRAW";
            rec.winnerSeat = 0;
        }
    } else {
        rec.result = (replayedWinner == "A") ? "P1_WIN" : (replayedWinner == "B" ? "P2_WIN" : "DRAW");
        if (replayedWinner == "A") rec.winnerSeat = 1;
        else if (replayedWinner == "B") rec.winnerSeat = 2;
        else rec.winnerSeat = 0;
    }

    std::string term = transcriptObj.value("termination", "");
    if (term == "FORFEIT") term = "RESIGNATION";

    if (statusStr == "RESIGNED") {
        if (term.empty()) {
            term = "RESIGNATION";
        }
    } else {
        std::string ruleTerm = classifyRuleTermination(board, *game);
        if (term.empty()) {
            term = ruleTerm;
        } else if (term == "BASE_CAPTURE" || term == "BASE_OCCUPATION" ||
                   term == "TOTAL_CAPTURE" || term == "ELIMINATION" ||
                   term == "STALEMATE" || term == "NO_PROGRESS_DRAW") {
            if (ruleTerm == "BASE_CAPTURE" && (term == "BASE_CAPTURE" || term == "BASE_OCCUPATION")) {
                // Keep client's preferred BASE_CAPTURE / BASE_OCCUPATION synonym
            } else if (ruleTerm == "TOTAL_CAPTURE" && (term == "TOTAL_CAPTURE" || term == "ELIMINATION")) {
                // Keep client's preferred TOTAL_CAPTURE / ELIMINATION synonym
            } else {
                term = ruleTerm;
            }
        }
    }
    if (term != "BASE_CAPTURE" && term != "BASE_OCCUPATION" &&
        term != "TOTAL_CAPTURE" && term != "ELIMINATION" &&
        term != "STALEMATE" && term != "NO_PROGRESS_DRAW" &&
        term != "RESIGNATION" &&
        term != "TIMEOUT" && term != "DISCONNECT_FORFEIT" &&
        term != "AGREED_DRAW") {
        term = (statusStr == "RESIGNED" ? "RESIGNATION" : classifyRuleTermination(board, *game));
    }
    rec.termination = term;
    storedJson["termination"] = term;

    rec.plyCount = static_cast<int>(plies.size());
    rec.octiTranscript = storedJson.dump();
    rec.finalFen = replayedFinalFen;
    rec.startedAt = startedAt;
    rec.endedAt = endedAt;
    rec.source = "client_upload";
    rec.uploadedAt = now;
    rec.winner = replayedWinner;
    rec.status = statusStr;
    rec.aiDifficulty = detectedAiDifficulty;
    rec.appVersion = appVersion;
    rec.platform = platform;
    rec.installId = installId;

    if (hasAuthedPlayer && authedPlayer.playerId > 0) {
        if (!installId.empty() && isSafeId(installId)) {
            store.bindInstallToPlayer(installId, authedPlayer.playerId);
        }
        int maxSeats = (mode == GAME_MODE_4) ? 4 : 2;
        int humanSeat = 0;
        for (int s = 1; s <= maxSeats; ++s) {
            if (claimedSeatPlayerId[s] == authedPlayer.playerId ||
                (seatHandle[s] == authedPlayer.handle && authedPlayer.handle != "Player")) {
                humanSeat = s;
                break;
            }
        }
        if (humanSeat == 0) {
            for (int s = 1; s <= maxSeats; ++s) {
                if (!seatIsAi[s] && seatHandle[s].rfind("AI_", 0) != 0) {
                    humanSeat = s;
                    break;
                }
            }
        }
        if (humanSeat == 0) humanSeat = 1;

        for (int s = 1; s <= 4; ++s) {
            seatPlayerId[s] = 0;
        }
        seatPlayerId[humanSeat] = authedPlayer.playerId;
        seatHandle[humanSeat] = authedPlayer.handle;

        rec.p1PlayerId = seatPlayerId[1];
        rec.p1Handle = seatHandle[1];
        rec.p2PlayerId = seatPlayerId[2];
        rec.p2Handle = seatHandle[2];
        if (mode == GAME_MODE_4) {
            rec.p3PlayerId = seatPlayerId[3];
            rec.p3Handle = seatHandle[3];
            rec.p4PlayerId = seatPlayerId[4];
            rec.p4Handle = seatHandle[4];
        }

        auto updateHumanEntry = [&](Json &arr) {
            if (!arr.is_array()) {
                return;
            }
            for (auto &pEntry : arr) {
                if (!pEntry.is_object()) continue;
                int s = canonicalSeatOf(pEntry);
                std::string t = pEntry.value("type", "");
                std::string h = pEntry.value("handle", "");
                if (s == humanSeat || (s == 0 && (t == "HUMAN" || h.rfind("AI_", 0) != 0))) {
                    pEntry["handle"] = authedPlayer.handle;
                    pEntry["player_uuid"] = authedPlayer.uuid;
                } else {
                    pEntry.erase("player_uuid");
                    pEntry.erase("playerId");
                }
            }
        };
        if (storedJson.contains("players")) {
            updateHumanEntry(storedJson["players"]);
        }
        if (storedJson.contains("game") && storedJson["game"].is_object() &&
            storedJson["game"].contains("players")) {
            updateHumanEntry(storedJson["game"]["players"]);
        }
        rec.octiTranscript = storedJson.dump();
    } else {
        auto stripPlayerIdentity = [](Json &arr) {
            if (!arr.is_array()) {
                return;
            }
            for (auto &pEntry : arr) {
                if (!pEntry.is_object()) continue;
                pEntry.erase("player_uuid");
                pEntry.erase("playerId");
            }
        };
        if (storedJson.contains("players")) {
            stripPlayerIdentity(storedJson["players"]);
        }
        if (storedJson.contains("game") && storedJson["game"].is_object() &&
            storedJson["game"].contains("players")) {
            stripPlayerIdentity(storedJson["game"]["players"]);
        }
        rec.octiTranscript = storedJson.dump();
    }

    if (detectedAiDifficulty == 1 || detectedAiDifficulty == 2) {
        Json resp = {
            {"ok", true},
            {"ignored", true},
            {"game_id", rec.gameId},
            {"ply_count", rec.plyCount}
        };
        sendJson(res, 200, resp);
        return;
    }

    std::string archiveErr;
    ArchiveStatus st = store.archiveGame(rec, archiveErr);
    if (st == ArchiveStatus::DUPLICATE_ID) {
        sendError(res, 409, "DUPLICATE_ID", "game already archived");
        return;
    }
    if (st != ArchiveStatus::OK) {
        sendError(res, 500, "INTERNAL_ERROR", "archive failed: " + archiveErr);
        return;
    }

    Json resp;
    resp["ok"] = true;
    resp["game_id"] = rec.gameId;
    resp["ply_count"] = rec.plyCount;
    sendJson(res, 200, resp);
}

void OctiHttpServer::Impl::handleArchiveDeleteInstall(const Request &req, Response &res)
{
    std::string installId;
    if (!pathId(req, "installId", res, installId)) {
        return;
    }
    if (installId.empty() || installId.size() > 64 || !isSafeId(installId)) {
        sendError(res, 400, "INVALID_ARGUMENT", "invalid installId");
        return;
    }
    Auth auth;
    if (!optionalAuth(req, res, auth) ||
        !allowMutationAttempt(req, res, auth.playerId)) {
        return;
    }
    const int64_t authedHumanId = (!auth.isRobot && auth.playerId > 0) ? auth.playerId : 0;
    std::string installToken;
    if (req.has_header("X-Octi-Install-Token")) {
        installToken = req.get_header_value("X-Octi-Install-Token");
    }
    if (installToken.empty() && authedHumanId <= 0) {
        if (auth.isRobot) {
            sendError(res, 403, "FORBIDDEN",
                      "robots cannot delete install archive data");
            return;
        }
        sendAuthUnauthorized(
            req, res, "Bearer",
            "missing X-Octi-Install-Token or Authorization: Bearer header",
            false);
        return;
    }
    InstallDeleteAuthStatus authSt =
        store.verifyInstallDeleteAuth(installId, installToken, authedHumanId);
    if (authSt == InstallDeleteAuthStatus::UNAUTHORIZED) {
        sendAuthUnauthorized(req, res, "Bearer",
                             "missing install delete credentials", false);
        return;
    }
    if (authSt != InstallDeleteAuthStatus::AUTHORIZED) {
        sendError(res, 403, "FORBIDDEN",
                  "not authorized to delete data for this installId");
        return;
    }
    int deleted = store.deleteGamesByInstallId(installId);
    if (deleted < 0) {
        sendError(res, 500, "INTERNAL_ERROR", "failed to delete games");
        return;
    }
    Json j;
    j["ok"] = true;
    j["deleted"] = deleted;
    sendJson(res, 200, j);
}

void OctiHttpServer::Impl::handleArchivePurgeGame(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!requireAuth(req, res, auth) || !pathId(req, "id", res, id) ||
        !allowMutationAttempt(req, res, auth.playerId)) {
        return;
    }
    if (auth.isRobot || !store.isPlayerAdmin(auth.playerId)) {
        sendError(res, 403, "FORBIDDEN", "admin privileges required");
        return;
    }
    std::string err;
    if (!store.purgeArchivedGame(id, err)) {
        if (err == "game_not_found") {
            sendError(res, 404, "NOT_FOUND", "archived game not found");
        } else {
            sendError(res, 500, "INTERNAL_ERROR", "failed to purge archived game: " + err);
        }
        return;
    }
    rooms.refreshCachedPlayerStats();
    Json j;
    j["ok"] = true;
    j["status"] = "purged";
    j["game_id"] = id;
    sendJson(res, 200, j);
}

void OctiHttpServer::Impl::handleAdminRecomputeRatings(const Request &req,
                                                       Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth) ||
        !allowMutationAttempt(req, res, auth.playerId)) {
        return;
    }
    if (auth.isRobot || !store.isPlayerAdmin(auth.playerId)) {
        sendError(res, 403, "FORBIDDEN", "admin privileges required");
        return;
    }
    if (!req.body.empty()) {
        Json body;
        if (!readJsonBody(req, res, body, true)) {
            return;
        }
    }
    RecomputeRatingsResult result;
    std::string err;
    if (!store.recomputeAllRatings(result, err)) {
        sendError(res, 500, "INTERNAL_ERROR",
                  "failed to recompute ratings: " + err);
        return;
    }
    rooms.refreshCachedPlayerStats();
    Json j;
    j["ok"] = true;
    j["games_scanned"] = result.gamesScanned;
    j["pool1_games_rated"] = result.pool1GamesRated;
    j["pool2_games_rated"] = result.pool2GamesRated;
    j["players_updated"] = result.playersUpdated;
    j["robots_updated"] = result.robotsUpdated;
    sendJson(res, 200, j);
}

bool OctiHttpServer::Impl::requireAdminAuth(const Request &req, Response &res,
                                            Auth &auth,
                                            std::string &actorLabelOut)
{
    res.set_header("Cache-Control", "no-store");
    if (!requireAuth(req, res, auth)) {
        return false;
    }
    if (auth.isRobot || !store.isPlayerAdmin(auth.playerId)) {
        sendError(res, 403, "FORBIDDEN", "admin privileges required");
        return false;
    }
    PlayerProfile prof;
    if (store.getProfileById(auth.playerId, prof) && !prof.handle.empty()) {
        actorLabelOut = "@" + prof.handle;
    } else if (!prof.uuid.empty()) {
        actorLabelOut = prof.uuid;
    } else {
        actorLabelOut = "admin:" + std::to_string(auth.playerId);
    }
    return true;
}

Json OctiHttpServer::Impl::buildBansPayloadJson(int64_t nowSec)
{
    std::vector<BanEntry> tempBans;
    std::vector<BanEntry> permBans;
    banManager.listBans(store, nowSec, tempBans, permBans);

    Json tempArr = Json::array();
    for (size_t i = 0; i < tempBans.size(); ++i) {
        const BanEntry &e = tempBans[i];
        Json item;
        item["ban_id"] = e.banId;
        item["target_type"] = toString(e.targetType);
        item["target_value"] = e.targetValue;
        item["display_label"] =
            e.displayLabel.empty() ? e.targetValue : e.displayLabel;
        item["reason"] = e.reason;
        item["source"] = e.source.empty() ? "admin" : e.source;
        item["created_by"] = e.createdBy;
        item["created_at"] = e.createdAt;
        item["expires_at"] = e.expiresAt;
        item["remaining_sec"] = std::max<int64_t>(0, e.expiresAt - nowSec);
        tempArr.push_back(std::move(item));
    }

    Json permArr = Json::array();
    for (size_t i = 0; i < permBans.size(); ++i) {
        const BanEntry &e = permBans[i];
        Json item;
        item["ban_id"] = e.banId;
        item["target_type"] = toString(e.targetType);
        item["target_value"] = e.targetValue;
        item["display_label"] =
            e.displayLabel.empty() ? e.targetValue : e.displayLabel;
        item["reason"] = e.reason;
        item["created_by"] = e.createdBy;
        item["created_at"] = e.createdAt;
        permArr.push_back(std::move(item));
    }

    Json j;
    j["ok"] = true;
    j["server_time"] = nowSec;
    j["ban_config_path"] = banManager.banConfigPath();
    j["temp_bans"] = std::move(tempArr);
    j["permanent_bans"] = std::move(permArr);
    return j;
}

void OctiHttpServer::Impl::handleAdminBansList(const Request &req,
                                               Response &res)
{
    Auth auth;
    std::string actorLabel;
    if (!requireAdminAuth(req, res, auth, actorLabel) ||
        !allowBrowseRead(req, res, auth)) {
        return;
    }
    sendJson(res, 200, buildBansPayloadJson(store.currentEpochSec()));
}

void OctiHttpServer::Impl::handleAdminBanAdd(const Request &req, Response &res)
{
    Auth auth;
    std::string actorLabel;
    if (!requireAdminAuth(req, res, auth, actorLabel) ||
        !allowMutationAttempt(req, res, auth.playerId)) {
        return;
    }
    Json body;
    if (!readJsonBody(req, res, body, false)) {
        return;
    }
    std::string rawType;
    std::string rawTarget;
    std::string rawDuration = "24h";
    std::string rawReason;
    std::string err;
    if (!fieldString(body, "target_type", true, rawType, err) ||
        !fieldString(body, "target", true, rawTarget, err) ||
        !fieldString(body, "duration", false, rawDuration, err) ||
        !fieldString(body, "reason", false, rawReason, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    BanTargetType tType = BanTargetType::IP;
    if (!parseBanTargetType(rawType, tType)) {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "target_type must be ip, account, or robot");
        return;
    }
    BanDuration dur = BanDuration::TEMP_24H;
    if (!rawDuration.empty() && !parseBanDuration(rawDuration, dur)) {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "duration must be 24h or permanent");
        return;
    }

    const int64_t nowSec = store.currentEpochSec();
    BanOpResult op = banManager.addBan(store, tType, rawTarget, dur, rawReason,
                                       auth.playerId, actorLabel,
                                       clientRemoteIp(req), nowSec);
    if (op.status != BanOpStatus::OK) {
        int httpCode = 400;
        if (op.status == BanOpStatus::NOT_FOUND) {
            httpCode = 404;
        } else if (op.status == BanOpStatus::IO_ERROR) {
            httpCode = 500;
        }
        sendError(res, httpCode,
                  op.errorCode.empty() ? "BAD_REQUEST" : op.errorCode,
                  op.errorMessage);
        return;
    }

    applyBanSideEffects(op.sideEffects);
    Json j = buildBansPayloadJson(nowSec);
    j["ban_id"] = op.entry.banId;
    sendJson(res, 200, j);
}

void OctiHttpServer::Impl::handleAdminBansMove(const Request &req,
                                               Response &res)
{
    Auth auth;
    std::string actorLabel;
    if (!requireAdminAuth(req, res, auth, actorLabel) ||
        !allowMutationAttempt(req, res, auth.playerId)) {
        return;
    }
    Json body;
    if (!readJsonBody(req, res, body, false)) {
        return;
    }
    if (!body.contains("ban_ids") || !body["ban_ids"].is_array() ||
        body["ban_ids"].empty() || body["ban_ids"].size() > 256) {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "ban_ids must be a non-empty array of up to 256 ban IDs");
        return;
    }
    std::vector<std::string> ids;
    ids.reserve(body["ban_ids"].size());
    for (size_t i = 0; i < body["ban_ids"].size(); ++i) {
        if (!body["ban_ids"][i].is_string()) {
            sendError(res, 400, "INVALID_ARGUMENT",
                      "every entry in ban_ids must be a string");
            return;
        }
        const std::string bid = body["ban_ids"][i].get<std::string>();
        if (!isSafeId(bid)) {
            sendError(res, 400, "INVALID_ARGUMENT", "invalid ban_id");
            return;
        }
        ids.push_back(bid);
    }

    std::string rawDuration;
    std::string err;
    if (!fieldString(body, "target_duration", true, rawDuration, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    BanDuration targetDur = BanDuration::TEMP_24H;
    if (!parseBanDuration(rawDuration, targetDur)) {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "target_duration must be 24h or permanent");
        return;
    }

    const int64_t nowSec = store.currentEpochSec();
    BanOpResult op = banManager.moveBans(store, ids, targetDur, actorLabel,
                                         nowSec);
    if (op.status != BanOpStatus::OK) {
        int httpCode = 400;
        if (op.status == BanOpStatus::NOT_FOUND) {
            httpCode = 404;
        } else if (op.status == BanOpStatus::IO_ERROR) {
            httpCode = 500;
        }
        sendError(res, httpCode,
                  op.errorCode.empty() ? "BAD_REQUEST" : op.errorCode,
                  op.errorMessage);
        return;
    }
    applyBanSideEffects(op.sideEffects);
    sendJson(res, 200, buildBansPayloadJson(nowSec));
}

void OctiHttpServer::Impl::handleAdminBanPatch(const Request &req,
                                               Response &res)
{
    Auth auth;
    std::string actorLabel;
    std::string id;
    if (!requireAdminAuth(req, res, auth, actorLabel) ||
        !pathId(req, "id", res, id) ||
        !allowMutationAttempt(req, res, auth.playerId)) {
        return;
    }
    Json body;
    if (!readJsonBody(req, res, body, false)) {
        return;
    }
    std::string rawDuration;
    std::string err;
    if (fieldPresent(body, "target_duration")) {
        if (!fieldString(body, "target_duration", true, rawDuration, err)) {
            sendError(res, 400, "INVALID_ARGUMENT", err);
            return;
        }
    } else if (!fieldString(body, "duration", true, rawDuration, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    BanDuration targetDur = BanDuration::TEMP_24H;
    if (!parseBanDuration(rawDuration, targetDur)) {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "target_duration must be 24h or permanent");
        return;
    }

    const int64_t nowSec = store.currentEpochSec();
    std::vector<std::string> ids = {id};
    BanOpResult op = banManager.moveBans(store, ids, targetDur, actorLabel,
                                         nowSec);
    if (op.status != BanOpStatus::OK) {
        int httpCode = 400;
        if (op.status == BanOpStatus::NOT_FOUND) {
            httpCode = 404;
        } else if (op.status == BanOpStatus::IO_ERROR) {
            httpCode = 500;
        }
        sendError(res, httpCode,
                  op.errorCode.empty() ? "BAD_REQUEST" : op.errorCode,
                  op.errorMessage);
        return;
    }
    applyBanSideEffects(op.sideEffects);
    sendJson(res, 200, buildBansPayloadJson(nowSec));
}

void OctiHttpServer::Impl::handleAdminBansRemove(const Request &req,
                                                 Response &res)
{
    Auth auth;
    std::string actorLabel;
    if (!requireAdminAuth(req, res, auth, actorLabel) ||
        !allowMutationAttempt(req, res, auth.playerId)) {
        return;
    }
    Json body;
    if (!readJsonBody(req, res, body, false)) {
        return;
    }
    if (!body.contains("ban_ids") || !body["ban_ids"].is_array() ||
        body["ban_ids"].empty() || body["ban_ids"].size() > 256) {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "ban_ids must be a non-empty array of up to 256 ban IDs");
        return;
    }
    std::vector<std::string> ids;
    ids.reserve(body["ban_ids"].size());
    for (size_t i = 0; i < body["ban_ids"].size(); ++i) {
        if (!body["ban_ids"][i].is_string()) {
            sendError(res, 400, "INVALID_ARGUMENT",
                      "every entry in ban_ids must be a string");
            return;
        }
        const std::string bid = body["ban_ids"][i].get<std::string>();
        if (!isSafeId(bid)) {
            sendError(res, 400, "INVALID_ARGUMENT", "invalid ban_id");
            return;
        }
        ids.push_back(bid);
    }

    const int64_t nowSec = store.currentEpochSec();
    BanOpResult op = banManager.removeBans(store, ids, actorLabel, nowSec);
    if (op.status != BanOpStatus::OK) {
        int httpCode = 400;
        if (op.status == BanOpStatus::NOT_FOUND) {
            httpCode = 404;
        } else if (op.status == BanOpStatus::IO_ERROR) {
            httpCode = 500;
        }
        sendError(res, httpCode,
                  op.errorCode.empty() ? "BAD_REQUEST" : op.errorCode,
                  op.errorMessage);
        return;
    }
    applyBanSideEffects(op.sideEffects);
    sendJson(res, 200, buildBansPayloadJson(nowSec));
}

void OctiHttpServer::Impl::handleAdminBanDelete(const Request &req,
                                                Response &res)
{
    Auth auth;
    std::string actorLabel;
    std::string id;
    if (!requireAdminAuth(req, res, auth, actorLabel) ||
        !pathId(req, "id", res, id) ||
        !allowMutationAttempt(req, res, auth.playerId)) {
        return;
    }
    const int64_t nowSec = store.currentEpochSec();
    std::vector<std::string> ids = {id};
    BanOpResult op = banManager.removeBans(store, ids, actorLabel, nowSec);
    if (op.status != BanOpStatus::OK) {
        int httpCode = 400;
        if (op.status == BanOpStatus::NOT_FOUND) {
            httpCode = 404;
        } else if (op.status == BanOpStatus::IO_ERROR) {
            httpCode = 500;
        }
        sendError(res, httpCode,
                  op.errorCode.empty() ? "BAD_REQUEST" : op.errorCode,
                  op.errorMessage);
        return;
    }
    applyBanSideEffects(op.sideEffects);
    sendJson(res, 200, buildBansPayloadJson(nowSec));
}

void OctiHttpServer::Impl::handleArchiveList(const Request &req,
                                             Response &res)
{
    Auth auth;
    if (!optionalAuth(req, res, auth) || !allowBrowseRead(req, res, auth)) {
        return;
    }
    ArchiveFilter f;
    if (req.has_param("mode")) {
        std::string mode = upper(req.get_param_value("mode"));
        if (!mode.empty()) {
            if (mode == "2P" || mode == "2P_ALL") {
                f.mode = "2P";
            } else if (mode == "4P" || mode == "4P_ALL") {
                f.mode = "4P";
            } else if (mode == "OFK") {
                f.mode = "OFK";
            } else if (isValidOctiVariantCode(mode)) {
                f.variant = mode;
            } else {
                sendError(res, 400, "INVALID_ARGUMENT",
                          "mode must be 2P, 4P, OFK, or a canonical variant code");
                return;
            }
        }
    }
    if (req.has_param("variant")) {
        std::string var = upper(req.get_param_value("variant"));
        if (!var.empty()) {
            if (var == "2P" || var == "2P_ALL") {
                f.mode = "2P";
            } else if (var == "4P_ALL") {
                f.mode = "4P";
            } else if (isValidOctiVariantCode(var)) {
                f.variant = var;
            } else {
                sendError(res, 400, "INVALID_ARGUMENT",
                          "variant must be one of the 13 canonical OCTI variant codes");
                return;
            }
        }
    }
    if (req.has_param("player")) {
        f.playerUuid = req.get_param_value("player");
        if (!isSafeId(f.playerUuid)) {
            sendError(res, 400, "INVALID_ARGUMENT", "invalid player id");
            return;
        }
    }
    if (req.has_param("robot")) {
        f.robotUuid = req.get_param_value("robot");
        if (!isSafeId(f.robotUuid)) {
            sendError(res, 400, "INVALID_ARGUMENT", "invalid robot id");
            return;
        }
    }
    if (req.has_param("source")) {
        std::string src = lower(req.get_param_value("source"));
        if (src != "online" && src != "client_upload") {
            sendError(res, 400, "INVALID_ARGUMENT", "source must be online or client_upload");
            return;
        }
        f.source = src;
    }
    if (req.has_param("status")) {
        std::string st = upper(req.get_param_value("status"));
        if (st != "GAME_OVER" && st != "RESIGNED") {
            sendError(res, 400, "INVALID_ARGUMENT", "status must be GAME_OVER or RESIGNED");
            return;
        }
        f.status = st;
    }
    if (req.has_param("winner")) {
        std::string w = upper(req.get_param_value("winner"));
        if (w != "A" && w != "B" && w != "C" && w != "D" && w != "NONE") {
            sendError(res, 400, "INVALID_ARGUMENT", "winner must be A, B, C, D, or NONE");
            return;
        }
        f.winner = w;
    }
    if (req.has_param("aiDifficulty") || req.has_param("ai_difficulty")) {
        std::string dStr = req.has_param("aiDifficulty") ? req.get_param_value("aiDifficulty") : req.get_param_value("ai_difficulty");
        int64_t diff = 0;
        if (!parseUnsignedClamped(dStr, 5, diff) || diff < 1 || diff > 5) {
            sendError(res, 400, "INVALID_ARGUMENT", "aiDifficulty must be 1..5");
            return;
        }
        f.aiDifficulty = static_cast<int>(diff);
    }
    if (req.has_param("appVersion") || req.has_param("app_version")) {
        std::string av = req.has_param("appVersion") ? req.get_param_value("appVersion") : req.get_param_value("app_version");
        if (av.size() > 128 || hasControlChars(av)) {
            sendError(res, 400, "INVALID_ARGUMENT", "invalid appVersion");
            return;
        }
        f.appVersion = av;
    }
    if (req.has_param("platform")) {
        std::string plat = req.get_param_value("platform");
        if (plat.size() > 128 || hasControlChars(plat)) {
            sendError(res, 400, "INVALID_ARGUMENT", "invalid platform");
            return;
        }
        f.platform = plat;
    }
    if (req.has_param("from")) {
        int64_t fromSec = 0;
        if (!parseUnsignedClamped(req.get_param_value("from"), INT64_MAX, fromSec)) {
            sendError(res, 400, "INVALID_ARGUMENT", "from must be a non-negative integer");
            return;
        }
        f.fromSec = fromSec;
    }
    if (req.has_param("to")) {
        int64_t toSec = 0;
        if (!parseUnsignedClamped(req.get_param_value("to"), INT64_MAX, toSec)) {
            sendError(res, 400, "INVALID_ARGUMENT", "to must be a non-negative integer");
            return;
        }
        f.toSec = toSec;
    }
    if (req.has_param("sort")) {
        std::string sVal = req.get_param_value("sort");
        std::string field = sVal;
        std::string dir = "";
        size_t colon = sVal.find(':');
        if (colon != std::string::npos) {
            field = sVal.substr(0, colon);
            dir = lower(sVal.substr(colon + 1));
        }
        if (field != "ended_at" && field != "uploaded_at" && field != "ply_count") {
            sendError(res, 400, "INVALID_ARGUMENT", "invalid sort field");
            return;
        }
        f.sortField = field;
        if (dir == "asc") {
            f.sortAsc = true;
        } else if (dir == "desc") {
            f.sortAsc = false;
        }
    }
    if (req.has_param("order")) {
        std::string ord = lower(req.get_param_value("order"));
        if (ord == "asc") {
            f.sortAsc = true;
        } else if (ord == "desc") {
            f.sortAsc = false;
        } else {
            sendError(res, 400, "INVALID_ARGUMENT", "order must be asc or desc");
            return;
        }
    }

    int64_t limit = 20;
    int64_t offset = 0;
    if ((req.has_param("limit") &&
         !parseUnsignedClamped(req.get_param_value("limit"), INT32_MAX,
                               limit)) ||
        (req.has_param("offset") &&
         !parseUnsignedClamped(req.get_param_value("offset"), INT32_MAX,
                               offset))) {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "limit and offset must be non-negative integers");
        return;
    }
    limit = std::max<int64_t>(1, std::min<int64_t>(limit,
                                                    AccountStore::kMaxArchiveLimit));
    f.limit = static_cast<int>(limit);
    f.offset = static_cast<int>(offset);
    std::vector<GameRecord> games;
    int64_t total = 0;
    if (!store.listArchivedGames(f, games, &total)) {
        sendError(res, 500, "INTERNAL_ERROR", "archive query failed");
        return;
    }
    Json arr = Json::array();
    for (size_t i = 0; i < games.size(); ++i) {
        arr.push_back(gameSummaryJson(games[i]));
    }
    Json j;
    j["total"] = total;
    j["limit"] = limit;
    j["offset"] = offset;
    j["games"] = arr;
    sendJson(res, 200, j);
}

void OctiHttpServer::Impl::handleArchiveGet(const Request &req, Response &res)
{
    Auth auth;
    if (!optionalAuth(req, res, auth) || !allowBrowseRead(req, res, auth)) {
        return;
    }
    std::string id;
    if (!pathId(req, "id", res, id)) {
        return;
    }
    GameRecord rec;
    if (!store.getArchivedGame(id, rec)) {
        if (auth.playerId == 0) {
            applyBanSideEffects(banManager.recordAnonymousEnumeration(
                store, clientRemoteIp(req), req.path, 404,
                store.currentEpochSec()));
        }
        sendError(res, 404, "NOT_FOUND", "no such archived game");
        return;
    }
    if (auth.playerId == 0) {
        applyBanSideEffects(banManager.recordAnonymousEnumeration(
            store, clientRemoteIp(req), req.path, 200,
            store.currentEpochSec()));
    }
    Json j = gameSummaryJson(rec);
    j["final_fen"] = rec.finalFen;
    std::string sanitized = sanitizeTranscriptJson(rec.octiTranscript, true);
    j["transcript_json"] = sanitized;
    j["octi_transcript"] = sanitized;
    j["license"] = "CC-BY-4.0";
    j["attribution"] = OCTI_DATA_ATTRIBUTION;
    sendJson(res, 200, j);
}

void OctiHttpServer::Impl::handleArchiveTranscript(const Request &req,
                                                   Response &res)
{
    Auth auth;
    if (!optionalAuth(req, res, auth) || !allowBrowseRead(req, res, auth)) {
        return;
    }
    std::string id;
    if (!pathId(req, "id", res, id)) {
        return;
    }
    std::string transcript;
    if (!store.getArchivedTranscript(id, transcript)) {
        if (auth.playerId == 0) {
            applyBanSideEffects(banManager.recordAnonymousEnumeration(
                store, clientRemoteIp(req), req.path, 404,
                store.currentEpochSec()));
        }
        sendError(res, 404, "NOT_FOUND", "no such archived game");
        return;
    }
    if (auth.playerId == 0) {
        applyBanSideEffects(banManager.recordAnonymousEnumeration(
            store, clientRemoteIp(req), req.path, 200,
            store.currentEpochSec()));
    }
    std::string sanitized = sanitizeTranscriptJson(transcript, true);
    res.status = 200;
    res.set_header("Cache-Control", "no-store");
    res.set_header("Content-Disposition",
                   "attachment; filename=\"" + id + ".json\"");
    res.set_content(sanitized, "application/json; charset=utf-8");
}

void OctiHttpServer::Impl::handleArchiveOcti(const Request &req,
                                             Response &res)
{
    handleArchiveTranscript(req, res);
}

void OctiHttpServer::Impl::handleArchivePlies(const Request &req, Response &res)
{
    Auth auth;
    if (!optionalAuth(req, res, auth) || !allowPliesAttempt(req, res)) {
        return;
    }
    std::string id;
    if (!pathId(req, "id", res, id)) {
        return;
    }
    const uint64_t storeRev = store.mutationRevision();
    const int64_t nowMs = steadyMs();
    CachedPayload pliesCached;
    if (pliesCache.get(id, storeRev, nowMs, pliesCached)) {
        if (auth.playerId == 0) {
            applyBanSideEffects(banManager.recordAnonymousEnumeration(
                store, clientRemoteIp(req), req.path, 200,
                store.currentEpochSec()));
        }
        sendCachedSnapshot(req, res, pliesCached, "no-cache");
        return;
    }
    std::string transcript;
    if (!store.getArchivedTranscript(id, transcript)) {
        if (auth.playerId == 0) {
            applyBanSideEffects(banManager.recordAnonymousEnumeration(
                store, clientRemoteIp(req), req.path, 404,
                store.currentEpochSec()));
        }
        sendError(res, 404, "NOT_FOUND", "no such archived game");
        return;
    }
    if (auth.playerId == 0) {
        applyBanSideEffects(banManager.recordAnonymousEnumeration(
            store, clientRemoteIp(req), req.path, 200,
            store.currentEpochSec()));
    }

    Json tObj;
    try {
        tObj = Json::parse(transcript);
        if (!tObj.is_object() || tObj.value("format", "") != "octi.transcript.v3") {
            sendError(res, 500, "INTERNAL_ERROR", "corrupt transcript");
            return;
        }
    } catch (...) {
        sendError(res, 500, "INTERNAL_ERROR", "corrupt transcript json");
        return;
    }

    std::string modeStr = tObj.value("mode", "2P");
    OctiGameMode mode = GAME_MODE_2;
    if (modeStr == "4P") mode = GAME_MODE_4;
    else if (modeStr == "OFK") mode = GAME_MODE_OFK;

    int basesVal = tObj.value("bases", 1);
    OctiGameBases bases = (basesVal == 3) ? GAME_BASES_3 : GAME_BASES_1;
    bool superProngs = tObj.value("superProngs", false);
    bool edgeless = tObj.value("edgeless", false);
    std::string startPlayerStr = tObj.value("startPlayer", "A");
    OctiPlayer startPlayer = PLAYER_A;
    if (startPlayerStr == "B") startPlayer = PLAYER_B;
    else if (startPlayerStr == "C") startPlayer = PLAYER_C;
    else if (startPlayerStr == "D") startPlayer = PLAYER_D;

    OctiBoard board(mode, superProngs, edgeless, bases);
    std::string initialFen = board.toPositionString();
    OctiGame *game = board.getGame();
    if (!game) {
        sendError(res, 500, "INTERNAL_ERROR", "engine initialization failed");
        return;
    }
    game->start(startPlayer);

    Json pliesArr = Json::array();
    if (tObj.contains("plies") && tObj["plies"].is_array()) {
        const auto &rawPlies = tObj["plies"];
        for (size_t i = 0; i < rawPlies.size(); ++i) {
            const auto &p = rawPlies[i];
            if (!p.is_object() || !p.contains("move") || !p["move"].is_string()) {
                continue;
            }
            std::string mv = p["move"].get<std::string>();
            OctiPlayer turn = game->getTurn();
            std::string playerStr;
            switch (turn) {
            case PLAYER_A: playerStr = "A"; break;
            case PLAYER_B: playerStr = "B"; break;
            case PLAYER_C: playerStr = "C"; break;
            case PLAYER_D: playerStr = "D"; break;
            default: break;
            }
            try {
                game->move(turn, mv);
            } catch (const std::exception &ex) {
                sendError(res, 500, "INTERNAL_ERROR", std::string("replay error at ply ") + std::to_string(i + 1) + ": " + ex.what());
                return;
            }

            Json plyOut;
            plyOut["ply"] = i + 1;
            plyOut["plyIndex"] = i + 1;
            plyOut["player"] = playerStr;
            plyOut["move"] = mv;
            plyOut["elapsedMs"] = p.value("elapsedMs", 0LL);
            if (p.contains("clockAfterMs") && p["clockAfterMs"].get<int64_t>() >= 0) {
                plyOut["clockAfterMs"] = p["clockAfterMs"].get<int64_t>();
            }
            std::string fenCur = board.toPositionString();
            plyOut["fen"] = fenCur;
            plyOut["fenAfter"] = fenCur;
            if (p.contains("ai") && p["ai"].is_object()) {
                plyOut["ai"] = p["ai"];
            }
            pliesArr.push_back(plyOut);
        }
    }

    Json resp;
    resp["game_id"] = id;
    resp["mode"] = modeStr;
    resp["initialFen"] = initialFen;
    resp["initial_fen"] = initialFen;
    resp["plies"] = pliesArr;
    CachedPayload pliesStored = pliesCache.put(
        id, storeRev, 0, nowMs, "ply-", dumpJson(resp));
    sendCachedSnapshot(req, res, pliesStored, "no-cache");
}

void OctiHttpServer::Impl::handleArchiveStats(const Request &req, Response &res)
{
    Auth auth;
    if (!optionalAuth(req, res, auth) || !allowBrowseRead(req, res, auth)) {
        return;
    }
    const uint64_t statsRev = store.mutationRevision();
    const int64_t statsNowMs = steadyMs();
    CachedPayload statsCached;
    if (archiveStatsCache.get("stats", statsRev, statsNowMs, statsCached)) {
        sendCachedSnapshot(req, res, statsCached, "no-cache");
        return;
    }
    ArchiveStats stats;
    if (!store.getArchiveStats(stats)) {
        sendError(res, 500, "INTERNAL_ERROR", "failed to calculate archive stats");
        return;
    }
    Json j;
    j["total_games"] = stats.totalGames;
    j["by_source"] = stats.bySource;
    j["by_mode"] = stats.byMode;
    j["by_variant"] = stats.byVariant;
    Json byDiff = Json::object();
    for (const auto &kv : stats.byAiDifficulty) {
        byDiff[std::to_string(kv.first)] = kv.second;
    }
    j["by_ai_difficulty"] = byDiff;
    j["by_winner"] = stats.byWinner;

    Json aiArr = Json::array();
    for (const auto &ds : stats.aiDifficultyStats) {
        Json d;
        d["difficulty"] = ds.difficulty;
        d["total_games"] = ds.totalGames;
        d["ai_wins"] = ds.aiWins;
        d["win_rate"] = ds.winRate;
        aiArr.push_back(d);
    }
    j["ai_win_rates"] = aiArr;
    j["average_plies"] = stats.averagePlies;
    j["uploads_by_day"] = stats.uploadsByDay;
    j["license"] = "CC-BY-4.0";
    j["attribution"] = OCTI_DATA_ATTRIBUTION;
    CachedPayload statsStored = archiveStatsCache.put(
        "stats", statsRev, 15000, statsNowMs, "ast-", dumpJson(j));
    sendCachedSnapshot(req, res, statsStored, "no-cache");
}

void OctiHttpServer::Impl::handleArchiveExport(const Request &req, Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth) ||
        !allowExportAttempt(req, res, auth.playerId)) {
        return;
    }

    auto exportSlot = std::make_shared<UrlProbeSlotGuard>(
        activeExportsCount, cfg.maxConcurrentExports);
    if (!exportSlot->allowed()) {
        res.set_header("Retry-After", "5");
        sendError(res, 503, "SERVICE_BUSY",
                  "too many concurrent archive exports; retry shortly");
        return;
    }

    static constexpr int64_t kMaxExportPageLimit = 2000;
    static constexpr int64_t kMaxExportLegacyOffset = 5000;
    static constexpr int kExportBatchRows = 64;

    ArchiveFilter f;
    f.includeTranscript = true;
    f.maxLimit = static_cast<int>(kMaxExportPageLimit);

    if (req.has_param("mode")) {
        std::string mode = upper(req.get_param_value("mode"));
        if (!mode.empty()) {
            if (mode == "2P" || mode == "2P_ALL") {
                f.mode = "2P";
            } else if (mode == "4P" || mode == "4P_ALL") {
                f.mode = "4P";
            } else if (mode == "OFK") {
                f.mode = "OFK";
            } else if (isValidOctiVariantCode(mode)) {
                f.variant = mode;
            } else {
                sendError(res, 400, "INVALID_ARGUMENT",
                          "mode must be 2P, 4P, OFK, or a canonical variant code");
                return;
            }
        }
    }
    if (req.has_param("variant")) {
        std::string var = upper(req.get_param_value("variant"));
        if (!var.empty()) {
            if (var == "2P" || var == "2P_ALL") {
                f.mode = "2P";
            } else if (var == "4P_ALL") {
                f.mode = "4P";
            } else if (isValidOctiVariantCode(var)) {
                f.variant = var;
            } else {
                sendError(res, 400, "INVALID_ARGUMENT",
                          "variant must be one of the 13 canonical OCTI variant codes");
                return;
            }
        }
    }
    if (req.has_param("player")) {
        f.playerUuid = req.get_param_value("player");
        if (!isSafeId(f.playerUuid)) {
            sendError(res, 400, "INVALID_ARGUMENT", "invalid player id");
            return;
        }
    }
    if (req.has_param("robot")) {
        f.robotUuid = req.get_param_value("robot");
        if (!isSafeId(f.robotUuid)) {
            sendError(res, 400, "INVALID_ARGUMENT", "invalid robot id");
            return;
        }
    }
    if (req.has_param("source")) {
        std::string src = lower(req.get_param_value("source"));
        if (src != "online" && src != "client_upload") {
            sendError(res, 400, "INVALID_ARGUMENT",
                      "source must be online or client_upload");
            return;
        }
        f.source = src;
    }
    if (req.has_param("status")) {
        std::string st = upper(req.get_param_value("status"));
        if (st != "GAME_OVER" && st != "RESIGNED") {
            sendError(res, 400, "INVALID_ARGUMENT",
                      "status must be GAME_OVER or RESIGNED");
            return;
        }
        f.status = st;
    }
    if (req.has_param("winner")) {
        std::string w = upper(req.get_param_value("winner"));
        if (w != "A" && w != "B" && w != "C" && w != "D" && w != "NONE") {
            sendError(res, 400, "INVALID_ARGUMENT",
                      "winner must be A, B, C, D, or NONE");
            return;
        }
        f.winner = w;
    }
    if (req.has_param("aiDifficulty") || req.has_param("ai_difficulty")) {
        std::string dStr = req.has_param("aiDifficulty")
                               ? req.get_param_value("aiDifficulty")
                               : req.get_param_value("ai_difficulty");
        int64_t diff = 0;
        if (!parseUnsignedClamped(dStr, 5, diff) || diff < 1 || diff > 5) {
            sendError(res, 400, "INVALID_ARGUMENT", "aiDifficulty must be 1..5");
            return;
        }
        f.aiDifficulty = static_cast<int>(diff);
    }
    if (req.has_param("appVersion") || req.has_param("app_version")) {
        std::string av = req.has_param("appVersion")
                             ? req.get_param_value("appVersion")
                             : req.get_param_value("app_version");
        if (av.size() > 128 || hasControlChars(av)) {
            sendError(res, 400, "INVALID_ARGUMENT", "invalid appVersion");
            return;
        }
        f.appVersion = av;
    }
    if (req.has_param("platform")) {
        std::string plat = req.get_param_value("platform");
        if (plat.size() > 128 || hasControlChars(plat)) {
            sendError(res, 400, "INVALID_ARGUMENT", "invalid platform");
            return;
        }
        f.platform = plat;
    }
    if (req.has_param("from")) {
        int64_t fromSec = 0;
        if (!parseUnsignedClamped(req.get_param_value("from"), INT64_MAX,
                                  fromSec)) {
            sendError(res, 400, "INVALID_ARGUMENT",
                      "from must be a non-negative integer");
            return;
        }
        f.fromSec = fromSec;
    }
    if (req.has_param("to")) {
        int64_t toSec = 0;
        if (!parseUnsignedClamped(req.get_param_value("to"), INT64_MAX,
                                  toSec)) {
            sendError(res, 400, "INVALID_ARGUMENT",
                      "to must be a non-negative integer");
            return;
        }
        f.toSec = toSec;
    }
    if (req.has_param("sort")) {
        std::string sVal = req.get_param_value("sort");
        std::string field = sVal;
        std::string dir = "";
        size_t colon = sVal.find(':');
        if (colon != std::string::npos) {
            field = sVal.substr(0, colon);
            dir = lower(sVal.substr(colon + 1));
        }
        if (field != "ended_at" && field != "uploaded_at" &&
            field != "ply_count") {
            sendError(res, 400, "INVALID_ARGUMENT", "invalid sort field");
            return;
        }
        f.sortField = field;
        if (dir == "asc") {
            f.sortAsc = true;
        } else if (dir == "desc") {
            f.sortAsc = false;
        }
    }
    if (req.has_param("order")) {
        std::string ord = lower(req.get_param_value("order"));
        if (ord == "asc") {
            f.sortAsc = true;
        } else if (ord == "desc") {
            f.sortAsc = false;
        } else {
            sendError(res, 400, "INVALID_ARGUMENT", "order must be asc or desc");
            return;
        }
    }

    if (req.has_param("cursor") && !req.get_param_value("cursor").empty()) {
        std::string cur = req.get_param_value("cursor");
        size_t colon = cur.find(':');
        int64_t curEnd = 0;
        if (colon == std::string::npos || colon == 0 ||
            colon + 1 >= cur.size() ||
            !parseUnsignedStrict(cur.substr(0, colon), INT64_MAX / 2, curEnd) ||
            !isSafeId(cur.substr(colon + 1))) {
            sendError(res, 400, "INVALID_ARGUMENT",
                      "cursor must have format <ended_at>:<game_id>");
            return;
        }
        f.hasCursor = true;
        f.afterEndedAt = curEnd;
        f.afterGameId = cur.substr(colon + 1);
    }
    if (req.has_param("after_ended_at") &&
        !req.get_param_value("after_ended_at").empty()) {
        int64_t afterEnd = 0;
        if (!parseUnsignedStrict(req.get_param_value("after_ended_at"),
                                 INT64_MAX / 2, afterEnd)) {
            sendError(res, 400, "INVALID_ARGUMENT",
                      "after_ended_at must be a non-negative integer");
            return;
        }
        f.hasCursor = true;
        f.afterEndedAt = afterEnd;
    }
    if (req.has_param("after_game_id") &&
        !req.get_param_value("after_game_id").empty()) {
        std::string afterId = req.get_param_value("after_game_id");
        if (!isSafeId(afterId)) {
            sendError(res, 400, "INVALID_ARGUMENT", "invalid after_game_id");
            return;
        }
        if (!f.hasCursor) {
            sendError(res, 400, "INVALID_ARGUMENT",
                      "after_game_id requires after_ended_at");
            return;
        }
        f.afterGameId = afterId;
    }
    if (f.hasCursor && !f.sortField.empty() && f.sortField != "ended_at") {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "keyset cursor pagination requires sort=ended_at");
        return;
    }

    int64_t limit = 1000;
    int64_t offset = 0;
    if (req.has_param("limit")) {
        if (!parseUnsignedStrict(req.get_param_value("limit"), INT32_MAX,
                                 limit) ||
            limit < 1) {
            sendError(res, 400, "INVALID_ARGUMENT",
                      "limit must be a positive integer");
            return;
        }
    }
    if (req.has_param("offset")) {
        if (!parseUnsignedStrict(req.get_param_value("offset"), INT32_MAX,
                                 offset)) {
            sendError(res, 400, "INVALID_ARGUMENT",
                      "offset must be a non-negative integer");
            return;
        }
        if (offset > kMaxExportLegacyOffset) {
            sendError(res, 400, "INVALID_ARGUMENT",
                      "offset exceeds maximum 5000; use X-Octi-Next-Cursor / "
                      "after_ended_at & after_game_id keyset pagination");
            return;
        }
    }
    if (f.hasCursor && offset != 0) {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "offset cannot be combined with cursor pagination");
        return;
    }
    limit = std::max<int64_t>(1, std::min<int64_t>(limit, kMaxExportPageLimit));

    /* Fetch first 64-row batch before headers are committed. */
    const int firstLimit = static_cast<int>(
        std::min<int64_t>(limit, kExportBatchRows));
    ArchiveFilter firstFilter = f;
    firstFilter.limit = firstLimit;
    firstFilter.offset = static_cast<int>(offset);

    std::vector<GameRecord> firstBatch;
    if (!store.listArchivedGames(firstFilter, firstBatch)) {
        sendError(res, 500, "INTERNAL_ERROR", "archive export query failed");
        return;
    }

    std::string firstChunk;
    firstChunk.reserve(firstBatch.size() * 1024);
    for (size_t i = 0; i < firstBatch.size(); ++i) {
        appendSingleLineNdjsonRow(firstBatch[i].octiTranscript, firstChunk);
    }

    /* Compute X-Octi-Next-Cursor when the page is full (rows == limit). */
    std::string nextCursor;
    const bool canUseKeysetAdvance =
        f.sortField.empty() || f.sortField == "ended_at";
    if (canUseKeysetAdvance && !firstBatch.empty()) {
        if (static_cast<int64_t>(firstBatch.size()) == limit) {
            const GameRecord &tail = firstBatch.back();
            nextCursor = std::to_string(tail.endedAt) + ":" + tail.gameId;
        } else if (static_cast<int>(firstBatch.size()) == firstLimit &&
                   limit > firstLimit) {
            ArchiveFilter tailProbe = f;
            tailProbe.includeTranscript = false;
            tailProbe.limit = 1;
            tailProbe.offset = static_cast<int>(offset + limit - 1);
            std::vector<GameRecord> tailRow;
            if (store.listArchivedGames(tailProbe, tailRow) &&
                !tailRow.empty()) {
                nextCursor = std::to_string(tailRow[0].endedAt) + ":" +
                             tailRow[0].gameId;
            }
        }
    }

    const bool useGzip =
        clientAcceptsGzip(req) && firstChunk.size() >= kMinDynamicGzipBytes;

    struct ExportStreamState {
        std::shared_ptr<UrlProbeSlotGuard> slot;
        ArchiveFilter baseFilter;
        bool useKeyset = true;
        int64_t remaining = 0;
        int64_t nextOffset = 0;
        int64_t lastEndedAt = 0;
        std::string lastGameId;
        std::string pendingFirstChunk;
        bool firstChunkPending = true;
        bool useGzip = false;
        bool gzipInited = false;
        z_stream zs;
        int testBatchDelayMs = 0;

        ExportStreamState()
        {
            std::memset(&zs, 0, sizeof(zs));
        }
        ~ExportStreamState()
        {
            if (gzipInited) {
                deflateEnd(&zs);
            }
        }
        bool initGzip()
        {
            if (deflateInit2(&zs, Z_BEST_SPEED, Z_DEFLATED, 15 + 16, 8,
                             Z_DEFAULT_STRATEGY) != Z_OK) {
                return false;
            }
            gzipInited = true;
            return true;
        }
        bool writeCompressed(httplib::DataSink &sink, const char *inData,
                             size_t inLen, int flushMode)
        {
            zs.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(inData));
            zs.avail_in = static_cast<uInt>(inLen);
            char outBuf[16384];
            do {
                zs.next_out = reinterpret_cast<Bytef *>(outBuf);
                zs.avail_out = sizeof(outBuf);
                int ret = deflate(&zs, flushMode);
                if (ret == Z_STREAM_ERROR) {
                    return false;
                }
                size_t have = sizeof(outBuf) - zs.avail_out;
                if (have > 0 && !sink.write(outBuf, have)) {
                    return false;
                }
                if (flushMode == Z_FINISH && ret == Z_STREAM_END) {
                    break;
                }
            } while (zs.avail_out == 0 ||
                     (flushMode == Z_FINISH && zs.avail_in == 0));
            return true;
        }
    };

    auto st = std::make_shared<ExportStreamState>();
    st->slot = exportSlot;
    st->baseFilter = f;
    st->useKeyset = canUseKeysetAdvance;
    st->remaining = limit - static_cast<int64_t>(firstBatch.size());
    if (static_cast<int>(firstBatch.size()) < firstLimit || firstBatch.empty()) {
        st->remaining = 0;
    } else {
        st->lastEndedAt = firstBatch.back().endedAt;
        st->lastGameId = firstBatch.back().gameId;
        st->nextOffset = offset + static_cast<int64_t>(firstBatch.size());
    }
    st->pendingFirstChunk = std::move(firstChunk);
    st->useGzip = useGzip;
    st->testBatchDelayMs = cfg.exportBatchDelayMsForTesting;
    if (st->useGzip && !st->initGzip()) {
        sendError(res, 500, "INTERNAL_ERROR", "gzip stream init failed");
        return;
    }

    res.status = 200;
    res.set_header("Cache-Control", "no-store");
    res.set_header("Vary", "Accept-Encoding");
    res.set_header("Content-Disposition",
                   "attachment; filename=\"octi-archive-export.ndjson\"");
    if (!nextCursor.empty()) {
        res.set_header("X-Octi-Next-Cursor", nextCursor);
    }
    if (st->useGzip) {
        res.set_header("Content-Encoding", "gzip");
    }

    res.set_chunked_content_provider(
        "application/x-ndjson; charset=utf-8",
        [this, st](size_t, httplib::DataSink &sink) -> bool {
            if (stopping.load()) {
                sink.done();
                return true;
            }
            if (st->testBatchDelayMs > 0) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(st->testBatchDelayMs));
            }
            if (st->firstChunkPending) {
                st->firstChunkPending = false;
                if (!st->pendingFirstChunk.empty()) {
                    const bool isFinal = (st->remaining <= 0);
                    if (st->useGzip) {
                        if (!st->writeCompressed(
                                sink, st->pendingFirstChunk.data(),
                                st->pendingFirstChunk.size(),
                                isFinal ? Z_FINISH : Z_SYNC_FLUSH)) {
                            return false;
                        }
                    } else if (!sink.write(st->pendingFirstChunk.data(),
                                           st->pendingFirstChunk.size())) {
                        return false;
                    }
                    std::string().swap(st->pendingFirstChunk);
                    if (isFinal) {
                        sink.done();
                        return true;
                    }
                    return true;
                }
                if (st->useGzip) {
                    if (!st->writeCompressed(sink, "", 0, Z_FINISH)) {
                        return false;
                    }
                }
                sink.done();
                return true;
            }

            if (st->remaining <= 0) {
                if (st->useGzip) {
                    if (!st->writeCompressed(sink, "", 0, Z_FINISH)) {
                        return false;
                    }
                }
                sink.done();
                return true;
            }

            const int batchLim = static_cast<int>(
                std::min<int64_t>(st->remaining, kExportBatchRows));
            ArchiveFilter bf = st->baseFilter;
            bf.limit = batchLim;
            if (st->useKeyset) {
                bf.offset = 0;
                bf.hasCursor = true;
                bf.afterEndedAt = st->lastEndedAt;
                bf.afterGameId = st->lastGameId;
            } else {
                bf.offset = static_cast<int>(st->nextOffset);
            }

            std::vector<GameRecord> batch;
            if (!store.listArchivedGames(bf, batch) || batch.empty()) {
                st->remaining = 0;
                if (st->useGzip) {
                    if (!st->writeCompressed(sink, "", 0, Z_FINISH)) {
                        return false;
                    }
                }
                sink.done();
                return true;
            }

            std::string chunk;
            chunk.reserve(batch.size() * 1024);
            for (size_t i = 0; i < batch.size(); ++i) {
                appendSingleLineNdjsonRow(batch[i].octiTranscript, chunk);
            }
            st->remaining -= static_cast<int64_t>(batch.size());
            st->nextOffset += static_cast<int64_t>(batch.size());
            st->lastEndedAt = batch.back().endedAt;
            st->lastGameId = batch.back().gameId;
            if (static_cast<int>(batch.size()) < batchLim) {
                st->remaining = 0;
            }

            const bool isFinal = (st->remaining <= 0);
            if (st->useGzip) {
                if (!st->writeCompressed(sink, chunk.data(), chunk.size(),
                                         isFinal ? Z_FINISH : Z_SYNC_FLUSH)) {
                    return false;
                }
            } else if (!chunk.empty() &&
                       !sink.write(chunk.data(), chunk.size())) {
                return false;
            }
            if (isFinal) {
                sink.done();
            }
            return true;
        });
}

void OctiHttpServer::Impl::handleFavicon(const Request &, Response &res)
{
    static const char kFaviconSvg[] =
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 128 128\">"
        "<rect x=\"2\" y=\"2\" width=\"124\" height=\"124\" rx=\"24\" fill=\"#0B0F17\" stroke=\"#C8A253\" stroke-width=\"2\"/>"
        "<polygon points=\"45,16 83,16 112,45 112,83 83,112 45,112 16,83 16,45\" fill=\"#E4C070\" stroke=\"#FEF08A\" stroke-width=\"1.5\"/>"
        "<polygon points=\"48,23 80,23 105,48 105,80 80,105 48,105 23,80 23,48\" fill=\"#0F172A\" stroke=\"#D97706\" stroke-width=\"1.5\"/>"
        "<g stroke=\"#0F172A\" stroke-width=\"1.2\" fill=\"#CBD5E1\" stroke-linejoin=\"round\">"
        "<polygon points=\"82,60 98,58 106,64 98,70 82,68\"/>"
        "<polygon points=\"46,60 30,58 22,64 30,70 46,68\"/>"
        "<polygon points=\"75,47 89,33 97,31 95,39 81,53\"/>"
        "<polygon points=\"53,47 39,33 31,31 33,39 47,53\"/>"
        "<polygon points=\"81,75 95,89 97,97 89,95 75,81\"/>"
        "<polygon points=\"47,75 33,89 31,97 39,95 53,81\"/>"
        "</g>"
        "<polygon points=\"59,44 55,28 59,25 56,19 64,9 72,19 69,25 73,28 69,44\" fill=\"#FDE047\" stroke=\"#FFFBEB\" stroke-width=\"1.3\" stroke-linejoin=\"round\"/>"
        "<polygon points=\"52,36 76,36 92,52 92,76 76,92 52,92 36,76 36,52\" fill=\"#E4C070\" stroke=\"#FDE047\" stroke-width=\"1.5\"/>"
        "<polygon points=\"54,40 74,40 88,54 88,74 74,88 54,88 40,74 40,54\" fill=\"#10B981\" stroke=\"#6EE7B7\" stroke-width=\"1.2\"/>"
        "<polygon points=\"64,49 67,61 79,64 67,67 64,79 61,67 49,64 61,61\" fill=\"#FDE047\" stroke=\"#78350F\" stroke-width=\"1\"/>"
        "<circle cx=\"64\" cy=\"64\" r=\"3\" fill=\"#FFFBEB\"/>"
        "</svg>";
    res.status = 200;
    res.set_header("Cache-Control", "public, max-age=86400");
    res.set_content(kFaviconSvg, "image/svg+xml; charset=utf-8");
}

void OctiHttpServer::Impl::handleIndex(const Request &req, Response &res)
{
    res.set_header("Cache-Control", "no-cache");
    res.set_header("ETag", indexEtag);
    res.set_header("Vary", "Accept-Encoding");
    res.set_header("Content-Security-Policy",
                   "default-src 'none'; script-src 'unsafe-inline'; "
                   "worker-src 'self' blob:; "
                   "style-src 'unsafe-inline'; connect-src 'self'; "
                   "img-src 'self' data:; media-src 'self' data: blob:; "
                   "object-src 'none'; base-uri 'none'; "
                   "form-action 'none'; frame-ancestors 'none'");
    if (isHttpsRequest(req)) {
        res.set_header("Strict-Transport-Security",
                       "max-age=31536000; includeSubDomains");
    }
    if (ifNoneMatchMatches(req, indexEtag)) {
        res.status = 304;
        return;
    }
    res.status = 200;
    if (!indexGzipBytes.empty() && clientAcceptsGzip(req)) {
        res.set_header("Content-Encoding", "gzip");
        res.set_content(indexGzipBytes, "text/html; charset=utf-8");
    } else {
        res.set_content(webUiHtml(), "text/html; charset=utf-8");
    }
}

void OctiHttpServer::Impl::handleTelemetryCrash(const Request &req, Response &res)
{
    const size_t maxBytes = routeMaxBodyBytes(req.method, req.path);
    if (req.body.empty() || req.body.size() > maxBytes) {
        sendError(res, req.body.size() > maxBytes ? 413 : 400,
                  req.body.size() > maxBytes ? "PAYLOAD_TOO_LARGE" : "MALFORMED_UPLOAD",
                  "invalid payload size");
        return;
    }
    if (!allowUploadAttempt(req, res)) {
        return;
    }
    Json root = Json::parse(req.body, nullptr, false);
    if (root.is_discarded() || !root.is_object()) {
        sendError(res, 400, "INVALID_ARGUMENT", "invalid JSON payload");
        return;
    }

    if (!root.contains("platform") || !root["platform"].is_string() ||
        root["platform"].get<std::string>().empty() ||
        root["platform"].get<std::string>().size() > 64) {
        sendError(res, 400, "INVALID_ARGUMENT", "missing or invalid platform");
        return;
    }
    std::string platform = root["platform"].get<std::string>();

    if (!root.contains("exception_type") || !root["exception_type"].is_string() ||
        root["exception_type"].get<std::string>().empty() ||
        root["exception_type"].get<std::string>().size() > 256) {
        sendError(res, 400, "INVALID_ARGUMENT", "missing or invalid exception_type");
        return;
    }
    std::string exceptionType = root["exception_type"].get<std::string>();

    int64_t createdAt = 0;
    if (root.contains("created_at") && root["created_at"].is_number_integer()) {
        createdAt = root["created_at"].get<int64_t>();
    }
    if (createdAt <= 0) {
        createdAt = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }

    std::string appVersion;
    if (root.contains("app_version") && root["app_version"].is_string()) {
        appVersion = root["app_version"].get<std::string>();
        if (appVersion.size() > 64) {
            sendError(res, 400, "INVALID_ARGUMENT", "app_version exceeds 64 chars");
            return;
        }
    }

    std::string osVersion;
    if (root.contains("os_version") && root["os_version"].is_string()) {
        osVersion = root["os_version"].get<std::string>();
        if (osVersion.size() > 128) {
            sendError(res, 400, "INVALID_ARGUMENT", "os_version exceeds 128 chars");
            return;
        }
    }

    std::string exceptionMessage;
    if (root.contains("exception_message") && root["exception_message"].is_string()) {
        exceptionMessage = root["exception_message"].get<std::string>();
        if (exceptionMessage.size() > 2048) {
            sendError(res, 400, "INVALID_ARGUMENT", "exception_message exceeds 2048 chars");
            return;
        }
    }

    std::string threadName;
    if (root.contains("thread_name") && root["thread_name"].is_string()) {
        threadName = root["thread_name"].get<std::string>();
        if (threadName.size() > 128) {
            sendError(res, 400, "INVALID_ARGUMENT", "thread_name exceeds 128 chars");
            return;
        }
    }

    std::string stackTrace;
    if (root.contains("stack_trace") && root["stack_trace"].is_string()) {
        stackTrace = root["stack_trace"].get<std::string>();
        if (stackTrace.size() > 32768) {
            sendError(res, 400, "INVALID_ARGUMENT", "stack_trace exceeds 32768 chars");
            return;
        }
    }

    std::string contextJson = "{}";
    if (root.contains("context")) {
        if (root["context"].is_object()) {
            contextJson = root["context"].dump();
        } else if (root["context"].is_string()) {
            contextJson = root["context"].get<std::string>();
        } else if (!root["context"].is_null()) {
            contextJson = root["context"].dump();
        }
        if (contextJson.size() > 16384) {
            sendError(res, 400, "INVALID_ARGUMENT", "context exceeds 16384 chars");
            return;
        }
    }

    std::string installId;
    if (root.contains("install_id") && root["install_id"].is_string()) {
        installId = root["install_id"].get<std::string>();
        if (installId.size() > 128) {
            sendError(res, 400, "INVALID_ARGUMENT", "install_id exceeds 128 chars");
            return;
        }
    }
    if (req.has_header("X-Octi-Install-Token")) {
        std::string installTok = req.get_header_value("X-Octi-Install-Token");
        if (!AccountStore::isValidInstallToken(installTok)) {
            sendError(res, 400, "INVALID_ARGUMENT", "invalid X-Octi-Install-Token");
            return;
        }
        if (!installId.empty() && isSafeId(installId)) {
            if (!store.registerOrVerifyInstallToken(installId, installTok)) {
                sendError(res, 403, "FORBIDDEN", "install token mismatch");
                return;
            }
        }
    } else if (!installId.empty() && isSafeId(installId) &&
               store.hasInstallToken(installId)) {
        sendError(res, 403, "FORBIDDEN",
                  "missing X-Octi-Install-Token for registered install_id");
        return;
    }

    std::string crashId;
    if (root.contains("crash_id") && root["crash_id"].is_string()) {
        crashId = root["crash_id"].get<std::string>();
    }
    if (crashId.empty()) {
        std::string seed = platform + "|" + appVersion + "|" + exceptionType + "|" +
                           exceptionMessage + "|" + stackTrace + "|" + std::to_string(createdAt);
        crashId = AccountStore::sha256Hex(seed);
    } else if (crashId.size() > 128) {
        sendError(res, 400, "INVALID_ARGUMENT", "crash_id exceeds 128 chars");
        return;
    }

    CrashReportRecord rec;
    rec.crashId = crashId;
    rec.createdAt = createdAt;
    rec.uploadedAt = 0;
    rec.platform = platform;
    rec.appVersion = appVersion;
    rec.osVersion = osVersion;
    rec.exceptionType = exceptionType;
    rec.exceptionMessage = exceptionMessage;
    rec.threadName = threadName;
    rec.stackTrace = stackTrace;
    rec.contextJson = contextJson.empty() ? "{}" : contextJson;
    rec.installId = installId;

    std::string err;
    ArchiveStatus st = store.recordCrashReport(rec, err);
    if (st == ArchiveStatus::STORED) {
        Json j;
        j["crash_id"] = rec.crashId;
        j["deduplicated"] = false;
        sendJson(res, 201, j);
    } else if (st == ArchiveStatus::DUPLICATE) {
        Json j;
        j["crash_id"] = rec.crashId;
        j["deduplicated"] = true;
        sendJson(res, 200, j);
    } else {
        sendError(res, 400, "INVALID_ARGUMENT", err.empty() ? "invalid crash report" : err);
    }
}

void OctiHttpServer::Impl::handleTelemetryCrashes(const Request &req, Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth)) {
        return;
    }
    if (auth.isRobot || !store.isPlayerAdmin(auth.playerId)) {
        sendError(res, 403, "FORBIDDEN", "admin privileges required");
        return;
    }
    CrashReportFilter f;
    if (req.has_param("platform")) {
        f.platform = req.get_param_value("platform");
    }
    if (req.has_param("app_version")) {
        f.appVersion = req.get_param_value("app_version");
    } else if (req.has_param("appVersion")) {
        f.appVersion = req.get_param_value("appVersion");
    }
    if (req.has_param("exception_type")) {
        f.exceptionType = req.get_param_value("exception_type");
    } else if (req.has_param("exceptionType")) {
        f.exceptionType = req.get_param_value("exceptionType");
    }

    int64_t limit = 20;
    int64_t offset = 0;
    if ((req.has_param("limit") &&
         !parseUnsignedClamped(req.get_param_value("limit"), INT32_MAX, limit)) ||
        (req.has_param("offset") &&
         !parseUnsignedClamped(req.get_param_value("offset"), INT32_MAX, offset))) {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "limit and offset must be non-negative integers");
        return;
    }
    limit = std::max<int64_t>(1, std::min<int64_t>(limit, 100));
    f.limit = static_cast<int>(limit);
    f.offset = static_cast<int>(offset);

    std::vector<CrashReportRecord> records;
    int64_t total = 0;
    if (!store.listCrashReports(f, records, &total)) {
        sendError(res, 500, "INTERNAL_ERROR", "failed to list crash reports");
        return;
    }

    Json arr = Json::array();
    for (const auto &r : records) {
        Json item;
        item["crash_id"] = r.crashId;
        item["created_at"] = r.createdAt;
        item["uploaded_at"] = r.uploadedAt;
        item["platform"] = r.platform;
        item["app_version"] = r.appVersion;
        item["os_version"] = r.osVersion;
        item["exception_type"] = r.exceptionType;
        item["exception_message"] = r.exceptionMessage;
        item["thread_name"] = r.threadName;
        arr.push_back(item);
    }
    Json j;
    j["total"] = total;
    j["crashes"] = arr;
    sendJson(res, 200, j);
}

bool OctiHttpServer::Impl::pathCrashId(const Request &req, Response &res,
                                       std::string &out)
{
    std::string val;
    auto it = req.path_params.find("id");
    if (it != req.path_params.end()) {
        val = it->second;
    } else if (req.matches.size() > 1) {
        val = req.matches[1].str();
    }
    if (val.empty() || val.size() > 128) {
        sendError(res, 404, "NOT_FOUND", "no such resource");
        return false;
    }
    for (char c : val) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (!std::isalnum(uc) && uc != '-' && uc != '_') {
            sendError(res, 404, "NOT_FOUND", "no such resource");
            return false;
        }
    }
    out = val;
    return true;
}

void OctiHttpServer::Impl::handleTelemetryCrashGet(const Request &req, Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth)) {
        return;
    }
    if (auth.isRobot || !store.isPlayerAdmin(auth.playerId)) {
        sendError(res, 403, "FORBIDDEN", "admin privileges required");
        return;
    }
    std::string id;
    if (!pathCrashId(req, res, id)) {
        return;
    }
    CrashReportRecord rec;
    if (!store.getCrashReport(id, rec)) {
        sendError(res, 404, "NOT_FOUND", "no such crash report");
        return;
    }
    Json j;
    j["crash_id"] = rec.crashId;
    j["created_at"] = rec.createdAt;
    j["uploaded_at"] = rec.uploadedAt;
    j["platform"] = rec.platform;
    j["app_version"] = rec.appVersion;
    j["os_version"] = rec.osVersion;
    j["exception_type"] = rec.exceptionType;
    j["exception_message"] = rec.exceptionMessage;
    j["thread_name"] = rec.threadName;
    j["stack_trace"] = rec.stackTrace;
    Json ctx = Json::parse(rec.contextJson, nullptr, false);
    if (!ctx.is_discarded()) {
        j["context"] = ctx;
    } else {
        j["context"] = Json::object();
    }
    j["context_json"] = rec.contextJson;
    sendJson(res, 200, j);
}

/* ------------------------------------------------------------------ */
/* Routing                                                             */
/* ------------------------------------------------------------------ */

void OctiHttpServer::Impl::registerRoutes()
{
    typedef void (Impl::*Handler)(const Request &, Response &);
    struct Route {
        const char *method;
        const char *pattern;
        Handler handler;
    };
    static const Route routes[] = {
        {"GET", "/", &Impl::handleIndex},
        {"GET", "/robots.txt", &Impl::handleRobotsTxt},
        {"GET", "/favicon.ico", &Impl::handleFavicon},
        {"GET", "/favicon.svg", &Impl::handleFavicon},
        {"GET", "/health", &Impl::handleHealth},
        {"GET", "/privacy", &Impl::handlePrivacy},
        {"POST", "/privacy", &Impl::handleDataDeletion},
        {"GET", "/api/v1/health", &Impl::handleHealth},
        {"GET", "/api/v1/avatars/:file", &Impl::handleAvatarGet},
        {"GET", "/avatar/:file", &Impl::handleAvatarGet},
        {"GET", "/sounds/:file", &Impl::handleSoundAssetGet},
        {"GET", "/assets/rulebook/:file", &Impl::handleRulebookAssetGet},
        {"GET", "/api/v1/auth/data-deletion", &Impl::handleDataDeletion},
        {"POST", "/api/v1/auth/data-deletion", &Impl::handleDataDeletion},
        {"GET", "/api/v1/auth/authorize", &Impl::handleAuthAuthorize},
        {"GET", "/api/v1/auth/callback", &Impl::handleAuthCallback},
        {"GET", "/api/v1/auth/providers", &Impl::handleProviders},
        {"POST", "/api/v1/auth/sso", &Impl::handleSso},
        {"POST", "/api/v1/auth/link", &Impl::handleLink},
        {"DELETE", "/api/v1/auth/link/:provider", &Impl::handleUnlink},
        {"GET", "/auth/identities", &Impl::handleAuthListIdentities},
        {"GET", "/api/v1/auth/identities", &Impl::handleAuthListIdentities},
        {"DELETE", "/auth/identities/:provider", &Impl::handleAuthUnlinkProvider},
        {"DELETE", "/api/v1/auth/identities/:provider", &Impl::handleAuthUnlinkProvider},
        {"POST", "/auth/logout", &Impl::handleLogout},
        {"POST", "/api/v1/auth/logout", &Impl::handleLogout},
        {"GET", "/api/v1/profile/me", &Impl::handleProfileGet},
        {"GET", "/api/v1/profile", &Impl::handleProfileGet},
        {"PATCH", "/api/v1/profile/me", &Impl::handleProfilePatch},
        {"PATCH", "/api/v1/profile", &Impl::handleProfilePatch},
        {"GET", "/api/v1/profile/:id", &Impl::handleUserGet},
        {"GET", "/api/v1/users", &Impl::handleUsersList},
        {"GET", "/api/v1/users/:id", &Impl::handleUserGet},
        {"GET", "/api/v1/robots", &Impl::handleRobotsList},
        {"GET", "/api/v1/robots/export-conf", &Impl::handleRobotsExportConf},
        {"GET", "/api/v1/robots/directory", &Impl::handleRobotsDirectory},
        {"GET", "/api/v1/robots/profile/:id", &Impl::handleRobotProfileGet},
        {"POST", "/api/v1/robots/connect", &Impl::handleRobotsConnect},
        {"POST", "/api/v1/robots/disconnect", &Impl::handleRobotsDisconnect},
        {"POST", "/api/v1/robots", &Impl::handleRobotCreate},
        {"PATCH", "/api/v1/robots/:id", &Impl::handleRobotPatch},
        {"DELETE", "/api/v1/robots/:id", &Impl::handleRobotDelete},
        {"POST", "/api/v1/robots/:id/revoke", &Impl::handleRobotDelete},
        {"GET", "/api/v1/players/:uuid", &Impl::handlePlayer},
        {"GET", "/api/v1/leaderboard", &Impl::handleLeaderboard},
        {"GET", "/api/v1/lobby/state", &Impl::handleLobbyState},
        {"POST", "/api/v1/lobby/chat", &Impl::handleLobbyChat},
        {"GET", "/api/v1/lobby/events", &Impl::handleLobbyEvents},
        {"GET", "/api/v1/tables", &Impl::handleTables},
        {"GET", "/api/v1/tables/:id", &Impl::handleGameGet},
        {"POST", "/api/v1/tables", &Impl::handleTableCreate},
        {"POST", "/api/v1/tables/:id/join", &Impl::handleTableJoin},
        {"POST", "/api/v1/tables/:id/leave", &Impl::handleTableLeave},
        {"DELETE", "/api/v1/tables/:id", &Impl::handleTablePurge},
        {"POST", "/api/v1/tables/:id/kick", &Impl::handleTableKick},
        {"POST", "/api/v1/tables/:id/observe", &Impl::handleTableObserve},
        {"POST", "/api/v1/tables/:id/unobserve", &Impl::handleTableUnobserve},
        {"POST", "/api/v1/matchmaking/enqueue", &Impl::handleEnqueue},
        {"POST", "/api/v1/matchmaking/dequeue", &Impl::handleDequeue},
        {"GET", "/api/v1/matchmaking/status", &Impl::handleMatchStatus},
        {"GET", "/api/v1/games/:id", &Impl::handleGameGet},
        {"POST", "/api/v1/games/:id/move", &Impl::handleMove},
        {"POST", "/api/v1/games/:id/resign", &Impl::handleResign},
        {"POST", "/api/v1/games/:id/action", &Impl::handleAction},
        {"POST", "/api/v1/games/:id/chat", &Impl::handleChat},
        {"POST", "/api/v1/games/:id/ban", &Impl::handleBan},
        {"POST", "/api/v1/games/:id/mute", &Impl::handleMute},
        {"POST", "/api/v1/games/:id/settings", &Impl::handleSettings},
        {"GET", "/api/v1/games/:id/events", &Impl::handleGameEvents},
        {"POST", "/api/v1/archive/upload", &Impl::handleArchiveUpload},
        {"DELETE", "/api/v1/archive/install/:installId", &Impl::handleArchiveDeleteInstall},
        {"DELETE", "/api/v1/archive/games/:id", &Impl::handleArchivePurgeGame},
        {"POST", "/api/v1/admin/recompute-ratings", &Impl::handleAdminRecomputeRatings},
        {"GET", "/api/v1/admin/bans", &Impl::handleAdminBansList},
        {"POST", "/api/v1/admin/bans", &Impl::handleAdminBanAdd},
        {"POST", "/api/v1/admin/bans/move", &Impl::handleAdminBansMove},
        {"POST", "/api/v1/admin/bans/remove", &Impl::handleAdminBansRemove},
        {"PATCH", "/api/v1/admin/bans/:id", &Impl::handleAdminBanPatch},
        {"DELETE", "/api/v1/admin/bans/:id", &Impl::handleAdminBanDelete},
        {"GET", "/api/v1/archive/games", &Impl::handleArchiveList},
        {"GET", "/api/v1/archive/games/:id", &Impl::handleArchiveGet},
        {"GET", "/api/v1/archive/games/:id/transcript", &Impl::handleArchiveTranscript},
        {"GET", "/api/v1/archive/games/:id/octi", &Impl::handleArchiveOcti},
        {"GET", "/api/v1/archive/games/:id/plies", &Impl::handleArchivePlies},
        {"GET", "/api/v1/archive/stats", &Impl::handleArchiveStats},
        {"GET", "/api/v1/archive/export", &Impl::handleArchiveExport},
        {"POST", "/api/v1/telemetry/crash", &Impl::handleTelemetryCrash},
        {"GET", "/api/v1/telemetry/crashes", &Impl::handleTelemetryCrashes},
        {"GET", "/api/v1/telemetry/crashes/:id", &Impl::handleTelemetryCrashGet},
    };

    server.set_default_headers(
        {{"X-Content-Type-Options", "nosniff"},
         {"Referrer-Policy", "no-referrer"},
         {"X-Frame-Options", "DENY"}});

    for (size_t i = 0; i < sizeof(routes) / sizeof(routes[0]); ++i) {
        Handler h = routes[i].handler;
        httplib::Server::Handler fn = [this, h](const Request &rq,
                                                Response &rs) {
            (this->*h)(rq, rs);
        };
        const std::string method = routes[i].method;
        if (method == "GET") {
            server.Get(routes[i].pattern, fn);
        } else if (method == "POST") {
            server.Post(routes[i].pattern, fn);
        } else if (method == "PATCH") {
            server.Patch(routes[i].pattern, fn);
        } else {
            server.Delete(routes[i].pattern, fn);
        }
    }

    /*
     * Nothing but JSON is ever read: a request that announces a body of
     * any other type (audio, octet-stream, multipart...) is refused before
     * a single body byte is consumed.
     */
    server.set_pre_routing_handler(
        [this](const Request &rq, Response &rs) -> httplib::Server::HandlerResponse {
            if (rq.path != "/health" && rq.path != "/api/v1/health" &&
                rq.path != "/robots.txt") {
                const int64_t nowSec = store.currentEpochSec();
                BanMatch ipMatch;
                if (banManager.matchIp(clientRemoteIp(rq), nowSec, ipMatch)) {
                    sendIpBanned(rs, ipMatch, nowSec);
                    return httplib::Server::HandlerResponse::Handled;
                }
            }

            if (rq.path != "/api/v1/health" &&
                (rq.path == "/api" || rq.path.rfind("/api/", 0) == 0)) {
                if (rq.has_header("User-Agent") &&
                    isDisallowedCrawlerUserAgent(rq.get_header_value("User-Agent")) &&
                    !hasValidBearerAuth(rq)) {
                    applyBanSideEffects(banManager.recordCrawlerUaHit(
                        store, clientRemoteIp(rq), store.currentEpochSec()));
                    sendError(rs, 403, "FORBIDDEN_CRAWLER",
                              "automated scraping is prohibited");
                    return httplib::Server::HandlerResponse::Handled;
                }
            }

            if (rq.path != "/api/v1/health" &&
                (rq.path == "/api/v1" || rq.path.rfind("/api/v1/", 0) == 0)) {
                int clientProto = 1;
                if (rq.has_header("X-Octi-Protocol-Version")) {
                    std::string val = rq.get_header_value("X-Octi-Protocol-Version");
                    if (val.empty()) {
                        applyBanSideEffects(banManager.recordExploitProbe(
                            store, buildAbuseContext(rq, 0),
                            store.currentEpochSec()));
                        sendError(rs, 400, "INVALID_PROTOCOL_VERSION",
                                  "invalid X-Octi-Protocol-Version header");
                        return httplib::Server::HandlerResponse::Handled;
                    }
                    for (size_t i = 0; i < val.size(); ++i) {
                        if (val[i] < '0' || val[i] > '9') {
                            applyBanSideEffects(banManager.recordExploitProbe(
                                store, buildAbuseContext(rq, 0),
                                store.currentEpochSec()));
                            sendError(rs, 400, "INVALID_PROTOCOL_VERSION",
                                      "invalid X-Octi-Protocol-Version header");
                            return httplib::Server::HandlerResponse::Handled;
                        }
                    }
                    try {
                        long long v = std::stoll(val);
                        if (v < 1 || v > 1000000) {
                            applyBanSideEffects(banManager.recordExploitProbe(
                                store, buildAbuseContext(rq, 0),
                                store.currentEpochSec()));
                            sendError(rs, 400, "INVALID_PROTOCOL_VERSION",
                                      "invalid X-Octi-Protocol-Version header");
                            return httplib::Server::HandlerResponse::Handled;
                        }
                        clientProto = static_cast<int>(v);
                    } catch (...) {
                        applyBanSideEffects(banManager.recordExploitProbe(
                            store, buildAbuseContext(rq, 0),
                            store.currentEpochSec()));
                        sendError(rs, 400, "INVALID_PROTOCOL_VERSION",
                                  "invalid X-Octi-Protocol-Version header");
                        return httplib::Server::HandlerResponse::Handled;
                    }
                }
                if (clientProto < cfg.minProtocolVersion) {
                    Json j;
                    j["error"] = "UPGRADE_REQUIRED";
                    j["message"] =
                        "client protocol version is outdated; please upgrade OCTI";
                    j["protocol_version"] = cfg.protocolVersion;
                    j["min_protocol_version"] = cfg.minProtocolVersion;
                    sendJson(rs, 426, j);
                    return httplib::Server::HandlerResponse::Handled;
                }
            }

            if (rq.method == "GET" || rq.method == "HEAD" ||
                rq.method == "OPTIONS") {
                return httplib::Server::HandlerResponse::Unhandled;
            }

            if (rq.has_header("Content-Length")) {
                std::string clStr = rq.get_header_value("Content-Length");
                size_t first = clStr.find_first_not_of(" \t");
                if (first != std::string::npos) {
                    size_t last = clStr.find_last_not_of(" \t");
                    clStr = clStr.substr(first, last - first + 1);
                }
                int64_t contentLen = 0;
                if (!parseUnsignedStrict(clStr, INT64_MAX / 2, contentLen)) {
                    applyBanSideEffects(banManager.recordExploitProbe(
                        store, buildAbuseContext(rq, 0),
                        store.currentEpochSec()));
                    sendError(rs, 400, "BAD_REQUEST",
                              "invalid Content-Length header");
                    return httplib::Server::HandlerResponse::Handled;
                }
                const size_t maxBytes = routeMaxBodyBytes(rq.method, rq.path);
                if (static_cast<size_t>(contentLen) > maxBytes) {
                    sendError(rs, 413, "PAYLOAD_TOO_LARGE",
                              "request body is too large");
                    return httplib::Server::HandlerResponse::Handled;
                }
            }

            const bool hasBody = rq.get_header_value("Content-Length") != "" ||
                                 rq.has_header("Transfer-Encoding");
            const bool empty = rq.get_header_value("Content-Length") == "0" &&
                               !rq.has_header("Transfer-Encoding");
            if (hasBody && !empty) {
                const std::string ct = rq.get_header_value("Content-Type");
                if (rq.path == "/api/v1/auth/data-deletion" ||
                    rq.path == "/privacy") {
                    if (!isJsonContentType(ct) &&
                        !isFormUrlEncodedContentType(ct)) {
                        sendError(rs, 415, "UNSUPPORTED_MEDIA_TYPE",
                                  "only application/json or form-urlencoded "
                                  "request bodies are accepted");
                        return httplib::Server::HandlerResponse::Handled;
                    }
                } else if (!isJsonContentType(ct)) {
                    sendError(rs, 415, "UNSUPPORTED_MEDIA_TYPE",
                              "only application/json request bodies are accepted");
                    return httplib::Server::HandlerResponse::Handled;
                }
            }
            return httplib::Server::HandlerResponse::Unhandled;
        });

    server.set_error_handler(
        [](const Request &, Response &rs) -> httplib::Server::HandlerResponse {
            if (!rs.body.empty() || rs.content_provider_) {
                return httplib::Server::HandlerResponse::Unhandled;
            }
            std::string code = "ERROR";
            switch (rs.status) {
            case 404: code = "NOT_FOUND"; break;
            case 405: code = "METHOD_NOT_ALLOWED"; break;
            case 413: code = "PAYLOAD_TOO_LARGE"; break;
            case 408: code = "REQUEST_TIMEOUT"; break;
            case 400: code = "BAD_REQUEST"; break;
            default:
                if (rs.status >= 500) {
                    code = "INTERNAL_ERROR";
                }
                break;
            }
            sendError(rs, rs.status, code,
                      rs.status == 413 ? "request body is too large"
                                       : "request failed");
            return httplib::Server::HandlerResponse::Handled;
        });

    server.set_exception_handler(
        [](const Request &, Response &rs, std::exception_ptr) {
            sendError(rs, 500, "INTERNAL_ERROR", "internal server error");
        });
}

/* ------------------------------------------------------------------ */
/* Public facade                                                       */
/* ------------------------------------------------------------------ */

OctiHttpServer::OctiHttpServer(AccountStore &store, OidcVerifier &verifier,
                               GameRoomManager &rooms,
                               const HttpServerConfig &config)
    : _impl(new Impl(store, verifier, rooms, config))
{
}

OctiHttpServer::~OctiHttpServer()
{
    stop();
}

bool OctiHttpServer::start(std::string &error)
{
    return _impl->start(error);
}

void OctiHttpServer::stop()
{
    _impl->stop();
}

bool OctiHttpServer::isRunning() const
{
    return _impl->server.is_running();
}

int OctiHttpServer::port() const
{
    return _impl->boundPort.load();
}

const std::string &OctiHttpServer::bindAddress() const
{
    return _impl->cfg.bindAddress;
}

uint64_t OctiHttpServer::tickCount() const
{
    return _impl->ticks.load();
}

int OctiHttpServer::activeStreams() const
{
    return _impl->sseActive.load();
}

int OctiHttpServer::activeExports() const
{
    return _impl->activeExportsCount.load();
}

std::string OctiHttpServer::normalizeIpLiteral(const std::string &raw)
{
    size_t first = raw.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return std::string();
    }
    size_t last = raw.find_last_not_of(" \t\r\n");
    std::string s = raw.substr(first, last - first + 1);
    if (s.empty() || s.size() > 128) {
        return std::string();
    }
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 0x20 || c >= 0x7F) {
            return std::string();
        }
    }

    struct in_addr a4;
    if (::inet_pton(AF_INET, s.c_str(), &a4) == 1) {
        char buf[INET_ADDRSTRLEN];
        if (::inet_ntop(AF_INET, &a4, buf, sizeof(buf)) != nullptr) {
            return std::string(buf);
        }
        return std::string();
    }

    struct in6_addr a6;
    if (::inet_pton(AF_INET6, s.c_str(), &a6) == 1) {
        bool v4Mapped = true;
        for (int i = 0; i < 10; ++i) {
            if (a6.s6_addr[i] != 0) {
                v4Mapped = false;
                break;
            }
        }
        if (v4Mapped && a6.s6_addr[10] == 0xFF && a6.s6_addr[11] == 0xFF) {
            struct in_addr mapped4;
            std::memcpy(&mapped4.s_addr, a6.s6_addr + 12, 4);
            char buf4[INET_ADDRSTRLEN];
            if (::inet_ntop(AF_INET, &mapped4, buf4, sizeof(buf4)) != nullptr) {
                return std::string(buf4);
            }
            return std::string();
        }
        char buf6[INET6_ADDRSTRLEN];
        if (::inet_ntop(AF_INET6, &a6, buf6, sizeof(buf6)) != nullptr) {
            return lower(std::string(buf6));
        }
    }
    return std::string();
}

std::string OctiHttpServer::resolveClientIp(
    const std::string &peerAddr,
    const std::string &xffHeader,
    const std::string &xRealIpHeader,
    const std::vector<std::string> &trustedProxies)
{
    std::string peer = normalizeIpLiteral(peerAddr);
    if (peer.empty()) {
        return peerAddr.empty() ? std::string("unknown") : peerAddr;
    }

    auto isTrusted = [&](const std::string &normIp) -> bool {
        if (normIp.empty()) {
            return false;
        }
        for (size_t i = 0; i < trustedProxies.size(); ++i) {
            if (trustedProxies[i] == normIp ||
                normalizeIpLiteral(trustedProxies[i]) == normIp) {
                return true;
            }
        }
        return false;
    };

    if (!isTrusted(peer)) {
        return peer;
    }

    size_t xffFirst = xffHeader.find_first_not_of(" \t\r\n");
    if (xffFirst != std::string::npos) {
        if (xffHeader.size() > 2048) {
            return peer;
        }
        std::vector<std::string> hops;
        size_t start = 0;
        while (start <= xffHeader.size()) {
            size_t comma = xffHeader.find(',', start);
            if (comma == std::string::npos) {
                hops.push_back(xffHeader.substr(start));
                break;
            }
            hops.push_back(xffHeader.substr(start, comma - start));
            start = comma + 1;
        }
        for (size_t idx = hops.size(); idx > 0; --idx) {
            std::string hopIp = normalizeIpLiteral(hops[idx - 1]);
            if (hopIp.empty()) {
                return peer;
            }
            if (!isTrusted(hopIp)) {
                return hopIp;
            }
        }
    }

    size_t xriFirst = xRealIpHeader.find_first_not_of(" \t\r\n");
    if (xriFirst != std::string::npos && xRealIpHeader.size() <= 256) {
        std::string realIp = normalizeIpLiteral(xRealIpHeader);
        if (realIp.empty()) {
            return peer;
        }
        if (!isTrusted(realIp)) {
            return realIp;
        }
    }

    return peer;
}

} // namespace octi

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
