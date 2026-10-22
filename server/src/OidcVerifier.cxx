/*
 * OidcVerifier.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "OidcVerifier.hxx"

#ifndef CPPHTTPLIB_OPENSSL_SUPPORT
#define CPPHTTPLIB_OPENSSL_SUPPORT
#endif
#include <httplib.h>
#include <nlohmann/json.hpp>

#include <openssl/bn.h>
#include <openssl/core_names.h>
#include <openssl/ecdsa.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/param_build.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>

#if OPENSSL_VERSION_NUMBER < 0x30000000L
#error "OidcVerifier requires OpenSSL 3.0 or newer"
#endif

namespace octi {

namespace {

typedef nlohmann::json Json;

const size_t kMaxTokenBytes = 16384;
const size_t kMaxJwksBytes = 262144;
const size_t kMaxHttpBodyBytes = 1048576;
const size_t kMaxSubBytes = 255;
const size_t kMaxJwksKeys = 64;
const size_t kMaxAccessTokenBytes = 4096;
const int64_t kMaxNumericDate = 253402300799LL;
const int64_t kMinRefreshIntervalSec = 30;
const int kMinRsaBits = 2048;
const int kMaxRsaBits = 8192;

struct PkeyDeleter {
    void operator()(EVP_PKEY *p) const { EVP_PKEY_free(p); }
};
struct PkeyCtxDeleter {
    void operator()(EVP_PKEY_CTX *p) const { EVP_PKEY_CTX_free(p); }
};
struct BnDeleter {
    void operator()(BIGNUM *p) const { BN_free(p); }
};
struct BldDeleter {
    void operator()(OSSL_PARAM_BLD *p) const { OSSL_PARAM_BLD_free(p); }
};
struct ParamDeleter {
    void operator()(OSSL_PARAM *p) const { OSSL_PARAM_free(p); }
};
struct MdCtxDeleter {
    void operator()(EVP_MD_CTX *p) const { EVP_MD_CTX_free(p); }
};
struct EcdsaSigDeleter {
    void operator()(ECDSA_SIG *p) const { ECDSA_SIG_free(p); }
};

typedef std::unique_ptr<EVP_PKEY, PkeyDeleter> PkeyPtr;
typedef std::unique_ptr<EVP_PKEY_CTX, PkeyCtxDeleter> PkeyCtxPtr;
typedef std::unique_ptr<BIGNUM, BnDeleter> BnPtr;
typedef std::unique_ptr<OSSL_PARAM_BLD, BldDeleter> BldPtr;
typedef std::unique_ptr<OSSL_PARAM, ParamDeleter> ParamPtr;
typedef std::unique_ptr<EVP_MD_CTX, MdCtxDeleter> MdCtxPtr;
typedef std::unique_ptr<ECDSA_SIG, EcdsaSigDeleter> EcdsaSigPtr;

const char kB64Alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

int b64Value(unsigned char c)
{
    if (c >= 'A' && c <= 'Z') {
        return c - 'A';
    }
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 26;
    }
    if (c >= '0' && c <= '9') {
        return c - '0' + 52;
    }
    if (c == '-') {
        return 62;
    }
    if (c == '_') {
        return 63;
    }
    return -1;
}

VerifyResult fail(VerifyStatus status, const char *detail)
{
    VerifyResult r;
    r.status = status;
    r.detail = detail;
    return r;
}

bool containsString(const std::vector<std::string> &list, const std::string &v)
{
    return std::find(list.begin(), list.end(), v) != list.end();
}

bool jsonString(const Json &obj, const char *key, std::string &out)
{
    Json::const_iterator it = obj.find(key);
    if (it == obj.end() || !it->is_string()) {
        return false;
    }
    out = it->get<std::string>();
    return true;
}

enum NumState { NUM_ABSENT, NUM_OK, NUM_INVALID };

NumState jsonNumericDate(const Json &obj, const char *key, int64_t &out)
{
    Json::const_iterator it = obj.find(key);
    if (it == obj.end()) {
        return NUM_ABSENT;
    }
    if (it->is_number_unsigned()) {
        uint64_t v = it->get<uint64_t>();
        if (v > static_cast<uint64_t>(kMaxNumericDate)) {
            return NUM_INVALID;
        }
        out = static_cast<int64_t>(v);
        return NUM_OK;
    }
    if (it->is_number_integer()) {
        int64_t v = it->get<int64_t>();
        if (v < 0 || v > kMaxNumericDate) {
            return NUM_INVALID;
        }
        out = v;
        return NUM_OK;
    }
    if (it->is_number_float()) {
        double d = it->get<double>();
        if (!(d >= 0.0 && d <= static_cast<double>(kMaxNumericDate))) {
            return NUM_INVALID;
        }
        out = static_cast<int64_t>(std::floor(d));
        return NUM_OK;
    }
    return NUM_INVALID;
}

bool endsWith(const std::string &s, const std::string &suffix)
{
    return s.size() >= suffix.size() &&
           s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool startsWith(const std::string &s, const std::string &prefix)
{
    return s.size() >= prefix.size() &&
           s.compare(0, prefix.size(), prefix) == 0;
}

/* https://login.microsoftonline.com/<tenant>/v2.0 -> <tenant> */
bool microsoftTenantFromIssuer(const std::string &iss, std::string &tenant)
{
    const std::string prefix = "https://login.microsoftonline.com/";
    const std::string suffix = "/v2.0";
    if (iss.size() <= prefix.size() + suffix.size() ||
        !startsWith(iss, prefix) || !endsWith(iss, suffix)) {
        return false;
    }
    std::string t = iss.substr(prefix.size(),
                               iss.size() - prefix.size() - suffix.size());
    for (size_t i = 0; i < t.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(t[i]);
        bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                  (c >= '0' && c <= '9') || c == '-' || c == '.';
        if (!ok) {
            return false;
        }
    }
    tenant = t;
    return true;
}

