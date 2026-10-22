/*
 * OidcVerifier.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef OCTI_SERVER_OIDC_VERIFIER_HXX
#define OCTI_SERVER_OIDC_VERIFIER_HXX

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

struct evp_pkey_st;

namespace octi {

/*
 * Base64URL (RFC 4648 section 5) without padding, as used by JWS/JWT.
 * base64UrlDecode() is strict and canonical: only the URL-safe alphabet is
 * accepted, '=' padding is rejected, a length of 4n+1 is rejected and the
 * unused trailing bits must be zero, so decode(encode(x)) == x and every
 * accepted text has exactly one byte representation.
 */
std::string base64UrlEncode(const void *data, size_t len);
std::string base64UrlEncode(const std::string &bytes);
bool base64UrlDecode(const std::string &text, std::string &out);

/*
 * Remote HTTP boundary (JWKS download, Facebook Graph introspection).
 * Returns false on transport failure. defaultHttpFetcher() performs an
 * HTTPS GET with certificate verification through cpp-httplib.
 */
struct HttpResponse {
    int status = 0;
    std::string body;
};

typedef std::function<bool(const std::string &url, HttpResponse &response)>
    HttpFetcher;

HttpFetcher defaultHttpFetcher();

/* Verified identity assertion handed to AccountStore. */
struct VerifiedClaims {
    std::string provider;   // 'google' | 'apple' | 'microsoft' | 'facebook' | 'line'
    std::string sub;        // immutable provider subject (account key)
    std::string email;      // informational only, never an account key
    std::string name;
    std::string picture;
    std::string issuer;
    int64_t exp = 0;
};

enum class VerifyStatus {
    OK,
    MALFORMED,
    UNSUPPORTED_PROVIDER,
    PROVIDER_DISABLED,
    UNSUPPORTED_ALG,
    KEY_SET_UNAVAILABLE,
    UNKNOWN_KEY,
    BAD_SIGNATURE,
    BAD_ISSUER,
    BAD_AUDIENCE,
    EXPIRED,
    NOT_YET_VALID,
    MISSING_SUB,
    TOKEN_REJECTED,
    REMOTE_FAILURE
};

const char *verifyStatusName(VerifyStatus status);

struct VerifyResult {
    VerifyStatus status = VerifyStatus::MALFORMED;
    std::string detail;
    VerifiedClaims claims;

    bool ok() const { return status == VerifyStatus::OK; }
};

/* Static description of one supported SSO provider (plan 2.3.2). */
struct ProviderDescriptor {
    std::string name;
    std::string displayName;
    std::vector<std::string> issuers;   // exact issuer strings
    bool tenantIssuer = false;          // https://login.microsoftonline.com/<tenant>/v2.0
    std::string jwksUrl;
    std::vector<std::string> algorithms;
};

class OidcVerifier {
public:
    OidcVerifier();
    ~OidcVerifier();

    OidcVerifier(const OidcVerifier &) = delete;
    OidcVerifier &operator=(const OidcVerifier &) = delete;

    /* google, apple, microsoft, facebook, line (canonical order). */
    static const std::vector<ProviderDescriptor> &providerRegistry();
    static bool isSupportedProvider(const std::string &name);

    /*
     * Configuration. A provider is enabled iff at least one client id
     * (accepted 'aud') is configured. Every setter is thread-safe and
     * returns false for an unsupported provider name.
     */
    bool setClientId(const std::string &provider, const std::string &clientId);
    bool setClientIds(const std::string &provider,
                      const std::vector<std::string> &clientIds);
    /* Only "RS256" and "ES256" may be allowed; returns false otherwise. */
    bool setAllowedAlgorithms(const std::string &provider,
                              const std::vector<std::string> &algorithms);
    void setClockSkewSec(int64_t seconds);
    void setKeyTtlSec(int64_t seconds);
    void setClock(std::function<int64_t()> clock);
    /* No fetcher (the default) means the verifier never touches the network. */
    void setHttpFetcher(HttpFetcher fetcher);
    void setFacebookAppSecret(const std::string &secret);

    std::vector<std::string> enabledProviders() const;
    bool isProviderEnabled(const std::string &provider) const;

    /*
     * Inject a JWKS document ({"keys":[...]}) for a provider. Unusable JWKs
     * (wrong key type, short RSA modulus, off-curve EC point) are skipped.
     * Returns false, leaving any existing key set untouched, when no usable
     * key remains. The key set is considered fresh for the key TTL.
     */
    bool setKeySet(const std::string &provider, const std::string &jwksJson);
    bool setKeySet(const std::string &provider, const std::string &jwksJson,
                   int64_t now);
    /* Download the provider JWKS through the HttpFetcher. */
    bool refreshKeySet(const std::string &provider);
    bool refreshKeySet(const std::string &provider, int64_t now);

    /* Verify an OIDC id_token (compact JWT). Never throws. */
    VerifyResult verifyIdToken(const std::string &provider,
                               const std::string &idToken) const;
    VerifyResult verifyIdToken(const std::string &provider,
                               const std::string &idToken, int64_t now) const;

    /*
     * Facebook classic Graph access_token: debug_token (is_valid, app_id,
     * user_id) followed by /me?fields=id,name,email,picture through the
     * HttpFetcher. Requires a facebook client id and the app secret.
     */
    VerifyResult verifyFacebookAccessToken(const std::string &accessToken) const;
    VerifyResult verifyFacebookAccessToken(const std::string &accessToken,
                                           int64_t now) const;

private:
    struct KeyEntry {
        std::string kid;
        std::string kty;
        std::string alg;
        std::shared_ptr<evp_pkey_st> key;
    };

    struct ProviderState {
        ProviderDescriptor descriptor;
        std::vector<std::string> clientIds;
        std::vector<KeyEntry> keys;
        bool keysLoaded = false;
        int64_t keysFetchedAt = 0;
        bool refreshAttempted = false;
        int64_t lastRefreshAttempt = 0;
    };

    int64_t nowSec() const;
    int findProvider(const std::string &name) const;
    bool installKeySet(size_t index, const std::string &jwksJson,
                       int64_t now) const;
    bool refreshKeySetImpl(const std::string &provider, int64_t now) const;
    VerifyStatus acquireKeys(size_t index, const std::string &alg,
                             const std::string &kid, bool hasKid, int64_t now,
                             std::vector<KeyEntry> &candidates) const;

    mutable std::mutex _mutex;
    mutable std::vector<ProviderState> _providers;
    std::function<int64_t()> _clock;
    HttpFetcher _fetcher;
    std::string _facebookAppSecret;
    int64_t _clockSkewSec;
    int64_t _keyTtlSec;
};

} // namespace octi

#endif // OCTI_SERVER_OIDC_VERIFIER_HXX

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