bool audienceMatches(const Json &payload,
                     const std::vector<std::string> &clientIds)
{
    Json::const_iterator it = payload.find("aud");
    if (it == payload.end()) {
        return false;
    }
    if (it->is_string()) {
        return containsString(clientIds, it->get<std::string>());
    }
    if (it->is_array()) {
        bool matched = false;
        for (Json::const_iterator e = it->begin(); e != it->end(); ++e) {
            if (e->is_string() &&
                containsString(clientIds, e->get<std::string>())) {
                matched = true;
            }
        }
        if (matched && it->size() > 1) {
            /* OIDC core 3.1.3.7: multiple audiences require azp == client id */
            std::string azp;
            if (!jsonString(payload, "azp", azp) ||
                !containsString(clientIds, azp)) {
                return false;
            }
        }
        return matched;
    }
    return false;
}

std::string urlEncode(const std::string &s)
{
    static const char hex[] = "0123456789ABCDEF";
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        bool unreserved = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                          (c >= '0' && c <= '9') || c == '-' || c == '_' ||
                          c == '.' || c == '~';
        if (unreserved) {
            out.push_back(static_cast<char>(c));
        } else {
            out.push_back('%');
            out.push_back(hex[c >> 4]);
            out.push_back(hex[c & 0x0F]);
        }
    }
    return out;
}

PkeyPtr buildKey(const char *keyType, OSSL_PARAM *params)
{
    PkeyCtxPtr ctx(EVP_PKEY_CTX_new_from_name(nullptr, keyType, nullptr));
    if (!ctx || EVP_PKEY_fromdata_init(ctx.get()) != 1) {
        return PkeyPtr();
    }
    EVP_PKEY *raw = nullptr;
    if (EVP_PKEY_fromdata(ctx.get(), &raw, EVP_PKEY_PUBLIC_KEY, params) != 1) {
        return PkeyPtr();
    }
    PkeyPtr pkey(raw);
    PkeyCtxPtr check(EVP_PKEY_CTX_new_from_pkey(nullptr, pkey.get(), nullptr));
    if (!check || EVP_PKEY_public_check(check.get()) != 1) {
        return PkeyPtr();
    }
    return pkey;
}

PkeyPtr rsaKeyFromJwk(const std::string &nBytes, const std::string &eBytes)
{
    if (nBytes.empty() || eBytes.empty() || eBytes.size() > 8) {
        return PkeyPtr();
    }
    BnPtr n(BN_bin2bn(reinterpret_cast<const unsigned char *>(nBytes.data()),
                      static_cast<int>(nBytes.size()), nullptr));
    BnPtr e(BN_bin2bn(reinterpret_cast<const unsigned char *>(eBytes.data()),
                      static_cast<int>(eBytes.size()), nullptr));
    if (!n || !e) {
        return PkeyPtr();
    }
    int bits = BN_num_bits(n.get());
    if (bits < kMinRsaBits || bits > kMaxRsaBits || !BN_is_odd(n.get()) ||
        !BN_is_odd(e.get()) || BN_is_one(e.get())) {
        return PkeyPtr();
    }
    BldPtr bld(OSSL_PARAM_BLD_new());
    if (!bld ||
        OSSL_PARAM_BLD_push_BN(bld.get(), OSSL_PKEY_PARAM_RSA_N, n.get()) != 1 ||
        OSSL_PARAM_BLD_push_BN(bld.get(), OSSL_PKEY_PARAM_RSA_E, e.get()) != 1) {
        return PkeyPtr();
    }
    ParamPtr params(OSSL_PARAM_BLD_to_param(bld.get()));
    if (!params) {
        return PkeyPtr();
    }
    return buildKey("RSA", params.get());
}

PkeyPtr ecKeyFromJwk(const std::string &x, const std::string &y)
{
    if (x.size() != 32 || y.size() != 32) {
        return PkeyPtr();
    }
    unsigned char pub[65];
    pub[0] = 0x04;
    std::memcpy(pub + 1, x.data(), 32);
    std::memcpy(pub + 33, y.data(), 32);
    BldPtr bld(OSSL_PARAM_BLD_new());
    if (!bld ||
        OSSL_PARAM_BLD_push_utf8_string(bld.get(), OSSL_PKEY_PARAM_GROUP_NAME,
                                        "P-256", 0) != 1 ||
        OSSL_PARAM_BLD_push_octet_string(bld.get(), OSSL_PKEY_PARAM_PUB_KEY,
                                         pub, sizeof(pub)) != 1) {
        return PkeyPtr();
    }
    ParamPtr params(OSSL_PARAM_BLD_to_param(bld.get()));
    if (!params) {
        return PkeyPtr();
    }
    return buildKey("EC", params.get());
}

bool verifySignature(EVP_PKEY *pkey, const std::string &alg,
                     const std::string &signingInput, const std::string &sig)
{
    std::string der;
    const unsigned char *sigData =
        reinterpret_cast<const unsigned char *>(sig.data());
    size_t sigLen = sig.size();

    if (alg == "ES256") {
        if (sig.size() != 64) {
            return false;
        }
        BIGNUM *r = BN_bin2bn(sigData, 32, nullptr);
        BIGNUM *s = BN_bin2bn(sigData + 32, 32, nullptr);
        EcdsaSigPtr es(ECDSA_SIG_new());
        if (!r || !s || !es || ECDSA_SIG_set0(es.get(), r, s) != 1) {
            BN_free(r);
            BN_free(s);
            return false;
        }
        int derLen = i2d_ECDSA_SIG(es.get(), nullptr);
        if (derLen <= 0) {
            return false;
        }
        der.resize(static_cast<size_t>(derLen));
        unsigned char *out = reinterpret_cast<unsigned char *>(&der[0]);
        if (i2d_ECDSA_SIG(es.get(), &out) != derLen) {
            return false;
        }
        sigData = reinterpret_cast<const unsigned char *>(der.data());
        sigLen = der.size();
    } else if (alg != "RS256") {
        return false;
    }

    MdCtxPtr md(EVP_MD_CTX_new());
    if (!md ||
        EVP_DigestVerifyInit(md.get(), nullptr, EVP_sha256(), nullptr,
                             pkey) != 1) {
        ERR_clear_error();
        return false;
    }
    int rc = EVP_DigestVerify(
        md.get(), sigData, sigLen,
        reinterpret_cast<const unsigned char *>(signingInput.data()),
        signingInput.size());
    ERR_clear_error();
    return rc == 1;
}

} // namespace

/* ------------------------------------------------------------------ */
/* Base64URL                                                           */
/* ------------------------------------------------------------------ */

std::string base64UrlEncode(const void *data, size_t len)
{
    const unsigned char *p = static_cast<const unsigned char *>(data);
    std::string out;
    out.reserve((len * 4 + 2) / 3);
    size_t i = 0;
    for (; i + 3 <= len; i += 3) {
        uint32_t v = (static_cast<uint32_t>(p[i]) << 16) |
                     (static_cast<uint32_t>(p[i + 1]) << 8) | p[i + 2];
        out.push_back(kB64Alphabet[(v >> 18) & 0x3F]);
        out.push_back(kB64Alphabet[(v >> 12) & 0x3F]);
        out.push_back(kB64Alphabet[(v >> 6) & 0x3F]);
        out.push_back(kB64Alphabet[v & 0x3F]);
    }
    size_t rem = len - i;
    if (rem == 1) {
        uint32_t v = static_cast<uint32_t>(p[i]) << 16;
        out.push_back(kB64Alphabet[(v >> 18) & 0x3F]);
        out.push_back(kB64Alphabet[(v >> 12) & 0x3F]);
    } else if (rem == 2) {
        uint32_t v = (static_cast<uint32_t>(p[i]) << 16) |
                     (static_cast<uint32_t>(p[i + 1]) << 8);
        out.push_back(kB64Alphabet[(v >> 18) & 0x3F]);
        out.push_back(kB64Alphabet[(v >> 12) & 0x3F]);
        out.push_back(kB64Alphabet[(v >> 6) & 0x3F]);
    }
    return out;
}

std::string base64UrlEncode(const std::string &bytes)
{
    return base64UrlEncode(bytes.data(), bytes.size());
}

bool base64UrlDecode(const std::string &text, std::string &out)
{
    out.clear();
    size_t n = text.size();
    if (n % 4 == 1) {
        return false;
    }
    std::string result;
    result.reserve(n / 4 * 3 + 2);
    size_t i = 0;
    for (; i + 4 <= n; i += 4) {
        int a = b64Value(static_cast<unsigned char>(text[i]));
        int b = b64Value(static_cast<unsigned char>(text[i + 1]));
        int c = b64Value(static_cast<unsigned char>(text[i + 2]));
        int d = b64Value(static_cast<unsigned char>(text[i + 3]));
        if (a < 0 || b < 0 || c < 0 || d < 0) {
            return false;
        }
        result.push_back(static_cast<char>((a << 2) | (b >> 4)));
        result.push_back(static_cast<char>(((b & 0x0F) << 4) | (c >> 2)));
        result.push_back(static_cast<char>(((c & 0x03) << 6) | d));
    }
    size_t rem = n - i;
    if (rem == 2) {
        int a = b64Value(static_cast<unsigned char>(text[i]));
        int b = b64Value(static_cast<unsigned char>(text[i + 1]));
        if (a < 0 || b < 0 || (b & 0x0F) != 0) {
            return false;
        }
        result.push_back(static_cast<char>((a << 2) | (b >> 4)));
    } else if (rem == 3) {
        int a = b64Value(static_cast<unsigned char>(text[i]));
        int b = b64Value(static_cast<unsigned char>(text[i + 1]));
        int c = b64Value(static_cast<unsigned char>(text[i + 2]));
        if (a < 0 || b < 0 || c < 0 || (c & 0x03) != 0) {
            return false;
        }
        result.push_back(static_cast<char>((a << 2) | (b >> 4)));
        result.push_back(static_cast<char>(((b & 0x0F) << 4) | (c >> 2)));
    }
    out.swap(result);
    return true;
}

/* ------------------------------------------------------------------ */
/* HTTP boundary                                                       */
/* ------------------------------------------------------------------ */

HttpFetcher defaultHttpFetcher()
{
    return [](const std::string &url, HttpResponse &response) -> bool {
        const std::string scheme = "https://";
        if (!startsWith(url, scheme)) {
            return false;
        }
        size_t pathPos = url.find('/', scheme.size());
        std::string hostPort =
            (pathPos == std::string::npos)
                ? url.substr(scheme.size())
                : url.substr(scheme.size(), pathPos - scheme.size());
        std::string path =
            (pathPos == std::string::npos) ? "/" : url.substr(pathPos);
        if (hostPort.empty()) {
            return false;
        }
        httplib::Client client(scheme + hostPort);
        client.set_connection_timeout(5, 0);
        client.set_read_timeout(10, 0);
        client.set_write_timeout(5, 0);
        client.enable_server_certificate_verification(true);
        httplib::Result res = client.Get(path);
        if (!res || res->body.size() > kMaxHttpBodyBytes) {
            return false;
        }
        response.status = res->status;
        response.body = res->body;
        return true;
    };
}

const char *verifyStatusName(VerifyStatus status)
{
    switch (status) {
    case VerifyStatus::OK:
        return "OK";
    case VerifyStatus::MALFORMED:
        return "MALFORMED";
    case VerifyStatus::UNSUPPORTED_PROVIDER:
        return "UNSUPPORTED_PROVIDER";
    case VerifyStatus::PROVIDER_DISABLED:
        return "PROVIDER_DISABLED";
    case VerifyStatus::UNSUPPORTED_ALG:
        return "UNSUPPORTED_ALG";
    case VerifyStatus::KEY_SET_UNAVAILABLE:
        return "KEY_SET_UNAVAILABLE";
    case VerifyStatus::UNKNOWN_KEY:
        return "UNKNOWN_KEY";
    case VerifyStatus::BAD_SIGNATURE:
        return "BAD_SIGNATURE";
    case VerifyStatus::BAD_ISSUER:
        return "BAD_ISSUER";
    case VerifyStatus::BAD_AUDIENCE:
        return "BAD_AUDIENCE";
    case VerifyStatus::EXPIRED:
        return "EXPIRED";
    case VerifyStatus::NOT_YET_VALID:
        return "NOT_YET_VALID";
    case VerifyStatus::MISSING_SUB:
        return "MISSING_SUB";
    case VerifyStatus::TOKEN_REJECTED:
        return "TOKEN_REJECTED";
    case VerifyStatus::REMOTE_FAILURE:
        return "REMOTE_FAILURE";
    }
    return "UNKNOWN";
}

/* ------------------------------------------------------------------ */
/* OidcVerifier                                                        */
/* ------------------------------------------------------------------ */

const std::vector<ProviderDescriptor> &OidcVerifier::providerRegistry()
{
    static const std::vector<ProviderDescriptor> registry = []() {
        std::vector<ProviderDescriptor> r;
        ProviderDescriptor d;

        d = ProviderDescriptor();
        d.name = "google";
        d.displayName = "Google";
        d.issuers.push_back("https://accounts.google.com");
        d.issuers.push_back("accounts.google.com");
        d.jwksUrl = "https://www.googleapis.com/oauth2/v3/certs";
        d.algorithms.push_back("RS256");
        r.push_back(d);

        d = ProviderDescriptor();
        d.name = "apple";
        d.displayName = "Apple";
        d.issuers.push_back("https://appleid.apple.com");
        d.jwksUrl = "https://appleid.apple.com/auth/keys";
        d.algorithms.push_back("RS256");
        r.push_back(d);

        d = ProviderDescriptor();
        d.name = "microsoft";
        d.displayName = "Microsoft";
        d.tenantIssuer = true;
        d.jwksUrl = "https://login.microsoftonline.com/common/discovery/v2.0/keys";
        d.algorithms.push_back("RS256");
        r.push_back(d);

        d = ProviderDescriptor();
        d.name = "facebook";
        d.displayName = "Facebook";
        d.issuers.push_back("https://www.facebook.com");
        d.jwksUrl = "https://www.facebook.com/.well-known/oauth/openid/jwks/";
        d.algorithms.push_back("RS256");
        r.push_back(d);

        d = ProviderDescriptor();
        d.name = "line";
        d.displayName = "LINE";
        d.issuers.push_back("https://access.line.me");
        d.jwksUrl = "https://api.line.me/oauth2/v2.1/certs";
        d.algorithms.push_back("RS256");
        d.algorithms.push_back("ES256");
        r.push_back(d);
        return r;
    }();
    return registry;
}

bool OidcVerifier::isSupportedProvider(const std::string &name)
{
    const std::vector<ProviderDescriptor> &reg = providerRegistry();
    for (size_t i = 0; i < reg.size(); ++i) {
        if (reg[i].name == name) {
            return true;
        }
    }
    return false;
}

OidcVerifier::OidcVerifier()
    : _clockSkewSec(60), _keyTtlSec(3600)
{
    const std::vector<ProviderDescriptor> &reg = providerRegistry();
    for (size_t i = 0; i < reg.size(); ++i) {
        ProviderState ps;
        ps.descriptor = reg[i];
        _providers.push_back(ps);
    }
    _clock = []() -> int64_t {
        return std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::system_clock::now().time_since_epoch())
            .count();
    };
}

OidcVerifier::~OidcVerifier()
{
}

int OidcVerifier::findProvider(const std::string &name) const
{
    for (size_t i = 0; i < _providers.size(); ++i) {
        if (_providers[i].descriptor.name == name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int64_t OidcVerifier::nowSec() const
{
    std::function<int64_t()> clock;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        clock = _clock;
    }
    return clock();
}

bool OidcVerifier::setClientId(const std::string &provider,
                               const std::string &clientId)
{
    std::vector<std::string> ids;
    if (!clientId.empty()) {
        ids.push_back(clientId);
    }
    return setClientIds(provider, ids);
}

bool OidcVerifier::setClientIds(const std::string &provider,
                                const std::vector<std::string> &clientIds)
{
    std::vector<std::string> cleaned;
    for (size_t i = 0; i < clientIds.size(); ++i) {
        if (!clientIds[i].empty() && !containsString(cleaned, clientIds[i])) {
            cleaned.push_back(clientIds[i]);
        }
    }
    std::lock_guard<std::mutex> lock(_mutex);
    int idx = findProvider(provider);
    if (idx < 0) {
        return false;
    }
    _providers[static_cast<size_t>(idx)].clientIds = cleaned;
    return true;
}

bool OidcVerifier::setAllowedAlgorithms(
    const std::string &provider, const std::vector<std::string> &algorithms)
{
    if (algorithms.empty()) {
        return false;
    }
    for (size_t i = 0; i < algorithms.size(); ++i) {
        if (algorithms[i] != "RS256" && algorithms[i] != "ES256") {
            return false;
        }
    }
    std::lock_guard<std::mutex> lock(_mutex);
    int idx = findProvider(provider);
    if (idx < 0) {
        return false;
    }
    _providers[static_cast<size_t>(idx)].descriptor.algorithms = algorithms;
    return true;
}

void OidcVerifier::setClockSkewSec(int64_t seconds)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _clockSkewSec = seconds < 0 ? 0 : (seconds > 86400 ? 86400 : seconds);
}

void OidcVerifier::setKeyTtlSec(int64_t seconds)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _keyTtlSec = seconds < 0 ? 0 : (seconds > 31536000 ? 31536000 : seconds);
}

void OidcVerifier::setClock(std::function<int64_t()> clock)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (clock) {
        _clock = clock;
    } else {
        _clock = []() -> int64_t {
            return std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                .count();
        };
    }
}

void OidcVerifier::setHttpFetcher(HttpFetcher fetcher)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _fetcher = fetcher;
}

void OidcVerifier::setFacebookAppSecret(const std::string &secret)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _facebookAppSecret = secret;
}

std::vector<std::string> OidcVerifier::enabledProviders() const
{
    std::vector<std::string> out;
    std::lock_guard<std::mutex> lock(_mutex);
    for (size_t i = 0; i < _providers.size(); ++i) {
        if (!_providers[i].clientIds.empty()) {
            out.push_back(_providers[i].descriptor.name);
        }
    }
    return out;
}

bool OidcVerifier::isProviderEnabled(const std::string &provider) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    int idx = findProvider(provider);
    return idx >= 0 && !_providers[static_cast<size_t>(idx)].clientIds.empty();
}

/* ---- JWKS ---- */

bool OidcVerifier::installKeySet(size_t index, const std::string &jwksJson,
                                 int64_t now) const
{
    if (jwksJson.empty() || jwksJson.size() > kMaxJwksBytes) {
        return false;
    }
    Json doc = Json::parse(jwksJson, nullptr, false);
    if (doc.is_discarded() || !doc.is_object()) {
        return false;
    }
    Json::const_iterator keysIt = doc.find("keys");
    if (keysIt == doc.end() || !keysIt->is_array()) {
        return false;
    }

    std::vector<KeyEntry> parsed;
    for (Json::const_iterator it = keysIt->begin(); it != keysIt->end(); ++it) {
        if (parsed.size() >= kMaxJwksKeys) {
            break;
        }
        if (!it->is_object()) {
            continue;
        }
        std::string kty, use, kid, alg;
        if (!jsonString(*it, "kty", kty)) {
            continue;
        }
        if (jsonString(*it, "use", use) && use != "sig") {
            continue;
        }
        jsonString(*it, "kid", kid);
        jsonString(*it, "alg", alg);

        PkeyPtr pkey;
        if (kty == "RSA") {
            std::string nText, eText, nBytes, eBytes;
            if (!jsonString(*it, "n", nText) || !jsonString(*it, "e", eText) ||
                !base64UrlDecode(nText, nBytes) ||
                !base64UrlDecode(eText, eBytes)) {
                continue;
            }
            pkey = rsaKeyFromJwk(nBytes, eBytes);
        } else if (kty == "EC") {
            std::string crv, xText, yText, xBytes, yBytes;
            if (!jsonString(*it, "crv", crv) || crv != "P-256" ||
                !jsonString(*it, "x", xText) || !jsonString(*it, "y", yText) ||
                !base64UrlDecode(xText, xBytes) ||
                !base64UrlDecode(yText, yBytes)) {
                continue;
            }
            pkey = ecKeyFromJwk(xBytes, yBytes);
        } else {
            continue;
        }
        if (!pkey) {
            ERR_clear_error();
            continue;
        }
        KeyEntry entry;
        entry.kid = kid;
        entry.kty = kty;
        entry.alg = alg;
        entry.key = std::shared_ptr<evp_pkey_st>(pkey.release(),
                                                 [](evp_pkey_st *p) {
                                                     EVP_PKEY_free(p);
                                                 });
        parsed.push_back(entry);
    }
    if (parsed.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(_mutex);
    ProviderState &ps = _providers[index];
    ps.keys.swap(parsed);
    ps.keysLoaded = true;
    ps.keysFetchedAt = now;
    return true;
}

bool OidcVerifier::setKeySet(const std::string &provider,
                             const std::string &jwksJson)
{
    return setKeySet(provider, jwksJson, nowSec());
}

bool OidcVerifier::setKeySet(const std::string &provider,
                             const std::string &jwksJson, int64_t now)
{
    int idx;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        idx = findProvider(provider);
    }
    if (idx < 0) {
        return false;
    }
    return installKeySet(static_cast<size_t>(idx), jwksJson, now);
}

bool OidcVerifier::refreshKeySet(const std::string &provider)
{
    return refreshKeySet(provider, nowSec());
}

bool OidcVerifier::refreshKeySet(const std::string &provider, int64_t now)
{
    return refreshKeySetImpl(provider, now);
}

bool OidcVerifier::refreshKeySetImpl(const std::string &provider,
                                     int64_t now) const
{
    int idx;
    std::string url;
    HttpFetcher fetcher;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        idx = findProvider(provider);
        if (idx < 0 || !_fetcher) {
            return false;
        }
        ProviderState &ps = _providers[static_cast<size_t>(idx)];
        ps.refreshAttempted = true;
        ps.lastRefreshAttempt = now;
        url = ps.descriptor.jwksUrl;
        fetcher = _fetcher;
    }
    HttpResponse resp;
    if (url.empty() || !fetcher(url, resp) || resp.status != 200) {
        return false;
    }
    return installKeySet(static_cast<size_t>(idx), resp.body, now);
}

VerifyStatus OidcVerifier::acquireKeys(size_t index, const std::string &alg,
                                       const std::string &kid, bool hasKid,
                                       int64_t now,
                                       std::vector<KeyEntry> &candidates) const
{
    const char *wantedKty = (alg == "ES256") ? "EC" : "RSA";
    for (int attempt = 0; attempt < 2; ++attempt) {
        std::string providerName;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            const ProviderState &ps = _providers[index];
            bool fresh = ps.keysLoaded &&
                         !(now > ps.keysFetchedAt &&
                           now - ps.keysFetchedAt > _keyTtlSec);
            candidates.clear();
            for (size_t i = 0; i < ps.keys.size(); ++i) {
                const KeyEntry &k = ps.keys[i];
                if (k.kty != wantedKty) {
                    continue;
                }
                if (!k.alg.empty() && k.alg != alg) {
                    continue;
                }
                if (hasKid && k.kid != kid) {
                    continue;
                }
                candidates.push_back(k);
            }
            if (fresh && !candidates.empty()) {
                return VerifyStatus::OK;
            }
            bool mayRefresh =
                attempt == 0 && _fetcher && !ps.descriptor.jwksUrl.empty() &&
                (!ps.refreshAttempted || now < ps.lastRefreshAttempt ||
                 now - ps.lastRefreshAttempt >= kMinRefreshIntervalSec);
            if (!mayRefresh) {
                candidates.clear();
                return fresh ? VerifyStatus::UNKNOWN_KEY
                             : VerifyStatus::KEY_SET_UNAVAILABLE;
            }
            providerName = ps.descriptor.name;
        }
        refreshKeySetImpl(providerName, now);
    }
    candidates.clear();
    return VerifyStatus::KEY_SET_UNAVAILABLE;
}

/* ---- Verification ---- */

VerifyResult OidcVerifier::verifyIdToken(const std::string &provider,
                                         const std::string &idToken) const
{
    return verifyIdToken(provider, idToken, nowSec());
}

VerifyResult OidcVerifier::verifyIdToken(const std::string &provider,
                                         const std::string &idToken,
                                         int64_t now) const
{
    ProviderDescriptor desc;
    std::vector<std::string> clientIds;
    size_t index = 0;
    int64_t skew = 0;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        int idx = findProvider(provider);
        if (idx < 0) {
            return fail(VerifyStatus::UNSUPPORTED_PROVIDER,
                        "unsupported provider");
        }
        index = static_cast<size_t>(idx);
        desc = _providers[index].descriptor;
        clientIds = _providers[index].clientIds;
        skew = _clockSkewSec;
    }
    if (clientIds.empty()) {
        return fail(VerifyStatus::PROVIDER_DISABLED, "provider not enabled");
    }
    if (idToken.empty() || idToken.size() > kMaxTokenBytes) {
        return fail(VerifyStatus::MALFORMED, "token size out of range");
    }

    size_t dot1 = idToken.find('.');
    size_t dot2 = (dot1 == std::string::npos) ? std::string::npos
                                              : idToken.find('.', dot1 + 1);
    if (dot1 == std::string::npos || dot2 == std::string::npos ||
        idToken.find('.', dot2 + 1) != std::string::npos || dot1 == 0 ||
        dot2 == dot1 + 1 || dot2 + 1 >= idToken.size()) {
        return fail(VerifyStatus::MALFORMED, "not a compact JWT");
    }
    std::string headerText = idToken.substr(0, dot1);
    std::string payloadText = idToken.substr(dot1 + 1, dot2 - dot1 - 1);
    std::string sigText = idToken.substr(dot2 + 1);
    std::string headerBytes, payloadBytes, sigBytes;
    if (!base64UrlDecode(headerText, headerBytes) ||
        !base64UrlDecode(payloadText, payloadBytes) ||
        !base64UrlDecode(sigText, sigBytes)) {
        return fail(VerifyStatus::MALFORMED, "invalid base64url segment");
    }

    Json header = Json::parse(headerBytes, nullptr, false);
    if (header.is_discarded() || !header.is_object()) {
        return fail(VerifyStatus::MALFORMED, "invalid JOSE header");
    }
    if (header.find("crit") != header.end()) {
        return fail(VerifyStatus::MALFORMED, "critical header unsupported");
    }
    std::string alg;
    if (!jsonString(header, "alg", alg)) {
        return fail(VerifyStatus::MALFORMED, "missing alg");
    }
    if ((alg != "RS256" && alg != "ES256") ||
        !containsString(desc.algorithms, alg)) {
        return fail(VerifyStatus::UNSUPPORTED_ALG, "algorithm not allowed");
    }
    std::string kid;
    bool hasKid = false;
    Json::const_iterator kidIt = header.find("kid");
    if (kidIt != header.end()) {
        if (!kidIt->is_string()) {
            return fail(VerifyStatus::MALFORMED, "invalid kid");
        }
        kid = kidIt->get<std::string>();
        hasKid = true;
    }

    std::vector<KeyEntry> candidates;
    VerifyStatus ks = acquireKeys(index, alg, kid, hasKid, now, candidates);
    if (ks != VerifyStatus::OK) {
        return fail(ks, ks == VerifyStatus::UNKNOWN_KEY
                            ? "no matching signing key"
                            : "signing keys unavailable");
    }

    std::string signingInput = idToken.substr(0, dot2);
    bool signatureOk = false;
    for (size_t i = 0; i < candidates.size() && !signatureOk; ++i) {
        signatureOk =
            verifySignature(candidates[i].key.get(), alg, signingInput, sigBytes);
    }
    if (!signatureOk) {
        return fail(VerifyStatus::BAD_SIGNATURE, "signature mismatch");
    }

    Json payload = Json::parse(payloadBytes, nullptr, false);
    if (payload.is_discarded() || !payload.is_object()) {
        return fail(VerifyStatus::MALFORMED, "invalid payload");
    }

    std::string iss;
    if (!jsonString(payload, "iss", iss)) {
        return fail(VerifyStatus::BAD_ISSUER, "missing issuer");
    }
    if (desc.tenantIssuer) {
        std::string tenant;
        if (!microsoftTenantFromIssuer(iss, tenant)) {
            return fail(VerifyStatus::BAD_ISSUER, "issuer mismatch");
        }
        std::string tid;
        if (jsonString(payload, "tid", tid) && tid != tenant) {
            return fail(VerifyStatus::BAD_ISSUER, "tenant mismatch");
        }
    } else if (!containsString(desc.issuers, iss)) {
        return fail(VerifyStatus::BAD_ISSUER, "issuer mismatch");
    }

    if (!audienceMatches(payload, clientIds)) {
        return fail(VerifyStatus::BAD_AUDIENCE, "audience mismatch");
    }

    int64_t exp = 0;
    NumState es = jsonNumericDate(payload, "exp", exp);
    if (es != NUM_OK) {
        return fail(VerifyStatus::MALFORMED, "missing or invalid exp");
    }
    if (now >= exp + skew) {
        return fail(VerifyStatus::EXPIRED, "token expired");
    }
    int64_t nbf = 0;
    NumState ns = jsonNumericDate(payload, "nbf", nbf);
    if (ns == NUM_INVALID) {
        return fail(VerifyStatus::MALFORMED, "invalid nbf");
    }
    if (ns == NUM_OK && now + skew < nbf) {
        return fail(VerifyStatus::NOT_YET_VALID, "token not yet valid");
    }

    std::string sub;
    if (!jsonString(payload, "sub", sub) || sub.empty() ||
        sub.size() > kMaxSubBytes) {
        return fail(VerifyStatus::MISSING_SUB, "missing or invalid sub");
    }

    VerifyResult result;
    result.status = VerifyStatus::OK;
    result.claims.provider = provider;
    result.claims.sub = sub;
    result.claims.issuer = iss;
    result.claims.exp = exp;
    jsonString(payload, "email", result.claims.email);
    jsonString(payload, "name", result.claims.name);
    jsonString(payload, "picture", result.claims.picture);
    return result;
}

VerifyResult
OidcVerifier::verifyFacebookAccessToken(const std::string &accessToken) const
{
    return verifyFacebookAccessToken(accessToken, nowSec());
}

VerifyResult
OidcVerifier::verifyFacebookAccessToken(const std::string &accessToken,
                                        int64_t now) const
{
    std::vector<std::string> clientIds;
    std::string appSecret;
    HttpFetcher fetcher;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        int idx = findProvider("facebook");
        if (idx < 0) {
            return fail(VerifyStatus::UNSUPPORTED_PROVIDER,
                        "unsupported provider");
        }
        clientIds = _providers[static_cast<size_t>(idx)].clientIds;
        appSecret = _facebookAppSecret;
        fetcher = _fetcher;
    }
    if (clientIds.empty() || appSecret.empty()) {
        return fail(VerifyStatus::PROVIDER_DISABLED,
                    "facebook graph verification not configured");
    }
    if (accessToken.empty() || accessToken.size() > kMaxAccessTokenBytes) {
        return fail(VerifyStatus::MALFORMED, "access token size out of range");
    }
    if (!fetcher) {
        return fail(VerifyStatus::REMOTE_FAILURE, "no http fetcher configured");
    }

    /* Step 1: debug_token proves the token was issued to our app. */
    std::string debugUrl =
        "https://graph.facebook.com/debug_token?input_token=" +
        urlEncode(accessToken) + "&access_token=" +
        urlEncode(clientIds[0] + "|" + appSecret);
    HttpResponse debugResp;
    if (!fetcher(debugUrl, debugResp) || debugResp.status != 200 ||
        debugResp.body.size() > kMaxHttpBodyBytes) {
        return fail(VerifyStatus::REMOTE_FAILURE, "debug_token request failed");
    }
    Json debugDoc = Json::parse(debugResp.body, nullptr, false);
    if (debugDoc.is_discarded() || !debugDoc.is_object()) {
        return fail(VerifyStatus::REMOTE_FAILURE, "debug_token bad response");
    }
    Json::const_iterator dataIt = debugDoc.find("data");
    if (dataIt == debugDoc.end() || !dataIt->is_object()) {
        return fail(VerifyStatus::TOKEN_REJECTED, "debug_token has no data");
    }
    Json::const_iterator validIt = dataIt->find("is_valid");
    if (validIt == dataIt->end() || !validIt->is_boolean() ||
        !validIt->get<bool>()) {
        return fail(VerifyStatus::TOKEN_REJECTED, "token is not valid");
    }
    std::string appId;
    if (!jsonString(*dataIt, "app_id", appId) ||
        !containsString(clientIds, appId)) {
        return fail(VerifyStatus::BAD_AUDIENCE, "token issued to another app");
    }
    std::string userId;
    if (!jsonString(*dataIt, "user_id", userId) || userId.empty() ||
        userId.size() > kMaxSubBytes) {
        return fail(VerifyStatus::MISSING_SUB, "debug_token has no user_id");
    }
    int64_t expiresAt = 0;
    NumState es = jsonNumericDate(*dataIt, "expires_at", expiresAt);
    if (es == NUM_INVALID) {
        return fail(VerifyStatus::MALFORMED, "invalid expires_at");
    }
    if (es == NUM_OK && expiresAt > 0 && now >= expiresAt) {
        return fail(VerifyStatus::EXPIRED, "token expired");
    }

    /* Step 2: /me for the profile attributes. */
    std::string meUrl =
        "https://graph.facebook.com/me?fields=id,name,email,picture&"
        "access_token=" +
        urlEncode(accessToken);
    HttpResponse meResp;
    if (!fetcher(meUrl, meResp) || meResp.status != 200 ||
        meResp.body.size() > kMaxHttpBodyBytes) {
        return fail(VerifyStatus::REMOTE_FAILURE, "graph /me request failed");
    }
    Json me = Json::parse(meResp.body, nullptr, false);
    if (me.is_discarded() || !me.is_object()) {
        return fail(VerifyStatus::REMOTE_FAILURE, "graph /me bad response");
    }
    std::string meId;
    if (!jsonString(me, "id", meId) || meId.empty() || meId != userId) {
        return fail(VerifyStatus::MISSING_SUB, "graph id mismatch");
    }

    VerifyResult result;
    result.status = VerifyStatus::OK;
    result.claims.provider = "facebook";
    result.claims.sub = meId;
    result.claims.issuer = "https://graph.facebook.com";
    result.claims.exp = (es == NUM_OK) ? expiresAt : 0;
    jsonString(me, "name", result.claims.name);
    jsonString(me, "email", result.claims.email);
    Json::const_iterator picIt = me.find("picture");
    if (picIt != me.end() && picIt->is_object()) {
        Json::const_iterator picData = picIt->find("data");
        if (picData != picIt->end() && picData->is_object()) {
            jsonString(*picData, "url", result.claims.picture);
        }
    }
    return result;
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
