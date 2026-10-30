/*
 * TestAccountAndOidc.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "AccountStore.hxx"
#include "OctiBoard.hxx"
#include "OctiGame.hxx"
#include "OidcVerifier.hxx"

#include <nlohmann/json.hpp>
#include <openssl/bn.h>
#include <openssl/core_names.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <rapidcheck.h>
#include <sqlite3.h>

#include <sys/stat.h>
#include <unistd.h>

#include <atomic>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestHarness.h>

using octi::AccountStore;
using octi::ArchiveStatus;
using octi::GameOutcome;
using octi::GameRecord;
using octi::LinkStatus;
using octi::LoginResult;
using octi::OidcVerifier;
using octi::PlayerProfile;
using octi::UnlinkStatus;
using octi::UpdateStatus;
using octi::VerifiedClaims;
using octi::VerifyResult;
using octi::VerifyStatus;

typedef nlohmann::json Json;

namespace {

const int64_t kNow = 1800000000;

/* ------------------------------------------------------------------ */
/* OpenSSL helpers: real keys, real signatures                         */
/* ------------------------------------------------------------------ */

struct PkeyDeleter {
    void operator()(EVP_PKEY *p) const { EVP_PKEY_free(p); }
};
typedef std::unique_ptr<EVP_PKEY, PkeyDeleter> Pkey;

struct BnDeleter {
    void operator()(BIGNUM *p) const { BN_free(p); }
};
typedef std::unique_ptr<BIGNUM, BnDeleter> Bn;

Pkey generateRsa(int bits)
{
    return Pkey(EVP_PKEY_Q_keygen(nullptr, nullptr, "RSA",
                                  static_cast<size_t>(bits)));
}

Pkey generateEc()
{
    return Pkey(EVP_PKEY_Q_keygen(nullptr, nullptr, "EC", "P-256"));
}

std::string bnParam(EVP_PKEY *key, const char *name, int padTo)
{
    BIGNUM *raw = nullptr;
    if (EVP_PKEY_get_bn_param(key, name, &raw) != 1) {
        return std::string();
    }
    Bn bn(raw);
    int len = padTo > 0 ? padTo : BN_num_bytes(bn.get());
    std::string out(static_cast<size_t>(len), '\0');
    if (padTo > 0) {
        BN_bn2binpad(bn.get(), reinterpret_cast<unsigned char *>(&out[0]), len);
    } else {
        BN_bn2bin(bn.get(), reinterpret_cast<unsigned char *>(&out[0]));
    }
    return out;
}

Json rsaJwk(EVP_PKEY *key, const std::string &kid)
{
    Json j;
    j["kty"] = "RSA";
    j["use"] = "sig";
    j["kid"] = kid;
    j["alg"] = "RS256";
    j["n"] = octi::base64UrlEncode(bnParam(key, OSSL_PKEY_PARAM_RSA_N, 0));
    j["e"] = octi::base64UrlEncode(bnParam(key, OSSL_PKEY_PARAM_RSA_E, 0));
    return j;
}

Json ecJwk(EVP_PKEY *key, const std::string &kid)
{
    Json j;
    j["kty"] = "EC";
    j["use"] = "sig";
    j["kid"] = kid;
    j["crv"] = "P-256";
    j["x"] = octi::base64UrlEncode(bnParam(key, OSSL_PKEY_PARAM_EC_PUB_X, 32));
    j["y"] = octi::base64UrlEncode(bnParam(key, OSSL_PKEY_PARAM_EC_PUB_Y, 32));
    return j;
}

std::string digestSign(EVP_PKEY *key, const std::string &input)
{
    std::unique_ptr<EVP_MD_CTX, void (*)(EVP_MD_CTX *)> md(EVP_MD_CTX_new(),
                                                            EVP_MD_CTX_free);
    size_t len = 0;
    if (!md || EVP_DigestSignInit(md.get(), nullptr, EVP_sha256(), nullptr,
                                  key) != 1 ||
        EVP_DigestSign(md.get(), nullptr, &len,
                       reinterpret_cast<const unsigned char *>(input.data()),
                       input.size()) != 1) {
        return std::string();
    }
    std::string sig(len, '\0');
    if (EVP_DigestSign(md.get(), reinterpret_cast<unsigned char *>(&sig[0]),
                       &len,
                       reinterpret_cast<const unsigned char *>(input.data()),
                       input.size()) != 1) {
        return std::string();
    }
    sig.resize(len);
    return sig;
}

std::string signRs256(EVP_PKEY *key, const std::string &input)
{
    return digestSign(key, input);
}

/* ES256 JWS signature is raw r || s (64 bytes), not ASN.1 DER. */
std::string signEs256(EVP_PKEY *key, const std::string &input)
{
    std::string der = digestSign(key, input);
    const unsigned char *p = reinterpret_cast<const unsigned char *>(der.data());
    ECDSA_SIG *sig = d2i_ECDSA_SIG(nullptr, &p, static_cast<long>(der.size()));
    if (!sig) {
        return std::string();
    }
    const BIGNUM *r = nullptr;
    const BIGNUM *s = nullptr;
    ECDSA_SIG_get0(sig, &r, &s);
    std::string raw(64, '\0');
    BN_bn2binpad(r, reinterpret_cast<unsigned char *>(&raw[0]), 32);
    BN_bn2binpad(s, reinterpret_cast<unsigned char *>(&raw[32]), 32);
    ECDSA_SIG_free(sig);
    return raw;
}

std::string hmacSha256(const std::string &key, const std::string &data)
{
    unsigned char md[EVP_MAX_MD_SIZE];
    unsigned int len = 0;
    HMAC(EVP_sha256(), key.data(), static_cast<int>(key.size()),
         reinterpret_cast<const unsigned char *>(data.data()), data.size(), md,
         &len);
    return std::string(reinterpret_cast<char *>(md), len);
}

struct TestKeys {
    Pkey rsa;
    Pkey rsaAttacker;
    Pkey rsaRotated;
    Pkey ec;
    Pkey ecAttacker;
    TestKeys()
        : rsa(generateRsa(2048)), rsaAttacker(generateRsa(2048)),
          rsaRotated(generateRsa(2048)), ec(generateEc()),
          ecAttacker(generateEc())
    {
    }
};

TestKeys &keys()
{
    static TestKeys k;
    return k;
}

std::string jwksDocument(const std::vector<Json> &entries)
{
    Json doc;
    doc["keys"] = Json::array();
    for (size_t i = 0; i < entries.size(); ++i) {
        doc["keys"].push_back(entries[i]);
    }
    return doc.dump();
}

std::string defaultJwks()
{
    std::vector<Json> e;
    e.push_back(rsaJwk(keys().rsa.get(), "rsa-1"));
    e.push_back(ecJwk(keys().ec.get(), "ec-1"));
    return jwksDocument(e);
}

std::string jwtSign(const Json &header, const Json &payload, EVP_PKEY *signer)
{
    std::string h = octi::base64UrlEncode(header.dump());
    std::string p = octi::base64UrlEncode(payload.dump());
    std::string input = h + "." + p;
    std::string alg = header.value("alg", "");
    std::string sig = (alg == "ES256") ? signEs256(signer, input)
                                       : signRs256(signer, input);
    return input + "." + octi::base64UrlEncode(sig);
}

Json rsHeader(const std::string &kid = "rsa-1")
{
    Json h;
    h["alg"] = "RS256";
    h["typ"] = "JWT";
    h["kid"] = kid;
    return h;
}

Json esHeader(const std::string &kid = "ec-1")
{
    Json h;
    h["alg"] = "ES256";
    h["typ"] = "JWT";
    h["kid"] = kid;
    return h;
}

std::string issuerFor(const std::string &provider)
{
    if (provider == "google") {
        return "https://accounts.google.com";
    }
    if (provider == "apple") {
        return "https://appleid.apple.com";
    }
    if (provider == "microsoft") {
        return "https://login.microsoftonline.com/"
               "9188040d-6c67-4c5b-b112-36a304b66dad/v2.0";
    }
    if (provider == "facebook") {
        return "https://www.facebook.com";
    }
    return "https://access.line.me";
}

Json claimsFor(const std::string &provider, const std::string &sub = "")
{
    Json c;
    c["iss"] = issuerFor(provider);
    c["aud"] = "client-" + provider;
    c["sub"] = sub.empty() ? ("sub-" + provider + "-1") : sub;
    c["iat"] = kNow - 10;
    c["exp"] = kNow + 3600;
    c["email"] = "user@example.com";
    c["name"] = "Test User";
    c["picture"] = "https://example.com/avatar.png";
    return c;
}

std::vector<std::string> allProviders()
{
    std::vector<std::string> out;
    const std::vector<octi::ProviderDescriptor> &reg =
        OidcVerifier::providerRegistry();
    for (size_t i = 0; i < reg.size(); ++i) {
        out.push_back(reg[i].name);
    }
    return out;
}

std::vector<std::string> splitToken(const std::string &token)
{
    std::vector<std::string> parts;
    size_t start = 0;
    for (;;) {
        size_t dot = token.find('.', start);
        if (dot == std::string::npos) {
            parts.push_back(token.substr(start));
            break;
        }
        parts.push_back(token.substr(start, dot - start));
        start = dot + 1;
    }
    return parts;
}

std::string flipSignatureByte(const std::string &token)
{
    std::vector<std::string> parts = splitToken(token);
    std::string sig;
    octi::base64UrlDecode(parts[2], sig);
    sig[sig.size() / 2] = static_cast<char>(sig[sig.size() / 2] ^ 0x01);
    return parts[0] + "." + parts[1] + "." + octi::base64UrlEncode(sig);
}

#define CHECK_STATUS(expected, result)                                         \
    STRCMP_EQUAL(octi::verifyStatusName(VerifyStatus::expected),               \
                 octi::verifyStatusName((result).status))

const char *linkName(LinkStatus s)
{
    switch (s) {
    case LinkStatus::LINKED:
        return "LINKED";
    case LinkStatus::ALREADY_LINKED_SELF:
        return "ALREADY_LINKED_SELF";
    case LinkStatus::CONFLICT_ALREADY_LINKED:
        return "CONFLICT_ALREADY_LINKED";
    case LinkStatus::PLAYER_NOT_FOUND:
        return "PLAYER_NOT_FOUND";
    case LinkStatus::INVALID_CLAIMS:
        return "INVALID_CLAIMS";
    case LinkStatus::ERROR:
        return "ERROR";
    }
    return "?";
}

const char *unlinkName(UnlinkStatus s)
{
    switch (s) {
    case UnlinkStatus::UNLINKED:
        return "UNLINKED";
    case UnlinkStatus::LAST_IDENTITY_FORBIDDEN:
        return "LAST_IDENTITY_FORBIDDEN";
    case UnlinkStatus::NOT_LINKED:
        return "NOT_LINKED";
    case UnlinkStatus::ERROR:
        return "ERROR";
    }
    return "?";
}

const char *updateName(UpdateStatus s)
{
    switch (s) {
    case UpdateStatus::OK:
        return "OK";
    case UpdateStatus::NOT_FOUND:
        return "NOT_FOUND";
    case UpdateStatus::INVALID_HANDLE:
        return "INVALID_HANDLE";
    case UpdateStatus::HANDLE_TAKEN:
        return "HANDLE_TAKEN";
    case UpdateStatus::INVALID_DISPLAY_NAME:
        return "INVALID_DISPLAY_NAME";
    case UpdateStatus::INVALID_AVATAR_URL:
        return "INVALID_AVATAR_URL";
    case UpdateStatus::ERROR:
        return "ERROR";
    }
    return "?";
}

const char *archiveName(ArchiveStatus s)
{
    switch (s) {
    case ArchiveStatus::OK:
        return "OK";
    case ArchiveStatus::DUPLICATE_ID:
        return "DUPLICATE_ID";
    case ArchiveStatus::INVALID:
        return "INVALID";
    case ArchiveStatus::ERROR:
        return "ERROR";
    }
    return "?";
}

/* mkdtemp scratch directory removed on destruction. */
class TempDir {
public:
    TempDir()
    {
        const char *base = std::getenv("TMPDIR");
        std::string tmpl = std::string(base && *base ? base : "/tmp") +
                           "/octi_server_test_XXXXXX";
        std::vector<char> buf(tmpl.begin(), tmpl.end());
        buf.push_back('\0');
        if (mkdtemp(buf.data()) != nullptr) {
            _path = buf.data();
        }
    }
    ~TempDir()
    {
        if (!_path.empty()) {
            std::error_code ec;
            std::filesystem::remove_all(_path, ec);
        }
    }
    const std::string &path() const { return _path; }

private:
    std::string _path;
};

mode_t modeOf(const std::string &path)
{
    struct stat st;
    if (::stat(path.c_str(), &st) != 0) {
        return 0xFFFF;
    }
    return st.st_mode & 0777;
}

std::string readFile(const std::string &path)
{
    std::ifstream in(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)),
                       std::istreambuf_iterator<char>());
}

VerifiedClaims makeClaims(const std::string &provider, const std::string &sub,
                          const std::string &name = "Test User",
                          const std::string &email = "user@example.com")
{
    VerifiedClaims c;
    c.provider = provider;
    c.sub = sub;
    c.name = name;
    c.email = email;
    c.picture = "https://example.com/avatar.png";
    c.issuer = issuerFor(provider);
    c.exp = kNow + 3600;
    return c;
}

} // namespace

/* ================================================================== */
/* OidcVerifierTest                                                    */
/* ================================================================== */

TEST_GROUP(OidcVerifierTest)
{
    OidcVerifier *verifier;

    void setup()
    {
        verifier = new OidcVerifier();
        verifier->setClock([]() -> int64_t { return kNow; });
        std::vector<std::string> providers = allProviders();
        for (size_t i = 0; i < providers.size(); ++i) {
            verifier->setClientId(providers[i], "client-" + providers[i]);
            verifier->setKeySet(providers[i], defaultJwks(), kNow);
        }
    }

    void teardown()
    {
        delete verifier;
        verifier = nullptr;
    }

    std::string token(const std::string &provider, const std::string &sub = "")
    {
        return jwtSign(rsHeader(), claimsFor(provider, sub), keys().rsa.get());
    }
};

/* ---- Base64URL ---- */

TEST(OidcVerifierTest, Base64UrlRfc4648Vectors)
{
    const char *plain[] = {"", "f", "fo", "foo", "foob", "fooba", "foobar"};
    const char *enc[] = {"", "Zg", "Zm8", "Zm9v", "Zm9vYg", "Zm9vYmE", "Zm9vYmFy"};
    for (int i = 0; i < 7; ++i) {
        STRCMP_EQUAL(enc[i], octi::base64UrlEncode(std::string(plain[i])).c_str());
        std::string back;
        CHECK_TRUE(octi::base64UrlDecode(enc[i], back));
        STRCMP_EQUAL(plain[i], back.c_str());
    }
    std::string urlSafe("\xfb\xff", 2);
    STRCMP_EQUAL("-_8", octi::base64UrlEncode(urlSafe).c_str());
    std::string back;
    CHECK_TRUE(octi::base64UrlDecode("-_8", back));
    CHECK_TRUE(back == urlSafe);
}

TEST(OidcVerifierTest, Base64UrlDecodeRejectsNonCanonicalInput)
{
    std::string out;
    CHECK_FALSE(octi::base64UrlDecode("Zg==", out));   // padding
    CHECK_FALSE(octi::base64UrlDecode("Z", out));      // length 4n+1
    CHECK_FALSE(octi::base64UrlDecode("Zh", out));     // non-zero trailing bits
    CHECK_FALSE(octi::base64UrlDecode("Zm9", out));    // non-zero trailing bits
    CHECK_FALSE(octi::base64UrlDecode("Zm+v", out));   // standard alphabet
    CHECK_FALSE(octi::base64UrlDecode("Zm/v", out));
    CHECK_FALSE(octi::base64UrlDecode("Zm 9v", out));  // whitespace
    CHECK_FALSE(octi::base64UrlDecode(std::string("Zm9v\0", 5), out));
    CHECK_TRUE(out.empty());
}

/* ---- Nominal ---- */

TEST(OidcVerifierTest, NominalRs256AllFiveProviders)
{
    std::vector<std::string> providers = allProviders();
    CHECK_EQUAL(5, static_cast<int>(providers.size()));
    for (size_t i = 0; i < providers.size(); ++i) {
        VerifyResult r = verifier->verifyIdToken(providers[i], token(providers[i]));
        CHECK_STATUS(OK, r);
        STRCMP_EQUAL(providers[i].c_str(), r.claims.provider.c_str());
        STRCMP_EQUAL(("sub-" + providers[i] + "-1").c_str(), r.claims.sub.c_str());
        STRCMP_EQUAL("user@example.com", r.claims.email.c_str());
        STRCMP_EQUAL("Test User", r.claims.name.c_str());
        STRCMP_EQUAL("https://example.com/avatar.png", r.claims.picture.c_str());
        STRCMP_EQUAL(issuerFor(providers[i]).c_str(), r.claims.issuer.c_str());
        CHECK_EQUAL(kNow + 3600, r.claims.exp);
    }
}

TEST(OidcVerifierTest, NominalLineEs256)
{
    std::string t = jwtSign(esHeader(), claimsFor("line"), keys().ec.get());
    VerifyResult r = verifier->verifyIdToken("line", t);
    CHECK_STATUS(OK, r);
    STRCMP_EQUAL("sub-line-1", r.claims.sub.c_str());
}

TEST(OidcVerifierTest, Es256RejectedForGoogleUnlessExplicitlyAllowed)
{
    std::string t = jwtSign(esHeader(), claimsFor("google"), keys().ec.get());
    CHECK_STATUS(UNSUPPORTED_ALG, verifier->verifyIdToken("google", t));

    std::vector<std::string> both;
    both.push_back("RS256");
    both.push_back("ES256");
    CHECK_TRUE(verifier->setAllowedAlgorithms("google", both));
    CHECK_STATUS(OK, verifier->verifyIdToken("google", t));

    std::vector<std::string> hs;
    hs.push_back("HS256");
    CHECK_FALSE(verifier->setAllowedAlgorithms("google", hs));
    CHECK_FALSE(verifier->setAllowedAlgorithms("google", std::vector<std::string>()));
    CHECK_FALSE(verifier->setAllowedAlgorithms("github", both));
}

TEST(OidcVerifierTest, GoogleAcceptsBothIssuerForms)
{
    Json c = claimsFor("google");
    c["iss"] = "accounts.google.com";
    std::string t = jwtSign(rsHeader(), c, keys().rsa.get());
    CHECK_STATUS(OK, verifier->verifyIdToken("google", t));
}

TEST(OidcVerifierTest, MicrosoftTenantIssuerMatching)
{
    const char *good[] = {
        "https://login.microsoftonline.com/9188040d-6c67-4c5b-b112-36a304b66dad/v2.0",
        "https://login.microsoftonline.com/contoso.onmicrosoft.com/v2.0"};
    for (int i = 0; i < 2; ++i) {
        Json c = claimsFor("microsoft");
        c["iss"] = good[i];
        CHECK_STATUS(OK, verifier->verifyIdToken(
                             "microsoft", jwtSign(rsHeader(), c, keys().rsa.get())));
    }
    const char *bad[] = {
        "https://login.microsoftonline.com//v2.0",
        "https://login.microsoftonline.com/v2.0",
        "https://login.microsoftonline.com/tenant/v1.0",
        "https://login.microsoftonline.com/te/nant/v2.0",
        "https://login.microsoftonline.com.evil.example/tenant/v2.0",
        "http://login.microsoftonline.com/tenant/v2.0",
        "https://sts.windows.net/tenant/",
        ""};
    for (int i = 0; i < 8; ++i) {
        Json c = claimsFor("microsoft");
        c["iss"] = bad[i];
        CHECK_STATUS(BAD_ISSUER, verifier->verifyIdToken(
                                     "microsoft",
                                     jwtSign(rsHeader(), c, keys().rsa.get())));
    }
    Json c = claimsFor("microsoft");
    c["tid"] = "11111111-1111-1111-1111-111111111111";
    CHECK_STATUS(BAD_ISSUER, verifier->verifyIdToken(
                                 "microsoft",
                                 jwtSign(rsHeader(), c, keys().rsa.get())));
    c["tid"] = "9188040d-6c67-4c5b-b112-36a304b66dad";
    CHECK_STATUS(OK, verifier->verifyIdToken(
                         "microsoft", jwtSign(rsHeader(), c, keys().rsa.get())));
}

TEST(OidcVerifierTest, AudienceMayBeArrayWithAuthorizedParty)
{
    Json c = claimsFor("google");
    c["aud"] = Json::array({"client-google"});
    CHECK_STATUS(OK, verifier->verifyIdToken(
                         "google", jwtSign(rsHeader(), c, keys().rsa.get())));

    c["aud"] = Json::array({"client-google", "other"});
    CHECK_STATUS(BAD_AUDIENCE, verifier->verifyIdToken(
                                   "google",
                                   jwtSign(rsHeader(), c, keys().rsa.get())));
    c["azp"] = "client-google";
    CHECK_STATUS(OK, verifier->verifyIdToken(
                         "google", jwtSign(rsHeader(), c, keys().rsa.get())));
    c["azp"] = "other";
    CHECK_STATUS(BAD_AUDIENCE, verifier->verifyIdToken(
                                   "google",
                                   jwtSign(rsHeader(), c, keys().rsa.get())));
}

TEST(OidcVerifierTest, MultipleClientIdsAndProviderDiscovery)
{
    OidcVerifier fresh;
    fresh.setClock([]() -> int64_t { return kNow; });
    CHECK_TRUE(fresh.enabledProviders().empty());
    CHECK_FALSE(fresh.isProviderEnabled("google"));
    CHECK_STATUS(PROVIDER_DISABLED,
                 fresh.verifyIdToken("google", token("google")));

    CHECK_TRUE(fresh.setClientId("line", "client-line"));
    std::vector<std::string> ids;
    ids.push_back("android-google");
    ids.push_back("client-google");
    ids.push_back("");
    CHECK_TRUE(fresh.setClientIds("google", ids));
    CHECK_FALSE(fresh.setClientId("github", "x"));

    std::vector<std::string> enabled = fresh.enabledProviders();
    CHECK_EQUAL(2, static_cast<int>(enabled.size()));
    STRCMP_EQUAL("google", enabled[0].c_str());
    STRCMP_EQUAL("line", enabled[1].c_str());
    CHECK_TRUE(fresh.isProviderEnabled("line"));

    CHECK_TRUE(fresh.setKeySet("google", defaultJwks(), kNow));
    CHECK_STATUS(OK, fresh.verifyIdToken("google", token("google")));

    CHECK_TRUE(fresh.setClientId("google", ""));
    CHECK_FALSE(fresh.isProviderEnabled("google"));
}

TEST(OidcVerifierTest, OptionalProfileClaimsMayBeAbsent)
{
    Json c = claimsFor("apple");
    c.erase("email");
    c.erase("name");
    c.erase("picture");
    VerifyResult r = verifier->verifyIdToken(
        "apple", jwtSign(rsHeader(), c, keys().rsa.get()));
    CHECK_STATUS(OK, r);
    CHECK_TRUE(r.claims.email.empty());
    CHECK_TRUE(r.claims.name.empty());
    CHECK_TRUE(r.claims.picture.empty());
}

/* ---- Boundary ---- */

TEST(OidcVerifierTest, BoundaryExpiryWithClockSkew)
{
    Json c = claimsFor("google");
    c["exp"] = kNow;
    std::string t = jwtSign(rsHeader(), c, keys().rsa.get());
    /* default skew 60 s: valid while now < exp + skew */
    CHECK_STATUS(OK, verifier->verifyIdToken("google", t, kNow - 1));
    CHECK_STATUS(OK, verifier->verifyIdToken("google", t, kNow + 59));
    CHECK_STATUS(EXPIRED, verifier->verifyIdToken("google", t, kNow + 60));
    CHECK_STATUS(EXPIRED, verifier->verifyIdToken("google", t, kNow + 61));

    verifier->setClockSkewSec(0);
    CHECK_STATUS(OK, verifier->verifyIdToken("google", t, kNow - 1));
    CHECK_STATUS(EXPIRED, verifier->verifyIdToken("google", t, kNow));
}

TEST(OidcVerifierTest, BoundaryNotBeforeWithClockSkew)
{
    Json c = claimsFor("google");
    c["nbf"] = kNow + 60;
    CHECK_STATUS(OK, verifier->verifyIdToken(
                         "google", jwtSign(rsHeader(), c, keys().rsa.get())));
    c["nbf"] = kNow + 61;
    CHECK_STATUS(NOT_YET_VALID,
                 verifier->verifyIdToken(
                     "google", jwtSign(rsHeader(), c, keys().rsa.get())));
    c["nbf"] = "tomorrow";
    CHECK_STATUS(MALFORMED, verifier->verifyIdToken(
                                "google",
                                jwtSign(rsHeader(), c, keys().rsa.get())));
}

TEST(OidcVerifierTest, BoundaryKeySetTtl)
{
    verifier->setKeyTtlSec(100);
    Json c = claimsFor("google");
    c["exp"] = kNow + 7200;
    std::string t = jwtSign(rsHeader(), c, keys().rsa.get());
    CHECK_STATUS(OK, verifier->verifyIdToken("google", t, kNow + 100));
    CHECK_STATUS(KEY_SET_UNAVAILABLE,
                 verifier->verifyIdToken("google", t, kNow + 101));
    CHECK_TRUE(verifier->setKeySet("google", defaultJwks(), kNow + 101));
    CHECK_STATUS(OK, verifier->verifyIdToken("google", t, kNow + 101));
}

TEST(OidcVerifierTest, BoundarySubLength)
{
    std::string sub255(255, 's');
    CHECK_STATUS(OK, verifier->verifyIdToken("google", token("google", sub255)));
    std::string sub256(256, 's');
    CHECK_STATUS(MISSING_SUB,
                 verifier->verifyIdToken("google", token("google", sub256)));
}

/* ---- Faults ---- */

TEST(OidcVerifierTest, FaultTamperedPayloadKeepsOldSignature)
{
    std::string t = token("google");
    std::vector<std::string> parts = splitToken(t);
    std::string payload;
    CHECK_TRUE(octi::base64UrlDecode(parts[1], payload));
    Json p = Json::parse(payload);
    p["sub"] = "someone-else";
    std::string forged = parts[0] + "." + octi::base64UrlEncode(p.dump()) + "." +
                         parts[2];
    CHECK_STATUS(BAD_SIGNATURE, verifier->verifyIdToken("google", forged));
}

TEST(OidcVerifierTest, FaultTamperedSignature)
{
    std::string t = token("google");
    CHECK_STATUS(OK, verifier->verifyIdToken("google", t));
    CHECK_STATUS(BAD_SIGNATURE,
                 verifier->verifyIdToken("google", flipSignatureByte(t)));

    std::string es = jwtSign(esHeader(), claimsFor("line"), keys().ec.get());
    CHECK_STATUS(BAD_SIGNATURE,
                 verifier->verifyIdToken("line", flipSignatureByte(es)));

    std::vector<std::string> parts = splitToken(es);
    std::string sig;
    CHECK_TRUE(octi::base64UrlDecode(parts[2], sig));
    sig.resize(63);
    CHECK_STATUS(BAD_SIGNATURE,
                 verifier->verifyIdToken(
                     "line", parts[0] + "." + parts[1] + "." +
                                 octi::base64UrlEncode(sig)));
}

TEST(OidcVerifierTest, FaultWrongIssuer)
{
    Json c = claimsFor("apple");
    c["iss"] = "https://appleid.apple.com.evil.example";
    CHECK_STATUS(BAD_ISSUER, verifier->verifyIdToken(
                                 "apple",
                                 jwtSign(rsHeader(), c, keys().rsa.get())));
    /* Google token presented to Apple must not be accepted. */
    CHECK_STATUS(BAD_ISSUER, verifier->verifyIdToken("apple", token("google")));
    c.erase("iss");
    CHECK_STATUS(BAD_ISSUER, verifier->verifyIdToken(
                                 "apple",
                                 jwtSign(rsHeader(), c, keys().rsa.get())));
}

TEST(OidcVerifierTest, FaultWrongAudience)
{
    Json c = claimsFor("google");
    c["aud"] = "someone-elses-client";
    CHECK_STATUS(BAD_AUDIENCE, verifier->verifyIdToken(
                                   "google",
                                   jwtSign(rsHeader(), c, keys().rsa.get())));
    c.erase("aud");
    CHECK_STATUS(BAD_AUDIENCE, verifier->verifyIdToken(
                                   "google",
                                   jwtSign(rsHeader(), c, keys().rsa.get())));
    c["aud"] = 42;
    CHECK_STATUS(BAD_AUDIENCE, verifier->verifyIdToken(
                                   "google",
                                   jwtSign(rsHeader(), c, keys().rsa.get())));
}

TEST(OidcVerifierTest, FaultExpiredToken)
{
    Json c = claimsFor("google");
    c["exp"] = kNow - 3600;
    CHECK_STATUS(EXPIRED, verifier->verifyIdToken(
                              "google",
                              jwtSign(rsHeader(), c, keys().rsa.get())));
    c.erase("exp");
    CHECK_STATUS(MALFORMED, verifier->verifyIdToken(
                                "google",
                                jwtSign(rsHeader(), c, keys().rsa.get())));
    c["exp"] = "never";
    CHECK_STATUS(MALFORMED, verifier->verifyIdToken(
                                "google",
                                jwtSign(rsHeader(), c, keys().rsa.get())));
}

TEST(OidcVerifierTest, FaultAlgNoneAttack)
{
    const char *algs[] = {"none", "None", "NONE", ""};
    for (int i = 0; i < 4; ++i) {
        Json h;
        h["alg"] = algs[i];
        h["kid"] = "rsa-1";
        std::string unsignedTok =
            octi::base64UrlEncode(h.dump()) + "." +
            octi::base64UrlEncode(claimsFor("google").dump()) + ".";
        CHECK_FALSE(verifier->verifyIdToken("google", unsignedTok).ok());
        /* with a garbage signature segment the algorithm itself is refused */
        VerifyResult r = verifier->verifyIdToken(
            "google", unsignedTok + octi::base64UrlEncode("garbage"));
        CHECK_STATUS(UNSUPPORTED_ALG, r);
    }
}

TEST(OidcVerifierTest, FaultHs256KeyConfusion)
{
    /* Classic attack: sign with HMAC using the RSA public key as secret. */
    std::string secret = bnParam(keys().rsa.get(), OSSL_PKEY_PARAM_RSA_N, 0);
    Json h;
    h["alg"] = "HS256";
    h["kid"] = "rsa-1";
    std::string input = octi::base64UrlEncode(h.dump()) + "." +
                        octi::base64UrlEncode(claimsFor("google").dump());
    std::string forged = input + "." + octi::base64UrlEncode(hmacSha256(secret, input));
    CHECK_STATUS(UNSUPPORTED_ALG, verifier->verifyIdToken("google", forged));
    h["alg"] = "HS512";
    input = octi::base64UrlEncode(h.dump()) + "." +
            octi::base64UrlEncode(claimsFor("google").dump());
    CHECK_STATUS(UNSUPPORTED_ALG,
                 verifier->verifyIdToken(
                     "google", input + "." + octi::base64UrlEncode(secret)));
}

TEST(OidcVerifierTest, FaultAlgorithmKeyTypeMismatch)
{
    /* ES256 header but the signature is an RSA signature. */
    Json h = esHeader("ec-1");
    std::string input = octi::base64UrlEncode(h.dump()) + "." +
                        octi::base64UrlEncode(claimsFor("line").dump());
    std::string sig = signRs256(keys().rsa.get(), input);
    CHECK_STATUS(BAD_SIGNATURE,
                 verifier->verifyIdToken(
                     "line", input + "." + octi::base64UrlEncode(sig)));
    /* ES256 header selecting an RSA kid finds no usable key. */
    Json h2 = esHeader("rsa-1");
    std::string t = jwtSign(h2, claimsFor("line"), keys().ec.get());
    CHECK_STATUS(UNKNOWN_KEY, verifier->verifyIdToken("line", t));
}

TEST(OidcVerifierTest, FaultUnknownKidAndAttackerKey)
{
    CHECK_STATUS(UNKNOWN_KEY,
                 verifier->verifyIdToken(
                     "google", jwtSign(rsHeader("nope"), claimsFor("google"),
                                       keys().rsa.get())));
    /* Attacker signs with own key but claims the legitimate kid. */
    CHECK_STATUS(BAD_SIGNATURE,
                 verifier->verifyIdToken(
                     "google", jwtSign(rsHeader("rsa-1"), claimsFor("google"),
                                       keys().rsaAttacker.get())));
    Json h = rsHeader();
    h["kid"] = 7;
    CHECK_STATUS(MALFORMED,
                 verifier->verifyIdToken(
                     "google", jwtSign(h, claimsFor("google"), keys().rsa.get())));
}

TEST(OidcVerifierTest, FaultUnsupportedProvider)
{
    CHECK_STATUS(UNSUPPORTED_PROVIDER,
                 verifier->verifyIdToken("github", token("google")));
    CHECK_STATUS(UNSUPPORTED_PROVIDER,
                 verifier->verifyIdToken("guest", token("google")));
    CHECK_STATUS(UNSUPPORTED_PROVIDER, verifier->verifyIdToken("", token("google")));
    CHECK_STATUS(UNSUPPORTED_PROVIDER,
                 verifier->verifyIdToken("Google", token("google")));
    CHECK_FALSE(OidcVerifier::isSupportedProvider("guest"));
    CHECK_TRUE(OidcVerifier::isSupportedProvider("line"));
}

TEST(OidcVerifierTest, FaultMissingOrEmptySubject)
{
    Json c = claimsFor("google");
    c.erase("sub");
    CHECK_STATUS(MISSING_SUB, verifier->verifyIdToken(
                                  "google",
                                  jwtSign(rsHeader(), c, keys().rsa.get())));
    c["sub"] = "";
    CHECK_STATUS(MISSING_SUB, verifier->verifyIdToken(
                                  "google",
                                  jwtSign(rsHeader(), c, keys().rsa.get())));
    c["sub"] = 12345;
    CHECK_STATUS(MISSING_SUB, verifier->verifyIdToken(
                                  "google",
                                  jwtSign(rsHeader(), c, keys().rsa.get())));
}

TEST(OidcVerifierTest, FaultMalformedShapes)
{
    const std::string good = token("google");
    std::vector<std::string> parts = splitToken(good);
    std::vector<std::string> bad;
    bad.push_back("");
    bad.push_back("abc");
    bad.push_back("a.b");
    bad.push_back("a.b.c.d");
    bad.push_back("..");
    bad.push_back(".b.c");
    bad.push_back("a..c");
    bad.push_back(good + ".extra");
    bad.push_back(parts[0] + "." + parts[1] + ".!!!");
    bad.push_back("!!!." + parts[1] + "." + parts[2]);
    bad.push_back(octi::base64UrlEncode("[]") + "." + parts[1] + "." + parts[2]);
    bad.push_back(octi::base64UrlEncode("not json") + "." + parts[1] + "." + parts[2]);
    bad.push_back(octi::base64UrlEncode("{\"kid\":\"rsa-1\"}") + "." + parts[1] +
                  "." + parts[2]);
    for (size_t i = 0; i < bad.size(); ++i) {
        CHECK_FALSE(verifier->verifyIdToken("google", bad[i]).ok());
    }
    /* payload that is signed correctly but is not a JSON object */
    Json h = rsHeader();
    std::string input = octi::base64UrlEncode(h.dump()) + "." +
                        octi::base64UrlEncode("[1,2,3]");
    CHECK_STATUS(MALFORMED,
                 verifier->verifyIdToken(
                     "google",
                     input + "." + octi::base64UrlEncode(
                                       signRs256(keys().rsa.get(), input))));
    /* critical header extensions are refused */
    Json hc = rsHeader();
    hc["crit"] = Json::array({"exp"});
    CHECK_STATUS(MALFORMED,
                 verifier->verifyIdToken(
                     "google",
                     jwtSign(hc, claimsFor("google"), keys().rsa.get())));
    /* oversized token */
    CHECK_STATUS(MALFORMED,
                 verifier->verifyIdToken("google", std::string(20000, 'a')));
}

TEST(OidcVerifierTest, FaultNoKeySetConfigured)
{
    OidcVerifier fresh;
    fresh.setClock([]() -> int64_t { return kNow; });
    fresh.setClientId("google", "client-google");
    CHECK_STATUS(KEY_SET_UNAVAILABLE,
                 fresh.verifyIdToken("google", token("google")));
    CHECK_FALSE(fresh.refreshKeySet("google"));
}

TEST(OidcVerifierTest, KeySetInjectionRejectsUnusableJwks)
{
    OidcVerifier fresh;
    fresh.setClock([]() -> int64_t { return kNow; });
    fresh.setClientId("google", "client-google");
    CHECK_FALSE(fresh.setKeySet("github", defaultJwks(), kNow));
    CHECK_FALSE(fresh.setKeySet("google", "", kNow));
    CHECK_FALSE(fresh.setKeySet("google", "{not json", kNow));
    CHECK_FALSE(fresh.setKeySet("google", "[]", kNow));
    CHECK_FALSE(fresh.setKeySet("google", "{\"keys\":[]}", kNow));
    CHECK_FALSE(fresh.setKeySet("google", "{\"keys\":\"x\"}", kNow));

    Pkey weak = generateRsa(1024);
    std::vector<Json> e;
    e.push_back(rsaJwk(weak.get(), "weak"));
    CHECK_FALSE(fresh.setKeySet("google", jwksDocument(e), kNow));

    Json offCurve;
    offCurve["kty"] = "EC";
    offCurve["crv"] = "P-256";
    offCurve["kid"] = "bad";
    offCurve["x"] = octi::base64UrlEncode(std::string(32, '\x01'));
    offCurve["y"] = octi::base64UrlEncode(std::string(32, '\x02'));
    e.clear();
    e.push_back(offCurve);
    CHECK_FALSE(fresh.setKeySet("google", jwksDocument(e), kNow));

    Json wrongCurve = ecJwk(keys().ec.get(), "ec-1");
    wrongCurve["crv"] = "P-384";
    e.clear();
    e.push_back(wrongCurve);
    CHECK_FALSE(fresh.setKeySet("google", jwksDocument(e), kNow));

    Json encryptionKey = rsaJwk(keys().rsa.get(), "enc");
    encryptionKey["use"] = "enc";
    e.clear();
    e.push_back(encryptionKey);
    CHECK_FALSE(fresh.setKeySet("google", jwksDocument(e), kNow));

    Json octKey;
    octKey["kty"] = "oct";
    octKey["kid"] = "rsa-1";
    octKey["k"] = octi::base64UrlEncode("secret");
    e.clear();
    e.push_back(octKey);
    CHECK_FALSE(fresh.setKeySet("google", jwksDocument(e), kNow));

    /* a good set survives a later bad injection untouched */
    CHECK_TRUE(fresh.setKeySet("google", defaultJwks(), kNow));
    CHECK_FALSE(fresh.setKeySet("google", "{\"keys\":[]}", kNow));
    CHECK_STATUS(OK, fresh.verifyIdToken("google", token("google")));

    /* unusable entries are skipped while usable ones are kept */
    e.clear();
    e.push_back(octKey);
    e.push_back(rsaJwk(keys().rsa.get(), "rsa-1"));
    CHECK_TRUE(fresh.setKeySet("google", jwksDocument(e), kNow));
    CHECK_STATUS(OK, fresh.verifyIdToken("google", token("google")));
}

TEST(OidcVerifierTest, JwksRefreshThroughFetcherHonorsRotationAndRateLimit)
{
    OidcVerifier fresh;
    int64_t clock = kNow;
    fresh.setClock([&clock]() -> int64_t { return clock; });
    fresh.setClientId("google", "client-google");

    int calls = 0;
    std::string servedUrl;
    std::string served;
    {
        std::vector<Json> e;
        e.push_back(rsaJwk(keys().rsa.get(), "rsa-1"));
        served = jwksDocument(e);
    }
    fresh.setHttpFetcher([&](const std::string &url, octi::HttpResponse &resp) {
        ++calls;
        servedUrl = url;
        resp.status = 200;
        resp.body = served;
        return true;
    });

    /* no keys cached: first verify downloads the provider JWKS */
    CHECK_STATUS(OK, fresh.verifyIdToken("google", token("google")));
    CHECK_EQUAL(1, calls);
    STRCMP_EQUAL("https://www.googleapis.com/oauth2/v3/certs", servedUrl.c_str());

    /* cached: no further download */
    CHECK_STATUS(OK, fresh.verifyIdToken("google", token("google")));
    CHECK_EQUAL(1, calls);

    /* key rotation: unknown kid triggers one refresh, then verifies */
    {
        std::vector<Json> e;
        e.push_back(rsaJwk(keys().rsa.get(), "rsa-1"));
        e.push_back(rsaJwk(keys().rsaRotated.get(), "rsa-2"));
        served = jwksDocument(e);
    }
    std::string rotated = jwtSign(rsHeader("rsa-2"), claimsFor("google"),
                                  keys().rsaRotated.get());
    clock = kNow + 31;
    CHECK_STATUS(OK, fresh.verifyIdToken("google", rotated));
    CHECK_EQUAL(2, calls);

    /* a flood of unknown kids cannot hammer the provider endpoint */
    std::string unknown = jwtSign(rsHeader("rsa-9"), claimsFor("google"),
                                  keys().rsa.get());
    CHECK_STATUS(UNKNOWN_KEY, fresh.verifyIdToken("google", unknown));
    CHECK_EQUAL(2, calls);      // within 30 s of the previous refresh
    clock = kNow + 31 + 30;
    CHECK_STATUS(UNKNOWN_KEY, fresh.verifyIdToken("google", unknown));
    CHECK_EQUAL(3, calls);
    CHECK_STATUS(UNKNOWN_KEY, fresh.verifyIdToken("google", unknown));
    CHECK_STATUS(UNKNOWN_KEY, fresh.verifyIdToken("google", unknown));
    CHECK_EQUAL(3, calls);
    clock = kNow + 31 + 30 + 30;
    CHECK_STATUS(UNKNOWN_KEY, fresh.verifyIdToken("google", unknown));
    CHECK_EQUAL(4, calls);
}

TEST(OidcVerifierTest, JwksRefreshFailuresAreFailClosed)
{
    OidcVerifier fresh;
    fresh.setClock([]() -> int64_t { return kNow; });
    fresh.setClientId("google", "client-google");
    int mode = 0;
    fresh.setHttpFetcher([&](const std::string &, octi::HttpResponse &resp) {
        if (mode == 0) {
            return false;                   // transport failure
        }
        resp.status = (mode == 1) ? 500 : 200;
        resp.body = (mode == 2) ? "garbage" : "";
        return true;
    });
    CHECK_FALSE(fresh.refreshKeySet("google", kNow));
    CHECK_STATUS(KEY_SET_UNAVAILABLE,
                 fresh.verifyIdToken("google", token("google"), kNow + 100));
    mode = 1;
    CHECK_FALSE(fresh.refreshKeySet("google", kNow + 100));
    mode = 2;
    CHECK_FALSE(fresh.refreshKeySet("google", kNow + 200));
    CHECK_STATUS(KEY_SET_UNAVAILABLE,
                 fresh.verifyIdToken("google", token("google"), kNow + 300));
}

TEST(OidcVerifierTest, ConcurrentVerificationWhileKeySetIsReplaced)
{
    std::string t = token("google");
    std::atomic<int> failures(0);
    std::vector<std::thread> workers;
    for (int w = 0; w < 4; ++w) {
        workers.push_back(std::thread([&]() {
            for (int i = 0; i < 40; ++i) {
                if (!verifier->verifyIdToken("google", t).ok()) {
                    ++failures;
                }
            }
        }));
    }
    for (int i = 0; i < 20; ++i) {
        CHECK_TRUE(verifier->setKeySet("google", defaultJwks(), kNow));
    }
    for (size_t i = 0; i < workers.size(); ++i) {
        workers[i].join();
    }
    CHECK_EQUAL(0, failures.load());
}

/* ---- Facebook Graph access token (stand-in only at the remote boundary) ---- */

namespace {

struct FakeGraph {
    std::vector<std::string> urls;
    bool transportFailure = false;
    int debugStatus = 200;
    std::string debugBody;
    int meStatus = 200;
    std::string meBody;

    octi::HttpFetcher fetcher()
    {
        return [this](const std::string &url, octi::HttpResponse &resp) {
            urls.push_back(url);
            if (transportFailure) {
                return false;
            }
            bool isDebug = url.find("/debug_token?") != std::string::npos;
            resp.status = isDebug ? debugStatus : meStatus;
            resp.body = isDebug ? debugBody : meBody;
            return true;
        };
    }
};

std::string debugJson(bool valid, const std::string &appId,
                      const std::string &userId, int64_t expiresAt)
{
    Json d;
    d["data"]["is_valid"] = valid;
    d["data"]["app_id"] = appId;
    d["data"]["user_id"] = userId;
    d["data"]["expires_at"] = expiresAt;
    return d.dump();
}

std::string meJson(const std::string &id)
{
    Json m;
    m["id"] = id;
    m["name"] = "Fay Book";
    m["email"] = "fay@example.com";
    m["picture"]["data"]["url"] = "https://example.com/fb.png";
    return m.dump();
}

} // namespace

TEST(OidcVerifierTest, FacebookGraphAccessTokenNominal)
{
    FakeGraph graph;
    graph.debugBody = debugJson(true, "client-facebook", "1234567890", 0);
    graph.meBody = meJson("1234567890");
    verifier->setHttpFetcher(graph.fetcher());
    verifier->setFacebookAppSecret("app-secret");

    VerifyResult r = verifier->verifyFacebookAccessToken("EAAB&tok|en");
    CHECK_STATUS(OK, r);
    STRCMP_EQUAL("facebook", r.claims.provider.c_str());
    STRCMP_EQUAL("1234567890", r.claims.sub.c_str());
    STRCMP_EQUAL("Fay Book", r.claims.name.c_str());
    STRCMP_EQUAL("fay@example.com", r.claims.email.c_str());
    STRCMP_EQUAL("https://example.com/fb.png", r.claims.picture.c_str());

    CHECK_EQUAL(2, static_cast<int>(graph.urls.size()));
    STRCMP_EQUAL(
        "https://graph.facebook.com/debug_token?input_token=EAAB%26tok%7Cen"
        "&access_token=client-facebook%7Capp-secret",
        graph.urls[0].c_str());
    STRCMP_EQUAL("https://graph.facebook.com/me?fields=id,name,email,picture&"
                 "access_token=EAAB%26tok%7Cen",
                 graph.urls[1].c_str());
}

TEST(OidcVerifierTest, FacebookGraphAccessTokenFaults)
{
    FakeGraph graph;
    verifier->setHttpFetcher(graph.fetcher());
    verifier->setFacebookAppSecret("app-secret");

    graph.debugBody = debugJson(false, "client-facebook", "1", 0);
    graph.meBody = meJson("1");
    CHECK_STATUS(TOKEN_REJECTED, verifier->verifyFacebookAccessToken("tok"));

    graph.debugBody = debugJson(true, "another-app", "1", 0);
    CHECK_STATUS(BAD_AUDIENCE, verifier->verifyFacebookAccessToken("tok"));

    graph.debugBody = debugJson(true, "client-facebook", "1", kNow - 1);
    CHECK_STATUS(EXPIRED, verifier->verifyFacebookAccessToken("tok"));
    graph.debugBody = debugJson(true, "client-facebook", "1", kNow + 1);
    CHECK_STATUS(OK, verifier->verifyFacebookAccessToken("tok"));

    graph.debugBody = debugJson(true, "client-facebook", "2", 0);
    CHECK_STATUS(MISSING_SUB, verifier->verifyFacebookAccessToken("tok"));

    graph.debugBody = "{\"data\":{}}";
    CHECK_STATUS(TOKEN_REJECTED, verifier->verifyFacebookAccessToken("tok"));
    graph.debugBody = "not json";
    CHECK_STATUS(REMOTE_FAILURE, verifier->verifyFacebookAccessToken("tok"));
    graph.debugBody = debugJson(true, "client-facebook", "1", 0);
    graph.debugStatus = 400;
    CHECK_STATUS(REMOTE_FAILURE, verifier->verifyFacebookAccessToken("tok"));
    graph.debugStatus = 200;
    graph.meStatus = 500;
    CHECK_STATUS(REMOTE_FAILURE, verifier->verifyFacebookAccessToken("tok"));
    graph.meStatus = 200;
    graph.transportFailure = true;
    CHECK_STATUS(REMOTE_FAILURE, verifier->verifyFacebookAccessToken("tok"));

    CHECK_STATUS(MALFORMED, verifier->verifyFacebookAccessToken(""));
    CHECK_STATUS(MALFORMED,
                 verifier->verifyFacebookAccessToken(std::string(5000, 'x')));
}

TEST(OidcVerifierTest, FacebookGraphRequiresAppSecretAndClientId)
{
    FakeGraph graph;
    graph.debugBody = debugJson(true, "client-facebook", "1", 0);
    graph.meBody = meJson("1");
    verifier->setHttpFetcher(graph.fetcher());
    CHECK_STATUS(PROVIDER_DISABLED, verifier->verifyFacebookAccessToken("tok"));
    CHECK_EQUAL(0, static_cast<int>(graph.urls.size()));

    verifier->setFacebookAppSecret("app-secret");
    verifier->setClientId("facebook", "");
    CHECK_STATUS(PROVIDER_DISABLED, verifier->verifyFacebookAccessToken("tok"));

    OidcVerifier noFetcher;
    noFetcher.setClientId("facebook", "client-facebook");
    noFetcher.setFacebookAppSecret("app-secret");
    CHECK_STATUS(REMOTE_FAILURE, noFetcher.verifyFacebookAccessToken("tok"));
}

/* ---- RapidCheck properties ---- */

TEST(OidcVerifierTest, RapidCheckBase64UrlRoundTrip)
{
    bool passed = rc::check("base64url round trip on arbitrary bytes", []() {
        std::vector<uint8_t> bytes =
            *rc::gen::arbitrary<std::vector<uint8_t>>();
        std::string raw(bytes.begin(), bytes.end());
        std::string enc = octi::base64UrlEncode(raw);
        RC_ASSERT(enc.find('=') == std::string::npos);
        RC_ASSERT(enc.find('+') == std::string::npos);
        RC_ASSERT(enc.find('/') == std::string::npos);
        RC_ASSERT(enc.size() == (raw.size() * 4 + 2) / 3);
        std::string dec;
        RC_ASSERT(octi::base64UrlDecode(enc, dec));
        RC_ASSERT(dec == raw);
    });
    CHECK_TRUE(passed);
}

TEST(OidcVerifierTest, RapidCheckBase64UrlDecodeIsCanonical)
{
    bool passed = rc::check("base64url decode accepts only canonical text", []() {
        std::string text = *rc::gen::arbitrary<std::string>();
        std::string dec;
        if (octi::base64UrlDecode(text, dec)) {
            RC_ASSERT(octi::base64UrlEncode(dec) == text);
        } else {
            RC_ASSERT(dec.empty());
        }
    });
    CHECK_TRUE(passed);
}

TEST(OidcVerifierTest, RapidCheckVerifyArbitraryStringsAlwaysRejects)
{
    bool passed = rc::check("verifyIdToken rejects arbitrary strings", [&]() {
        std::string tok = *rc::gen::arbitrary<std::string>();
        RC_ASSERT(!verifier->verifyIdToken("google", tok).ok());
        /* structurally plausible three-segment garbage */
        std::string a = *rc::gen::arbitrary<std::string>();
        std::string b = *rc::gen::arbitrary<std::string>();
        std::string c = *rc::gen::arbitrary<std::string>();
        RC_ASSERT(!verifier->verifyIdToken("line", a + "." + b + "." + c).ok());
        RC_ASSERT(!verifier->verifyIdToken(a, tok).ok());
    });
    CHECK_TRUE(passed);
}

TEST(OidcVerifierTest, RapidCheckVerifyTruncatedAndMutatedTokensAlwaysReject)
{
    const std::string good = token("google");
    CHECK_STATUS(OK, verifier->verifyIdToken("google", good));
    bool passed = rc::check("truncated or mutated token never verifies", [&]() {
        size_t cut = *rc::gen::inRange<size_t>(0, good.size());
        RC_ASSERT(!verifier->verifyIdToken("google", good.substr(0, cut)).ok());

        size_t idx = *rc::gen::inRange<size_t>(0, good.size());
        char repl = *rc::gen::arbitrary<char>();
        RC_PRE(repl != good[idx]);
        std::string mutated = good;
        mutated[idx] = repl;
        RC_ASSERT(!verifier->verifyIdToken("google", mutated).ok());

        std::string extended = good + *rc::gen::arbitrary<std::string>();
        RC_PRE(extended != good);
        RC_ASSERT(!verifier->verifyIdToken("google", extended).ok());
    });
    CHECK_TRUE(passed);
}

TEST(OidcVerifierTest, RapidCheckVerifyOversizedInputsAlwaysReject)
{
    bool passed = rc::check("oversized tokens are rejected", [&]() {
        size_t extra = *rc::gen::inRange<size_t>(0, 60000);
        char fill = *rc::gen::element('A', 'z', '.', '-', '_');
        std::string big(16385 + extra, fill);
        RC_ASSERT(!verifier->verifyIdToken("google", big).ok());
        std::string shaped = "e30." + std::string(16385 + extra, 'A') + ".AAAA";
        RC_ASSERT(!verifier->verifyIdToken("apple", shaped).ok());
        RC_ASSERT(!verifier->verifyFacebookAccessToken(big).ok());
    });
    CHECK_TRUE(passed);
}

/* ================================================================== */
/* AccountStoreTest                                                    */
/* ================================================================== */

TEST_GROUP(AccountStoreTest)
{
    TempDir *dir;
    AccountStore *store;
    int64_t clockNow;

    std::string dbPath() { return dir->path() + "/data/accounts.db"; }

    void setup()
    {
        dir = new TempDir();
        store = new AccountStore();
        clockNow = kNow;
        store->setClock([this]() -> int64_t { return clockNow; });
        std::string err;
        CHECK_TRUE_TEXT(!dir->path().empty(), "mkdtemp failed");
        CHECK_TRUE_TEXT(store->openOrCreate(dbPath(), err), err.c_str());
    }

    void teardown()
    {
        delete store;
        store = nullptr;
        delete dir;
        dir = nullptr;
    }

    LoginResult login(const std::string &provider, const std::string &sub,
                      const std::string &name = "Test User")
    {
        return store->loginWithSso(makeClaims(provider, sub, name),
                                   "octi-test/1.0");
    }

    int64_t newPlayer(const std::string &sub, const std::string &name)
    {
        LoginResult r = login("google", sub, name);
        CHECK_TRUE_TEXT(r.ok, r.error.c_str());
        return r.profile.playerId;
    }

    GameRecord record(const std::string &id, int64_t endedAt, int64_t p1,
                      int64_t p2, const std::string &mode = "2P")
    {
        octi::OctiBoard board(octi::GAME_MODE_2);
        octi::OctiGame *game = board.getGame();
        game->start(octi::PLAYER_A);
        game->move("33+b");
        game->move("37+e");
        game->move("33");
        game->move("37");
        GameRecord g;
        g.gameId = id;
        g.gameMode = mode;
        g.gameBases = 3;
        g.timeControl = "600+5";
        g.p1PlayerId = p1;
        g.p2PlayerId = p2;
        g.p1Handle = "alpha";
        g.p2Handle = "beta";
        g.result = "P1_WIN";
        g.winnerSeat = 1;
        g.termination = "RESIGNATION";
        g.plyCount = 4;
        g.octiTranscript = board.serializeGame();
        g.finalFen = board.toPositionString();
        g.startedAt = endedAt - 300;
        g.endedAt = endedAt;
        return g;
    }

    int64_t countRows(const std::string &sql)
    {
        sqlite3 *db = nullptr;
        int64_t n = -1;
        if (sqlite3_open_v2(dbPath().c_str(), &db, SQLITE_OPEN_READONLY,
                            nullptr) == SQLITE_OK) {
            sqlite3_stmt *st = nullptr;
            if (sqlite3_prepare_v2(db, sql.c_str(), -1, &st, nullptr) ==
                    SQLITE_OK &&
                sqlite3_step(st) == SQLITE_ROW) {
                n = sqlite3_column_int64(st, 0);
            }
            sqlite3_finalize(st);
        }
        sqlite3_close(db);
        return n;
    }

    std::string queryText(const std::string &sql)
    {
        sqlite3 *db = nullptr;
        std::string out;
        if (sqlite3_open_v2(dbPath().c_str(), &db, SQLITE_OPEN_READONLY,
                            nullptr) == SQLITE_OK) {
            sqlite3_stmt *st = nullptr;
            if (sqlite3_prepare_v2(db, sql.c_str(), -1, &st, nullptr) ==
                    SQLITE_OK &&
                sqlite3_step(st) == SQLITE_ROW) {
                const unsigned char *t = sqlite3_column_text(st, 0);
                if (t) {
                    out = reinterpret_cast<const char *>(t);
                }
            }
            sqlite3_finalize(st);
        }
        sqlite3_close(db);
        return out;
    }
};

/* ---- Storage layout, permissions, schema ---- */

TEST(AccountStoreTest, CreatesWalDatabaseWithRestrictedPermissions)
{
    CHECK_TRUE(store->isOpen());
    CHECK_EQUAL(0600, static_cast<int>(modeOf(dbPath())));
    CHECK_EQUAL(0700, static_cast<int>(modeOf(dir->path() + "/data")));
    STRCMP_EQUAL("wal", queryText("PRAGMA journal_mode").c_str());

    /* sidecar files, once they exist, are not wider than 0600 */
    CHECK_TRUE(login("google", "perm-check").ok);
    std::string wal = dbPath() + "-wal";
    if (::access(wal.c_str(), F_OK) == 0) {
        CHECK_EQUAL(0600, static_cast<int>(modeOf(wal)));
    }
}

TEST(AccountStoreTest, ExistingLooseFileAndDirectoryAreTightened)
{
    std::string loose = dir->path() + "/loose";
    CHECK_EQUAL(0, ::mkdir(loose.c_str(), 0755));
    CHECK_EQUAL(0, ::chmod(loose.c_str(), 0755));
    {
        std::ofstream f(loose + "/x.db");
        f << "";
    }
    CHECK_EQUAL(0, ::chmod((loose + "/x.db").c_str(), 0644));
    AccountStore s2;
    std::string err;
    CHECK_TRUE_TEXT(s2.openOrCreate(loose + "/x.db", err), err.c_str());
    CHECK_EQUAL(0600, static_cast<int>(modeOf(loose + "/x.db")));
    CHECK_EQUAL(0700, static_cast<int>(modeOf(loose)));
}

TEST(AccountStoreTest, OpenFailsForBadPathsAndDoubleOpen)
{
    AccountStore s2;
    std::string err;
    CHECK_FALSE(s2.openOrCreate("", err));
    CHECK_FALSE(err.empty());
    /* a regular file used as a directory component */
    CHECK_FALSE(s2.openOrCreate(dbPath() + "/child.db", err));
    CHECK_FALSE(s2.isOpen());
    CHECK_FALSE(store->openOrCreate(dbPath(), err));
    CHECK_TRUE(store->isOpen());
}

TEST(AccountStoreTest, SchemaMatchesPlanAndHasNoGuestSupport)
{
    CHECK_EQUAL(4, countRows("SELECT COUNT(*) FROM sqlite_master WHERE "
                             "type='table' AND name IN ('players',"
                             "'linked_identities','sessions','games')"));
    CHECK_EQUAL(0, countRows("SELECT COUNT(*) FROM pragma_table_info('players')"
                             " WHERE name LIKE '%guest%'"));
    CHECK_EQUAL(14, countRows("SELECT COUNT(*) FROM pragma_table_info('players')"));
    CHECK_EQUAL(8, countRows(
                       "SELECT COUNT(*) FROM pragma_table_info('linked_identities')"));
    CHECK_EQUAL(7, countRows("SELECT COUNT(*) FROM pragma_table_info('sessions')"));
    CHECK_EQUAL(31, countRows("SELECT COUNT(*) FROM pragma_table_info('games')"));
    CHECK_TRUE(countRows("SELECT COUNT(*) FROM sqlite_master WHERE type='index'"
                         " AND name='idx_games_ended_at'") == 1);

    LoginResult guest = store->loginWithSso(makeClaims("guest", "anon"), "x");
    CHECK_FALSE(guest.ok);
    CHECK_EQUAL(0, countRows("SELECT COUNT(*) FROM players"));
}

TEST(AccountStoreTest, DataSurvivesCloseAndReopen)
{
    LoginResult r = login("google", "persist-1", "Persist Me");
    CHECK_TRUE(r.ok);
    store->close();
    CHECK_FALSE(store->isOpen());
    CHECK_FALSE(login("google", "persist-1").ok);

    std::string err;
    CHECK_TRUE_TEXT(store->openOrCreate(dbPath(), err), err.c_str());
    LoginResult again = login("google", "persist-1");
    CHECK_TRUE(again.ok);
    CHECK_FALSE(again.created);
    CHECK_EQUAL(r.profile.playerId, again.profile.playerId);
    STRCMP_EQUAL(r.profile.uuid.c_str(), again.profile.uuid.c_str());
    CHECK_TRUE(store->validateSession(r.sessionToken).valid);
}

/* ---- SSO login ---- */

TEST(AccountStoreTest, FirstSsoLoginProvisionsPlayerAndSession)
{
    LoginResult r = store->loginWithSso(
        makeClaims("google", "g-sub-1", "Samurai Jack", "jack@example.com"),
        "octi-android/2.0.1");
    CHECK_TRUE_TEXT(r.ok, r.error.c_str());
    CHECK_TRUE(r.created);
    CHECK_TRUE(r.profile.playerId > 0);
    STRCMP_EQUAL("Samurai_Jack", r.profile.handle.c_str());
    STRCMP_EQUAL("Samurai Jack", r.profile.displayName.c_str());
    STRCMP_EQUAL("https://example.com/avatar.png", r.profile.avatarUrl.c_str());
    CHECK_EQUAL(1200, r.profile.elo2p);
    CHECK_EQUAL(1200, r.profile.elo4p);
    CHECK_EQUAL(1200, r.profile.eloOfk);
    CHECK_EQUAL(0, r.profile.gamesPlayed);
    CHECK_EQUAL(kNow, r.profile.createdAt);
    CHECK_EQUAL(36, static_cast<int>(r.profile.uuid.size()));
    CHECK_EQUAL('4', r.profile.uuid[14]);
    CHECK_TRUE(r.profile.uuid[19] == '8' || r.profile.uuid[19] == '9' ||
               r.profile.uuid[19] == 'a' || r.profile.uuid[19] == 'b');
    CHECK_EQUAL(1, static_cast<int>(r.identities.size()));
    STRCMP_EQUAL("google", r.identities[0].provider.c_str());
    STRCMP_EQUAL("g-sub-1", r.identities[0].providerSub.c_str());
    STRCMP_EQUAL("jack@example.com", r.identities[0].providerEmail.c_str());
    CHECK_EQUAL(43, static_cast<int>(r.sessionToken.size()));
    CHECK_EQUAL(kNow + AccountStore::kDefaultSessionTtlSec, r.sessionExpiresAt);

    octi::SessionValidation v = store->validateSession(r.sessionToken);
    CHECK_TRUE(v.valid);
    CHECK_EQUAL(r.profile.playerId, v.playerId);
    CHECK_EQUAL(r.sessionExpiresAt, v.expiresAt);
}

TEST(AccountStoreTest, RepeatLoginReusesPlayerAndMintsFreshSession)
{
    LoginResult first = login("apple", "a-sub-1", "Apple User");
    clockNow = kNow + 500;
    LoginResult second = login("apple", "a-sub-1", "Renamed At Provider");
    CHECK_TRUE(first.ok && second.ok);
    CHECK_TRUE(first.created);
    CHECK_FALSE(second.created);
    CHECK_EQUAL(first.profile.playerId, second.profile.playerId);
    STRCMP_EQUAL(first.profile.handle.c_str(), second.profile.handle.c_str());
    CHECK_TRUE(first.sessionToken != second.sessionToken);
    CHECK_EQUAL(kNow + 500, second.profile.lastSeenAt);
    CHECK_EQUAL(kNow + 500, second.identities[0].lastLoginAt);
    CHECK_EQUAL(kNow, second.identities[0].linkedAt);
    CHECK_EQUAL(1, static_cast<int>(second.identities.size()));
    CHECK_EQUAL(1, countRows("SELECT COUNT(*) FROM players"));
    CHECK_EQUAL(2, countRows("SELECT COUNT(*) FROM sessions"));
}

TEST(AccountStoreTest, EmailIsInformationalAndNeverAnAccountKey)
{
    LoginResult a = store->loginWithSso(
        makeClaims("google", "g-A", "Same Email", "shared@example.com"), "t");
    LoginResult b = store->loginWithSso(
        makeClaims("apple", "a-B", "Same Email", "shared@example.com"), "t");
    CHECK_TRUE(a.ok && b.ok);
    CHECK_TRUE(a.profile.playerId != b.profile.playerId);
    /* the provider changes the email: still the same account */
    LoginResult a2 = store->loginWithSso(
        makeClaims("google", "g-A", "Same Email", "changed@example.com"), "t");
    CHECK_EQUAL(a.profile.playerId, a2.profile.playerId);
    STRCMP_EQUAL("changed@example.com", a2.identities[0].providerEmail.c_str());
    /* a login without email keeps the previously stored one */
    LoginResult a3 = store->loginWithSso(makeClaims("google", "g-A", "X", ""), "t");
    STRCMP_EQUAL("changed@example.com", a3.identities[0].providerEmail.c_str());
}

TEST(AccountStoreTest, SameSubjectOnDifferentProvidersAreDifferentIdentities)
{
    LoginResult g = login("google", "same-sub");
    LoginResult l = login("line", "same-sub");
    CHECK_TRUE(g.ok && l.ok);
    CHECK_TRUE(g.profile.playerId != l.profile.playerId);
}

TEST(AccountStoreTest, LoginRejectsInvalidClaims)
{
    CHECK_FALSE(store->loginWithSso(makeClaims("google", ""), "t").ok);
    CHECK_FALSE(store->loginWithSso(makeClaims("google", std::string(256, 'x')), "t").ok);
    CHECK_FALSE(store->loginWithSso(makeClaims("twitter", "x"), "t").ok);
    CHECK_FALSE(store->loginWithSso(makeClaims("", "x"), "t").ok);
    CHECK_EQUAL(0, countRows("SELECT COUNT(*) FROM players"));
    CHECK_EQUAL(0, countRows("SELECT COUNT(*) FROM sessions"));
}

TEST(AccountStoreTest, HandleDerivationIsSanitizedBoundedAndCollisionSafe)
{
    struct Case {
        const char *name;
        const char *expected;   // nullptr = Player-<8 hex>
    };
    Case cases[] = {
        {"Alice Smith", "Alice_Smith"},
        {"a.b-c_d", "a_b-c_d"},
        {"Bad<>Chars!!", "BadChars"},
        {"ab", nullptr},
        {"", nullptr},
        {"\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e", nullptr},
        {"abc", "abc"},
        {"123456789012345678901234567890", "123456789012345678901234"},
    };
    int n = 0;
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        LoginResult r = login("google", "hd-" + std::to_string(n++), cases[i].name);
        CHECK_TRUE_TEXT(r.ok, r.error.c_str());
        CHECK_TRUE(AccountStore::isValidHandle(r.profile.handle));
        if (cases[i].expected) {
            STRCMP_EQUAL(cases[i].expected, r.profile.handle.c_str());
        } else {
            CHECK_EQUAL(15, static_cast<int>(r.profile.handle.size()));
            CHECK_TRUE(r.profile.handle.compare(0, 7, "Player-") == 0);
        }
        CHECK_FALSE(r.profile.displayName.empty());
    }

    /* collisions, including case-insensitive ones, never reuse a handle */
    std::set<std::string> seen;
    const char *names[] = {"Bob", "Bob", "bob", "BOB"};
    for (int i = 0; i < 4; ++i) {
        LoginResult r = login("google", "col-" + std::to_string(i), names[i]);
        CHECK_TRUE(r.ok);
        std::string lower = r.profile.handle;
        for (size_t k = 0; k < lower.size(); ++k) {
            lower[k] = static_cast<char>(std::tolower(
                static_cast<unsigned char>(lower[k])));
        }
        CHECK_TRUE(seen.insert(lower).second);
        CHECK_TRUE(AccountStore::isValidHandle(r.profile.handle));
    }
    CHECK_EQUAL(4, static_cast<int>(seen.size()));
}

TEST(AccountStoreTest, ConcurrentFirstLoginsGetUniqueHandles)
{
    std::atomic<int> failures(0);
    std::vector<std::thread> workers;
    for (int t = 0; t < 4; ++t) {
        workers.push_back(std::thread([&, t]() {
            for (int i = 0; i < 8; ++i) {
                LoginResult r = store->loginWithSso(
                    makeClaims("google", "race-" + std::to_string(t) + "-" +
                                             std::to_string(i),
                               "Racer"),
                    "t");
                if (!r.ok) {
                    ++failures;
                }
            }
        }));
    }
    for (size_t i = 0; i < workers.size(); ++i) {
        workers[i].join();
    }
    CHECK_EQUAL(0, failures.load());
    CHECK_EQUAL(32, countRows("SELECT COUNT(*) FROM players"));
    CHECK_EQUAL(32, countRows("SELECT COUNT(DISTINCT lower(handle)) FROM players"));
}

TEST(AccountStoreTest, TwoStoreInstancesShareOneDatabaseFile)
{
    AccountStore other;
    std::string err;
    CHECK_TRUE_TEXT(other.openOrCreate(dbPath(), err), err.c_str());
    std::atomic<int> failures(0);
    std::thread t([&]() {
        for (int i = 0; i < 10; ++i) {
            if (!other.loginWithSso(makeClaims("apple", "o-" + std::to_string(i)),
                                    "t").ok) {
                ++failures;
            }
        }
    });
    for (int i = 0; i < 10; ++i) {
        if (!store->loginWithSso(makeClaims("google", "m-" + std::to_string(i)),
                                 "t").ok) {
            ++failures;
        }
    }
    t.join();
    CHECK_EQUAL(0, failures.load());
    CHECK_EQUAL(20, countRows("SELECT COUNT(*) FROM players"));
}

/* ---- Linking ---- */

TEST(AccountStoreTest, LinkAllFiveProvidersToOnePlayerAndLoginViaEach)
{
    LoginResult base = store->loginWithSso(
        makeClaims("google", "g-1", "Multi Linker"), "t");
    CHECK_TRUE(base.ok);
    int64_t pid = base.profile.playerId;

    const char *others[] = {"apple", "microsoft", "facebook", "line"};
    for (int i = 0; i < 4; ++i) {
        octi::LinkResult lr = store->linkIdentity(
            pid, makeClaims(others[i], std::string("sub-") + others[i]));
        STRCMP_EQUAL("LINKED", linkName(lr.status));
    }
    std::vector<octi::LinkedIdentity> ids;
    CHECK_TRUE(store->listLinkedIdentities(pid, ids));
    CHECK_EQUAL(5, static_cast<int>(ids.size()));

    std::vector<std::string> providers = allProviders();
    for (size_t i = 0; i < providers.size(); ++i) {
        std::string sub = providers[i] == "google" ? "g-1"
                                                   : "sub-" + providers[i];
        LoginResult r = store->loginWithSso(makeClaims(providers[i], sub), "t");
        CHECK_TRUE_TEXT(r.ok, r.error.c_str());
        CHECK_FALSE(r.created);
        CHECK_EQUAL(pid, r.profile.playerId);
        CHECK_EQUAL(5, static_cast<int>(r.identities.size()));
        STRCMP_EQUAL(base.profile.handle.c_str(), r.profile.handle.c_str());
    }
    CHECK_EQUAL(1, countRows("SELECT COUNT(*) FROM players"));
}

TEST(AccountStoreTest, LinkIsIdempotentForSamePlayer)
{
    LoginResult base = login("google", "g-1");
    int64_t pid = base.profile.playerId;
    CHECK_EQUAL(0, static_cast<int>(store->linkIdentity(pid, makeClaims("apple", "a-1")).status));
    clockNow = kNow + 99;
    octi::LinkResult again = store->linkIdentity(
        pid, makeClaims("apple", "a-1", "New Name", "new@example.com"));
    STRCMP_EQUAL("ALREADY_LINKED_SELF", linkName(again.status));
    std::vector<octi::LinkedIdentity> ids;
    CHECK_TRUE(store->listLinkedIdentities(pid, ids));
    CHECK_EQUAL(2, static_cast<int>(ids.size()));
    STRCMP_EQUAL("new@example.com", ids[1].providerEmail.c_str());
    STRCMP_EQUAL("New Name", ids[1].displayHint.c_str());
    CHECK_EQUAL(kNow + 99, ids[1].lastLoginAt);
    CHECK_EQUAL(kNow, ids[1].linkedAt);
}

TEST(AccountStoreTest, LinkConflictReportsOwnerWithoutMerging)
{
    LoginResult a = login("google", "g-A", "Owner Player");
    LoginResult b = login("apple", "a-B", "Other Player");
    int64_t aid = a.profile.playerId;
    int64_t bid = b.profile.playerId;
    std::vector<octi::RatingUpdate> up;
    octi::RatingUpdate u;
    u.playerId = aid;
    u.newElo = 1337;
    u.outcome = GameOutcome::WIN;
    up.push_back(u);
    std::string err;
    CHECK_TRUE(store->applyRatingUpdates("2P", up, err));

    octi::LinkResult lr = store->linkIdentity(bid, makeClaims("google", "g-A"));
    STRCMP_EQUAL("CONFLICT_ALREADY_LINKED", linkName(lr.status));
    STRCMP_EQUAL(a.profile.uuid.c_str(), lr.conflictProfile.uuid.c_str());
    STRCMP_EQUAL(a.profile.handle.c_str(), lr.conflictProfile.handle.c_str());
    CHECK_EQUAL(1337, lr.conflictProfile.elo2p);
    CHECK_EQUAL(1, lr.conflictProfile.gamesPlayed);

    std::vector<octi::LinkedIdentity> ida, idb;
    CHECK_TRUE(store->listLinkedIdentities(aid, ida));
    CHECK_TRUE(store->listLinkedIdentities(bid, idb));
    CHECK_EQUAL(1, static_cast<int>(ida.size()));
    CHECK_EQUAL(1, static_cast<int>(idb.size()));
    CHECK_EQUAL(2, countRows("SELECT COUNT(*) FROM players"));
}

TEST(AccountStoreTest, LinkFaults)
{
    LoginResult a = login("google", "g-A");
    STRCMP_EQUAL("PLAYER_NOT_FOUND",
                 linkName(store->linkIdentity(9999, makeClaims("apple", "x")).status));
    STRCMP_EQUAL("INVALID_CLAIMS",
                 linkName(store->linkIdentity(a.profile.playerId,
                                              makeClaims("guest", "x")).status));
    STRCMP_EQUAL("INVALID_CLAIMS",
                 linkName(store->linkIdentity(a.profile.playerId,
                                              makeClaims("apple", "")).status));
    AccountStore closed;
    STRCMP_EQUAL("ERROR",
                 linkName(closed.linkIdentity(1, makeClaims("apple", "x")).status));
}

TEST(AccountStoreTest, UnlinkDownToExactlyOneRemainingIdentity)
{
    LoginResult base = login("google", "g-1");
    int64_t pid = base.profile.playerId;
    const char *others[] = {"apple", "microsoft", "facebook", "line"};
    for (int i = 0; i < 4; ++i) {
        CHECK_TRUE(store->linkIdentity(pid, makeClaims(others[i], "s")).status ==
                   LinkStatus::LINKED);
    }
    for (int i = 0; i < 4; ++i) {
        STRCMP_EQUAL("UNLINKED", unlinkName(store->unlinkIdentity(pid, others[i])));
        std::vector<octi::LinkedIdentity> ids;
        CHECK_TRUE(store->listLinkedIdentities(pid, ids));
        CHECK_EQUAL(4 - i, static_cast<int>(ids.size()));
    }
    STRCMP_EQUAL("LAST_IDENTITY_FORBIDDEN",
                 unlinkName(store->unlinkIdentity(pid, "google")));
    STRCMP_EQUAL("NOT_LINKED", unlinkName(store->unlinkIdentity(pid, "apple")));
    STRCMP_EQUAL("NOT_LINKED", unlinkName(store->unlinkIdentity(pid, "twitter")));
    std::vector<octi::LinkedIdentity> ids;
    CHECK_TRUE(store->listLinkedIdentities(pid, ids));
    CHECK_EQUAL(1, static_cast<int>(ids.size()));
    STRCMP_EQUAL("google", ids[0].provider.c_str());
    /* the surviving identity still logs in */
    CHECK_TRUE(login("google", "g-1").ok);
    /* an unlinked identity is free again and provisions a new player */
    LoginResult fresh = login("apple", "s");
    CHECK_TRUE(fresh.created);
    CHECK_TRUE(fresh.profile.playerId != pid);
}

TEST(AccountStoreTest, UnlinkNeverOrphansAPlayerWithTwoIdentitiesOfOneProvider)
{
    LoginResult base = login("google", "g-first");
    int64_t pid = base.profile.playerId;
    CHECK_TRUE(store->linkIdentity(pid, makeClaims("google", "g-second")).status ==
               LinkStatus::LINKED);
    /* removing provider 'google' would delete every identity of the player */
    STRCMP_EQUAL("LAST_IDENTITY_FORBIDDEN",
                 unlinkName(store->unlinkIdentity(pid, "google")));
    std::vector<octi::LinkedIdentity> ids;
    CHECK_TRUE(store->listLinkedIdentities(pid, ids));
    CHECK_EQUAL(2, static_cast<int>(ids.size()));
}

TEST(AccountStoreTest, UnlinkOtherPlayersIdentityIsNotPossible)
{
    LoginResult a = login("google", "g-A");
    LoginResult b = login("apple", "a-B");
    CHECK_TRUE(store->linkIdentity(b.profile.playerId, makeClaims("line", "l-B")).status ==
               LinkStatus::LINKED);
    STRCMP_EQUAL("NOT_LINKED", unlinkName(store->unlinkIdentity(a.profile.playerId, "line")));
    std::vector<octi::LinkedIdentity> ids;
    CHECK_TRUE(store->listLinkedIdentities(b.profile.playerId, ids));
    CHECK_EQUAL(2, static_cast<int>(ids.size()));
}

/* ---- Profiles ---- */

TEST(AccountStoreTest, ProfileLookupByIdAndUuid)
{
    LoginResult r = login("google", "g-1", "Lookup Me");
    PlayerProfile byId, byUuid;
    CHECK_TRUE(store->getProfileById(r.profile.playerId, byId));
    CHECK_TRUE(store->getProfileByUuid(r.profile.uuid, byUuid));
    STRCMP_EQUAL(byId.handle.c_str(), byUuid.handle.c_str());
    CHECK_EQUAL(byId.playerId, byUuid.playerId);
    CHECK_FALSE(store->getProfileById(424242, byId));
    CHECK_FALSE(store->getProfileByUuid("00000000-0000-4000-8000-000000000000", byUuid));
    CHECK_FALSE(store->getProfileByUuid("' OR '1'='1", byUuid));
}

TEST(AccountStoreTest, BoundaryHandleLengthThreeToTwentyFour)
{
    int64_t pid = newPlayer("g-1", "Handle Tester");
    octi::ProfileUpdate u;
    u.setHandle = true;

    struct Case {
        std::string handle;
        const char *expected;
    };
    Case cases[] = {
        {"abc", "OK"},
        {std::string(24, 'x'), "OK"},
        {"ab", "INVALID_HANDLE"},
        {"", "INVALID_HANDLE"},
        {std::string(25, 'x'), "INVALID_HANDLE"},
        {"has space", "INVALID_HANDLE"},
        {"semi;colon", "INVALID_HANDLE"},
        {"quote'quote", "INVALID_HANDLE"},
        {"uni\xc3\xa9", "INVALID_HANDLE"},
        {"a-b_c", "OK"},
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        u.handle = cases[i].handle;
        STRCMP_EQUAL(cases[i].expected, updateName(store->updateProfile(pid, u)));
    }
    PlayerProfile p;
    CHECK_TRUE(store->getProfileById(pid, p));
    STRCMP_EQUAL("a-b_c", p.handle.c_str());
    CHECK_TRUE(AccountStore::isValidHandle("abc"));
    CHECK_TRUE(AccountStore::isValidHandle(std::string(24, 'a')));
    CHECK_FALSE(AccountStore::isValidHandle("ab"));
    CHECK_FALSE(AccountStore::isValidHandle(std::string(25, 'a')));
}

TEST(AccountStoreTest, HandleUniquenessIsCaseInsensitive)
{
    int64_t a = newPlayer("g-A", "Zed One");
    int64_t b = newPlayer("g-B", "Zed Two");
    octi::ProfileUpdate u;
    u.setHandle = true;
    u.handle = "Champion";
    STRCMP_EQUAL("OK", updateName(store->updateProfile(a, u)));
    u.handle = "champion";
    STRCMP_EQUAL("HANDLE_TAKEN", updateName(store->updateProfile(b, u)));
    u.handle = "CHAMPION";
    STRCMP_EQUAL("HANDLE_TAKEN", updateName(store->updateProfile(b, u)));
    /* the owner may change only the letter case of its own handle */
    u.handle = "CHAMPION";
    STRCMP_EQUAL("OK", updateName(store->updateProfile(a, u)));
    PlayerProfile p;
    CHECK_TRUE(store->getProfileById(a, p));
    STRCMP_EQUAL("CHAMPION", p.handle.c_str());
}

TEST(AccountStoreTest, UpdateDisplayNameAndAvatarWithValidation)
{
    int64_t pid = newPlayer("g-1", "Profile Tester");
    octi::ProfileUpdate u;
    u.setDisplayName = true;
    u.displayName = "\xe3\x82\xb5\xe3\x83\xa0\xe3\x83\xa9\xe3\x82\xa4";
    u.setAvatarUrl = true;
    u.avatarUrl = "https://cdn.example.com/a.png";
    STRCMP_EQUAL("OK", updateName(store->updateProfile(pid, u)));
    PlayerProfile p;
    CHECK_TRUE(store->getProfileById(pid, p));
    STRCMP_EQUAL(u.displayName.c_str(), p.displayName.c_str());
    STRCMP_EQUAL(u.avatarUrl.c_str(), p.avatarUrl.c_str());
    STRCMP_EQUAL("Profile_Tester", p.handle.c_str());

    octi::ProfileUpdate bad;
    bad.setDisplayName = true;
    bad.displayName = "";
    STRCMP_EQUAL("INVALID_DISPLAY_NAME", updateName(store->updateProfile(pid, bad)));
    bad.displayName = "   ";
    STRCMP_EQUAL("INVALID_DISPLAY_NAME", updateName(store->updateProfile(pid, bad)));
    bad.displayName = std::string(65, 'n');
    STRCMP_EQUAL("INVALID_DISPLAY_NAME", updateName(store->updateProfile(pid, bad)));
    bad.displayName = std::string(64, 'n');
    STRCMP_EQUAL("OK", updateName(store->updateProfile(pid, bad)));
    bad.displayName = "line\nbreak";
    STRCMP_EQUAL("INVALID_DISPLAY_NAME", updateName(store->updateProfile(pid, bad)));
    bad.displayName = "bad\xff\xfe";
    STRCMP_EQUAL("INVALID_DISPLAY_NAME", updateName(store->updateProfile(pid, bad)));

    octi::ProfileUpdate avatar;
    avatar.setAvatarUrl = true;
    avatar.avatarUrl = "javascript:alert(1)";
    STRCMP_EQUAL("INVALID_AVATAR_URL", updateName(store->updateProfile(pid, avatar)));
    avatar.avatarUrl = "http://insecure.example.com/a.png";
    STRCMP_EQUAL("INVALID_AVATAR_URL", updateName(store->updateProfile(pid, avatar)));
    avatar.avatarUrl = "https://x.example.com/has space";
    STRCMP_EQUAL("INVALID_AVATAR_URL", updateName(store->updateProfile(pid, avatar)));
    avatar.avatarUrl = "https://" + std::string(600, 'a');
    STRCMP_EQUAL("INVALID_AVATAR_URL", updateName(store->updateProfile(pid, avatar)));
    avatar.avatarUrl = "";
    STRCMP_EQUAL("OK", updateName(store->updateProfile(pid, avatar)));
    CHECK_TRUE(store->getProfileById(pid, p));
    CHECK_TRUE(p.avatarUrl.empty());

    STRCMP_EQUAL("NOT_FOUND", updateName(store->updateProfile(777777, u)));
}

TEST(AccountStoreTest, SqlInjectionPayloadsAreStoredVerbatim)
{
    const std::string evil = "x'); DROP TABLE players; --";
    LoginResult r = store->loginWithSso(makeClaims("google", evil, evil, evil), evil);
    CHECK_TRUE_TEXT(r.ok, r.error.c_str());
    CHECK_EQUAL(1, countRows("SELECT COUNT(*) FROM players"));
    STRCMP_EQUAL(evil.c_str(), r.identities[0].providerSub.c_str());
    CHECK_TRUE(AccountStore::isValidHandle(r.profile.handle));

    octi::ProfileUpdate u;
    u.setDisplayName = true;
    u.displayName = evil;
    STRCMP_EQUAL("OK", updateName(store->updateProfile(r.profile.playerId, u)));
    PlayerProfile p;
    CHECK_TRUE(store->getProfileById(r.profile.playerId, p));
    STRCMP_EQUAL(evil.c_str(), p.displayName.c_str());
    CHECK_EQUAL(1, countRows("SELECT COUNT(*) FROM players"));
    CHECK_FALSE(store->validateSession(evil).valid);
}

/* ---- Sessions ---- */

TEST(AccountStoreTest, SessionTokenIsNeverStoredInPlaintext)
{
    LoginResult r = login("google", "g-1");
    CHECK_TRUE(r.ok);
    std::string hash = AccountStore::sha256Hex(r.sessionToken);
    CHECK_EQUAL(64, static_cast<int>(hash.size()));
    for (size_t i = 0; i < hash.size(); ++i) {
        CHECK_TRUE((hash[i] >= '0' && hash[i] <= '9') ||
                   (hash[i] >= 'a' && hash[i] <= 'f'));
    }
    STRCMP_EQUAL(hash.c_str(),
                 queryText("SELECT token_hash FROM sessions").c_str());
    CHECK_EQUAL(0, countRows("SELECT COUNT(*) FROM sessions WHERE token_hash = '" +
                             r.sessionToken + "'"));

    /* raw bytes on disk (database + WAL): hash present, token absent */
    std::string disk = readFile(dbPath()) + readFile(dbPath() + "-wal");
    CHECK_TRUE(disk.find(hash) != std::string::npos);
    CHECK_TRUE(disk.find(r.sessionToken) == std::string::npos);

    /* SHA-256 known answer */
    STRCMP_EQUAL("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
                 AccountStore::sha256Hex("abc").c_str());
}

TEST(AccountStoreTest, SessionTokensAreUniqueHighEntropyBase64Url)
{
    std::set<std::string> tokens;
    int64_t pid = newPlayer("g-1", "Tokens");
    for (int i = 0; i < 50; ++i) {
        std::string t, err;
        int64_t exp = 0;
        CHECK_TRUE(store->mintSession(pid, "agent", t, exp, err));
        CHECK_EQUAL(43, static_cast<int>(t.size()));
        std::string raw;
        CHECK_TRUE(octi::base64UrlDecode(t, raw));
        CHECK_EQUAL(32, static_cast<int>(raw.size()));
        CHECK_TRUE(tokens.insert(t).second);
    }
    std::string t, err;
    int64_t exp = 0;
    CHECK_FALSE(store->mintSession(123456, "agent", t, exp, err));
}

TEST(AccountStoreTest, SessionExpiryBoundary)
{
    store->setSessionTtlSec(1000);
    LoginResult r = login("google", "g-1");
    CHECK_EQUAL(kNow + 1000, r.sessionExpiresAt);
    octi::SessionValidation v = store->validateSession(r.sessionToken, kNow + 999);
    CHECK_TRUE(v.valid);
    v = store->validateSession(r.sessionToken, kNow + 1000);
    CHECK_FALSE(v.valid);
    STRCMP_EQUAL("expired", v.reason.c_str());
    clockNow = kNow + 5000;
    CHECK_FALSE(store->validateSession(r.sessionToken).valid);
    clockNow = kNow;
    CHECK_TRUE(store->validateSession(r.sessionToken).valid);
}

TEST(AccountStoreTest, SessionFaultsUnknownMalformedAndRevoked)
{
    LoginResult r = login("google", "g-1");
    /* Tokens are random: pick a first character that differs from the real one. */
    const char swapped = r.sessionToken[0] == 'A' ? 'B' : 'A';
    octi::SessionValidation v =
        store->validateSession(std::string(1, swapped) + r.sessionToken.substr(1));
    CHECK_FALSE(v.valid);
    STRCMP_EQUAL("unknown", v.reason.c_str());
    v = store->validateSession("");
    STRCMP_EQUAL("malformed", v.reason.c_str());
    v = store->validateSession("has space");
    STRCMP_EQUAL("malformed", v.reason.c_str());
    v = store->validateSession(std::string(300, 'a'));
    STRCMP_EQUAL("malformed", v.reason.c_str());
    v = store->validateSession(AccountStore::sha256Hex(r.sessionToken));
    CHECK_FALSE(v.valid);   // presenting the stored hash must not authenticate

    CHECK_TRUE(store->revokeSession(r.sessionToken));
    v = store->validateSession(r.sessionToken);
    CHECK_FALSE(v.valid);
    STRCMP_EQUAL("revoked", v.reason.c_str());
    CHECK_TRUE(store->revokeSession(r.sessionToken));   // idempotent
    CHECK_FALSE(store->revokeSession("never-issued-token"));
    CHECK_FALSE(store->revokeSession(""));
}

TEST(AccountStoreTest, RevokeAllSessionsAndPurge)
{
    LoginResult a1 = login("google", "g-1");
    LoginResult a2 = login("google", "g-1");
    LoginResult b = login("apple", "a-1");
    CHECK_EQUAL(2, store->revokeAllSessions(a1.profile.playerId));
    CHECK_EQUAL(0, store->revokeAllSessions(a1.profile.playerId));
    CHECK_FALSE(store->validateSession(a1.sessionToken).valid);
    CHECK_FALSE(store->validateSession(a2.sessionToken).valid);
    CHECK_TRUE(store->validateSession(b.sessionToken).valid);

    CHECK_EQUAL(2, store->purgeExpiredSessions(kNow));
    CHECK_EQUAL(1, countRows("SELECT COUNT(*) FROM sessions"));
    CHECK_EQUAL(1, store->purgeExpiredSessions(kNow + AccountStore::kDefaultSessionTtlSec));
    CHECK_EQUAL(0, countRows("SELECT COUNT(*) FROM sessions"));
}

TEST(AccountStoreTest, CountActiveSessionsIgnoresRevokedExpiredAndOthers)
{
    LoginResult a1 = login("google", "g-1");
    LoginResult a2 = login("google", "g-1");
    LoginResult b = login("apple", "a-1");
    int64_t a = a1.profile.playerId;
    CHECK_EQUAL(2, store->countActiveSessions(a));
    CHECK_EQUAL(1, store->countActiveSessions(b.profile.playerId));
    CHECK_EQUAL(0, store->countActiveSessions(999999));

    CHECK_TRUE(store->revokeSession(a1.sessionToken));
    CHECK_EQUAL(1, store->countActiveSessions(a));
    CHECK_TRUE(store->validateSession(a2.sessionToken).valid);

    /* The expiry boundary matches validateSession: now >= expires_at. */
    clockNow = a2.sessionExpiresAt - 1;
    CHECK_EQUAL(1, store->countActiveSessions(a));
    clockNow = a2.sessionExpiresAt;
    CHECK_EQUAL(0, store->countActiveSessions(a));
    CHECK_FALSE(store->validateSession(a2.sessionToken, clockNow).valid);
}

TEST(AccountStoreTest, ClosedStoreFailsGracefully)
{
    store->close();
    PlayerProfile p;
    std::vector<octi::LeaderboardEntry> lb;
    std::vector<GameRecord> games;
    std::string t, err;
    int64_t exp = 0;
    CHECK_FALSE(store->getProfileById(1, p));
    CHECK_FALSE(store->leaderboard("2P", 10, lb));
    CHECK_FALSE(store->listArchivedGames(octi::ArchiveFilter(), games));
    CHECK_FALSE(store->mintSession(1, "a", t, exp, err));
    CHECK_FALSE(store->validateSession("abc").valid);
    CHECK_EQUAL(-1, store->revokeAllSessions(1));
    CHECK_EQUAL(-1, store->countActiveSessions(1));
    STRCMP_EQUAL("ERROR", unlinkName(store->unlinkIdentity(1, "google")));
    STRCMP_EQUAL("ERROR", archiveName(store->archiveGame(record("g-1", kNow, 0, 0), err)));
}

/* ---- Ratings & leaderboard ---- */

TEST(AccountStoreTest, LeaderboardOrdersByEloThenGamesThenAge)
{
    int64_t a = newPlayer("g-A", "Aaa");
    int64_t b = newPlayer("g-B", "Bbb");
    int64_t c = newPlayer("g-C", "Ccc");
    int64_t d = newPlayer("g-D", "Ddd");
    int64_t e = newPlayer("g-E", "Eee");
    std::string err;
    octi::RatingUpdate x;
    x.playerId = a;
    x.newElo = 1300;
    x.outcome = GameOutcome::WIN;
    octi::RatingUpdate y;
    y.playerId = b;
    y.newElo = 1250;
    y.outcome = GameOutcome::LOSS;
    std::vector<octi::RatingUpdate> up;
    up.push_back(x);
    up.push_back(y);
    CHECK_TRUE_TEXT(store->applyRatingUpdates("2P", up, err), err.c_str());

    up.clear();
    x.playerId = c;
    x.newElo = 1250;
    y.playerId = d;
    y.newElo = 1150;
    up.push_back(x);
    up.push_back(y);
    CHECK_TRUE(store->applyRatingUpdates("2P", up, err));

    up.clear();
    x.playerId = c;
    x.newElo = 1250;
    x.outcome = GameOutcome::DRAW;
    y.playerId = b;
    y.newElo = 1250;
    y.outcome = GameOutcome::DRAW;
    up.push_back(x);
    up.push_back(y);
    CHECK_TRUE(store->applyRatingUpdates("2P", up, err));

    up.clear();
    x.playerId = c;
    x.newElo = 1250;
    x.outcome = GameOutcome::WIN;
    y.playerId = e;
    y.newElo = 1100;
    y.outcome = GameOutcome::LOSS;
    up.push_back(x);
    up.push_back(y);
    CHECK_TRUE(store->applyRatingUpdates("2P", up, err));

    std::vector<octi::LeaderboardEntry> lb;
    CHECK_TRUE(store->leaderboard("2P", 10, lb));
    CHECK_EQUAL(5, static_cast<int>(lb.size()));
    /* A 1300; C 1250 (3 games) before B 1250 (2 games); D 1150; E 1100 */
    STRCMP_EQUAL("Aaa", lb[0].handle.c_str());
    STRCMP_EQUAL("Ccc", lb[1].handle.c_str());
    STRCMP_EQUAL("Bbb", lb[2].handle.c_str());
    STRCMP_EQUAL("Ddd", lb[3].handle.c_str());
    STRCMP_EQUAL("Eee", lb[4].handle.c_str());
    for (int i = 0; i < 5; ++i) {
        CHECK_EQUAL(i + 1, lb[static_cast<size_t>(i)].rank);
    }
    CHECK_EQUAL(1300, lb[0].elo);
    CHECK_EQUAL(3, lb[1].gamesPlayed);
    CHECK_EQUAL(2, lb[1].wins);
    CHECK_EQUAL(0, lb[1].losses);
    CHECK_EQUAL(1, lb[1].draws);
    CHECK_EQUAL(2, lb[2].gamesPlayed);
    CHECK_EQUAL(1, lb[2].losses);
    CHECK_EQUAL(1, lb[2].draws);
    CHECK_FALSE(lb[0].uuid.empty());

    /* other modes are independent: all still 1200, ordered by games then age */
    CHECK_TRUE(store->leaderboard("4P", 10, lb));
    CHECK_EQUAL(5, static_cast<int>(lb.size()));
    for (size_t i = 0; i < lb.size(); ++i) {
        CHECK_EQUAL(1200, lb[i].elo);
    }
    STRCMP_EQUAL("Ccc", lb[0].handle.c_str());
    CHECK_TRUE(store->leaderboard("OFK", 10, lb));
    CHECK_EQUAL(1200, lb[0].elo);
}

TEST(AccountStoreTest, LeaderboardLimitClampingAndInvalidMode)
{
    for (int i = 0; i < 3; ++i) {
        newPlayer("g-" + std::to_string(i), "Player" + std::to_string(i));
    }
    std::vector<octi::LeaderboardEntry> lb;
    CHECK_TRUE(store->leaderboard("2P", 2, lb));
    CHECK_EQUAL(2, static_cast<int>(lb.size()));
    CHECK_TRUE(store->leaderboard("2P", 0, lb));
    CHECK_EQUAL(1, static_cast<int>(lb.size()));
    CHECK_TRUE(store->leaderboard("2P", -5, lb));
    CHECK_EQUAL(1, static_cast<int>(lb.size()));
    CHECK_TRUE(store->leaderboard("2P", 100000, lb));
    CHECK_EQUAL(3, static_cast<int>(lb.size()));
    CHECK_FALSE(store->leaderboard("3P", 10, lb));
    CHECK_FALSE(store->leaderboard("", 10, lb));
    CHECK_FALSE(store->leaderboard("2P; DROP TABLE players", 10, lb));
    CHECK_EQUAL(3, countRows("SELECT COUNT(*) FROM players"));
}

TEST(AccountStoreTest, RatingUpdatesAreAtomic)
{
    int64_t a = newPlayer("g-A", "Atomic A");
    int64_t b = newPlayer("g-B", "Atomic B");
    std::string err;
    octi::RatingUpdate ok;
    ok.playerId = a;
    ok.newElo = 1500;
    ok.outcome = GameOutcome::WIN;
    octi::RatingUpdate ghost;
    ghost.playerId = 987654;
    ghost.newElo = 900;
    ghost.outcome = GameOutcome::LOSS;
    std::vector<octi::RatingUpdate> up;
    up.push_back(ok);
    up.push_back(ghost);
    CHECK_FALSE(store->applyRatingUpdates("2P", up, err));
    CHECK_FALSE(err.empty());
    PlayerProfile p;
    CHECK_TRUE(store->getProfileById(a, p));
    CHECK_EQUAL(1200, p.elo2p);
    CHECK_EQUAL(0, p.gamesPlayed);
    CHECK_EQUAL(0, p.wins);

    /* duplicate player, out-of-range rating and bad mode are refused */
    octi::RatingUpdate dup = ok;
    up.clear();
    up.push_back(ok);
    up.push_back(dup);
    CHECK_FALSE(store->applyRatingUpdates("2P", up, err));
    octi::RatingUpdate huge = ok;
    huge.newElo = 10001;
    up.clear();
    up.push_back(huge);
    CHECK_FALSE(store->applyRatingUpdates("2P", up, err));
    huge.newElo = -1;
    up.clear();
    up.push_back(huge);
    CHECK_FALSE(store->applyRatingUpdates("2P", up, err));
    up.clear();
    up.push_back(ok);
    CHECK_FALSE(store->applyRatingUpdates("5P", up, err));
    CHECK_TRUE(store->getProfileById(a, p));
    CHECK_EQUAL(0, p.gamesPlayed);

    octi::RatingUpdate other;
    other.playerId = b;
    other.newElo = 1100;
    other.outcome = GameOutcome::DRAW;
    up.clear();
    up.push_back(ok);
    up.push_back(other);
    CHECK_TRUE(store->applyRatingUpdates("OFK", up, err));
    CHECK_TRUE(store->getProfileById(a, p));
    CHECK_EQUAL(1500, p.eloOfk);
    CHECK_EQUAL(1200, p.elo2p);
    CHECK_EQUAL(1, p.wins);
    CHECK_TRUE(store->getProfileById(b, p));
    CHECK_EQUAL(1100, p.eloOfk);
    CHECK_EQUAL(1, p.draws);
    CHECK_TRUE(store->applyRatingUpdates("4P", std::vector<octi::RatingUpdate>(), err));
}

/* ---- Archive ---- */

TEST(AccountStoreTest, ArchiveAndQueryOctiTranscript)
{
    int64_t a = newPlayer("g-A", "Alpha");
    int64_t b = newPlayer("g-B", "Beta");
    GameRecord g = record("g-20261022-0001", kNow, a, b);
    std::string err;
    STRCMP_EQUAL("OK", archiveName(store->archiveGame(g, err)));

    GameRecord got;
    CHECK_TRUE(store->getArchivedGame(g.gameId, got));
    STRCMP_EQUAL(g.gameId.c_str(), got.gameId.c_str());
    STRCMP_EQUAL("2P", got.gameMode.c_str());
    CHECK_EQUAL(3, got.gameBases);
    CHECK_FALSE(got.superProngs);
    CHECK_FALSE(got.edgeless);
    STRCMP_EQUAL("600+5", got.timeControl.c_str());
    CHECK_EQUAL(a, got.p1PlayerId);
    CHECK_EQUAL(b, got.p2PlayerId);
    CHECK_EQUAL(0, static_cast<int>(got.p3PlayerId));
    CHECK_EQUAL(0, static_cast<int>(got.p4PlayerId));
    STRCMP_EQUAL("alpha", got.p1Handle.c_str());
    STRCMP_EQUAL("P1_WIN", got.result.c_str());
    CHECK_EQUAL(1, got.winnerSeat);
    STRCMP_EQUAL("RESIGNATION", got.termination.c_str());
    CHECK_EQUAL(4, got.plyCount);
    CHECK_TRUE(got.octiTranscript == g.octiTranscript);
    STRCMP_EQUAL(g.finalFen.c_str(), got.finalFen.c_str());
    CHECK_EQUAL(kNow - 300, got.startedAt);
    CHECK_EQUAL(kNow, got.endedAt);

    /* the stored transcript is a real JSON v3.0 transcript that replays to the stored FEN */
    CHECK_TRUE(got.octiTranscript.find("\"octi.transcript.v3\"") !=
               std::string::npos);
    octi::OctiBoard replay;
    replay.deserializeGame(got.octiTranscript);
    CHECK_EQUAL(4, replay.getGame()->getNumMoves());
    STRCMP_EQUAL(got.finalFen.c_str(), replay.toPositionString().c_str());

    std::string transcript;
    CHECK_TRUE(store->getArchivedTranscript(g.gameId, transcript));
    CHECK_TRUE(transcript == g.octiTranscript);
    CHECK_FALSE(store->getArchivedTranscript("g-missing", transcript));
    CHECK_FALSE(store->getArchivedGame("g-missing", got));
    CHECK_FALSE(store->getArchivedGame("' OR 1=1 --", got));
}

TEST(AccountStoreTest, ArchiveListingFiltersOrderingAndPaging)
{
    int64_t a = newPlayer("g-A", "Alpha");
    int64_t b = newPlayer("g-B", "Beta");
    int64_t c = newPlayer("g-C", "Gamma");
    std::string err;
    for (int i = 0; i < 7; ++i) {
        GameRecord g = record("g-" + std::to_string(i), kNow + i * 10,
                              (i % 2 == 0) ? a : b, c,
                              (i < 5) ? "2P" : "OFK");
        STRCMP_EQUAL("OK", archiveName(store->archiveGame(g, err)));
    }
    octi::ArchiveFilter f;
    f.limit = 3;
    std::vector<GameRecord> page;
    int64_t total = 0;
    CHECK_TRUE(store->listArchivedGames(f, page, &total));
    CHECK_EQUAL(7, static_cast<int>(total));
    CHECK_EQUAL(3, static_cast<int>(page.size()));
    STRCMP_EQUAL("g-6", page[0].gameId.c_str());
    STRCMP_EQUAL("g-5", page[1].gameId.c_str());
    STRCMP_EQUAL("g-4", page[2].gameId.c_str());
    CHECK_TRUE(page[0].octiTranscript.empty());
    CHECK_FALSE(page[0].finalFen.empty());

    f.offset = 6;
    CHECK_TRUE(store->listArchivedGames(f, page, &total));
    CHECK_EQUAL(1, static_cast<int>(page.size()));
    STRCMP_EQUAL("g-0", page[0].gameId.c_str());
    f.offset = 100;
    CHECK_TRUE(store->listArchivedGames(f, page, &total));
    CHECK_EQUAL(0, static_cast<int>(page.size()));
    CHECK_EQUAL(7, static_cast<int>(total));

    f = octi::ArchiveFilter();
    f.mode = "OFK";
    CHECK_TRUE(store->listArchivedGames(f, page, &total));
    CHECK_EQUAL(2, static_cast<int>(total));
    CHECK_EQUAL(2, static_cast<int>(page.size()));

    PlayerProfile pa;
    CHECK_TRUE(store->getProfileById(a, pa));
    f = octi::ArchiveFilter();
    f.playerUuid = pa.uuid;
    CHECK_TRUE(store->listArchivedGames(f, page, &total));
    CHECK_EQUAL(4, static_cast<int>(total));   // games 0, 2, 4, 6
    for (size_t i = 0; i < page.size(); ++i) {
        CHECK_EQUAL(a, page[i].p1PlayerId);
    }
    /* seat 2 player is found as well */
    PlayerProfile pc;
    CHECK_TRUE(store->getProfileById(c, pc));
    f.playerUuid = pc.uuid;
    CHECK_TRUE(store->listArchivedGames(f, page, &total));
    CHECK_EQUAL(7, static_cast<int>(total));
    f.mode = "2P";
    CHECK_TRUE(store->listArchivedGames(f, page, &total));
    CHECK_EQUAL(5, static_cast<int>(total));

    f = octi::ArchiveFilter();
    f.playerUuid = "00000000-0000-4000-8000-000000000000";
    CHECK_TRUE(store->listArchivedGames(f, page, &total));
    CHECK_EQUAL(0, static_cast<int>(page.size()));
    CHECK_EQUAL(0, static_cast<int>(total));

    f = octi::ArchiveFilter();
    f.mode = "9P";
    CHECK_FALSE(store->listArchivedGames(f, page, &total));
    f.mode = "' OR 1=1 --";
    CHECK_FALSE(store->listArchivedGames(f, page, &total));
}

TEST(AccountStoreTest, ArchiveLimitAndOffsetAreClamped)
{
    int64_t a = newPlayer("g-A", "Alpha");
    int64_t b = newPlayer("g-B", "Beta");
    std::string err;
    for (int i = 0; i < 105; ++i) {
        STRCMP_EQUAL("OK", archiveName(store->archiveGame(
                               record("g-" + std::to_string(1000 + i), kNow + i, a, b),
                               err)));
    }
    octi::ArchiveFilter f;
    std::vector<GameRecord> page;
    int64_t total = 0;
    f.limit = 100000;
    CHECK_TRUE(store->listArchivedGames(f, page, &total));
    CHECK_EQUAL(100, static_cast<int>(page.size()));
    CHECK_EQUAL(105, static_cast<int>(total));
    f.limit = 0;
    CHECK_TRUE(store->listArchivedGames(f, page, &total));
    CHECK_EQUAL(1, static_cast<int>(page.size()));
    f.limit = -3;
    f.offset = -10;
    CHECK_TRUE(store->listArchivedGames(f, page, &total));
    CHECK_EQUAL(1, static_cast<int>(page.size()));
    STRCMP_EQUAL("g-1104", page[0].gameId.c_str());
    f.limit = 100;
    f.offset = 2000000000;
    CHECK_TRUE(store->listArchivedGames(f, page, &total));
    CHECK_EQUAL(0, static_cast<int>(page.size()));
}

TEST(AccountStoreTest, ArchiveRejectsInvalidRecordsAndDuplicates)
{
    int64_t a = newPlayer("g-A", "Alpha");
    int64_t b = newPlayer("g-B", "Beta");
    std::string err;
    GameRecord good = record("g-ok", kNow, a, b);
    STRCMP_EQUAL("OK", archiveName(store->archiveGame(good, err)));
    STRCMP_EQUAL("DUPLICATE_ID", archiveName(store->archiveGame(good, err)));
    CHECK_EQUAL(1, countRows("SELECT COUNT(*) FROM games"));

    GameRecord g = good;
    g.gameId = "g-bad-mode";
    g.gameMode = "3P";
    STRCMP_EQUAL("INVALID", archiveName(store->archiveGame(g, err)));
    g = good;
    g.gameId = "g-bad-bases";
    g.gameBases = 2;
    STRCMP_EQUAL("INVALID", archiveName(store->archiveGame(g, err)));
    g = good;
    g.gameId = "g-bad-result";
    g.result = "P3_WIN";
    STRCMP_EQUAL("INVALID", archiveName(store->archiveGame(g, err)));
    g = good;
    g.gameId = "g-bad-term";
    g.termination = "RAGE_QUIT";
    STRCMP_EQUAL("INVALID", archiveName(store->archiveGame(g, err)));
    g = good;
    g.gameId = "g-bad-seat";
    g.winnerSeat = 5;
    STRCMP_EQUAL("INVALID", archiveName(store->archiveGame(g, err)));
    g = good;
    g.gameId = "g-empty-transcript";
    g.octiTranscript = "";
    STRCMP_EQUAL("INVALID", archiveName(store->archiveGame(g, err)));
    g = good;
    g.gameId = "g-empty-fen";
    g.finalFen = "";
    STRCMP_EQUAL("INVALID", archiveName(store->archiveGame(g, err)));
    g = good;
    g.gameId = "g-time-travel";
    g.endedAt = g.startedAt - 1;
    STRCMP_EQUAL("INVALID", archiveName(store->archiveGame(g, err)));
    g = good;
    g.gameId = "../etc/passwd";
    STRCMP_EQUAL("INVALID", archiveName(store->archiveGame(g, err)));
    g = good;
    g.gameId = "";
    STRCMP_EQUAL("INVALID", archiveName(store->archiveGame(g, err)));
    g = good;
    g.gameId = "g-unknown-player";
    g.p1PlayerId = 424242;
    STRCMP_EQUAL("INVALID", archiveName(store->archiveGame(g, err)));
    g = good;
    g.gameId = "g-empty-handle";
    g.p2Handle = "";
    STRCMP_EQUAL("INVALID", archiveName(store->archiveGame(g, err)));
    CHECK_EQUAL(1, countRows("SELECT COUNT(*) FROM games"));

    /* transcript size boundary: 1 MiB accepted, one byte more refused */
    g = good;
    g.gameId = "g-max-transcript";
    g.octiTranscript = std::string(1048576, 'x');
    STRCMP_EQUAL("OK", archiveName(store->archiveGame(g, err)));
    g.gameId = "g-over-transcript";
    g.octiTranscript = std::string(1048577, 'x');
    STRCMP_EQUAL("INVALID", archiveName(store->archiveGame(g, err)));
    CHECK_EQUAL(2, countRows("SELECT COUNT(*) FROM games"));

    /* archived games without registered players (NULL player ids) */
    g = good;
    g.gameId = "g-anon";
    g.p1PlayerId = 0;
    g.p2PlayerId = 0;
    STRCMP_EQUAL("OK", archiveName(store->archiveGame(g, err)));
}

TEST(AccountStoreTest, ArchiveWithRatingsCommitsAtomically)
{
    int64_t a = newPlayer("g-A", "Alpha");
    int64_t b = newPlayer("g-B", "Beta");
    std::string err;
    octi::RatingUpdate w;
    w.playerId = a;
    w.newElo = 1216;
    w.outcome = GameOutcome::WIN;
    octi::RatingUpdate l;
    l.playerId = b;
    l.newElo = 1184;
    l.outcome = GameOutcome::LOSS;
    std::vector<octi::RatingUpdate> up;
    up.push_back(w);
    up.push_back(l);
    STRCMP_EQUAL("OK", archiveName(store->archiveGameWithRatings(
                           record("g-1", kNow, a, b), up, err)));
    PlayerProfile pa, pb;
    CHECK_TRUE(store->getProfileById(a, pa));
    CHECK_TRUE(store->getProfileById(b, pb));
    CHECK_EQUAL(1216, pa.elo2p);
    CHECK_EQUAL(1184, pb.elo2p);
    CHECK_EQUAL(1, pa.wins);
    CHECK_EQUAL(1, pb.losses);
    CHECK_EQUAL(1, countRows("SELECT COUNT(*) FROM games"));

    /* failing rating update (unknown player) rolls the archive row back too */
    octi::RatingUpdate ghost;
    ghost.playerId = 555555;
    ghost.newElo = 1000;
    ghost.outcome = GameOutcome::LOSS;
    up.clear();
    up.push_back(w);
    up.push_back(ghost);
    STRCMP_EQUAL("INVALID", archiveName(store->archiveGameWithRatings(
                                record("g-2", kNow + 1, a, b), up, err)));
    CHECK_EQUAL(1, countRows("SELECT COUNT(*) FROM games"));
    CHECK_TRUE(store->getProfileById(a, pa));
    CHECK_EQUAL(1, pa.gamesPlayed);

    /* duplicate game id must not apply ratings a second time */
    up.clear();
    up.push_back(w);
    up.push_back(l);
    STRCMP_EQUAL("DUPLICATE_ID", archiveName(store->archiveGameWithRatings(
                                     record("g-1", kNow, a, b), up, err)));
    CHECK_TRUE(store->getProfileById(a, pa));
    CHECK_EQUAL(1, pa.gamesPlayed);
}

/* ---- RapidCheck properties ---- */

TEST(AccountStoreTest, RapidCheckLinkUnlinkStateMachineInvariants)
{
    const std::vector<std::string> providers = allProviders();
    int iteration = 0;
    bool passed = rc::check("link/unlink keeps UNIQUE(provider,sub) and >=1 identity", [&]() {
        const std::string prefix = "it" + std::to_string(++iteration) + "-";
        const int kPlayers = 3;
        int64_t pid[kPlayers];
        std::map<std::pair<std::string, std::string>, int> owner;
        for (int i = 0; i < kPlayers; ++i) {
            std::string sub = prefix + "root" + std::to_string(i);
            LoginResult r = store->loginWithSso(makeClaims("google", sub, "SM Player"), "t");
            RC_ASSERT(r.ok);
            pid[i] = r.profile.playerId;
            owner[std::make_pair(std::string("google"), sub)] = i;
        }
        const int nOps = *rc::gen::inRange(1, 30);
        for (int k = 0; k < nOps; ++k) {
            const bool doLink = *rc::gen::arbitrary<bool>();
            const int who = *rc::gen::inRange(0, kPlayers);
            const std::string &prov =
                providers[static_cast<size_t>(*rc::gen::inRange(0, 5))];
            if (doLink) {
                std::string sub = prefix + "s" + std::to_string(*rc::gen::inRange(0, 3));
                std::pair<std::string, std::string> key(prov, sub);
                std::map<std::pair<std::string, std::string>, int>::iterator it =
                    owner.find(key);
                LinkStatus expected = LinkStatus::LINKED;
                if (it != owner.end()) {
                    expected = (it->second == who) ? LinkStatus::ALREADY_LINKED_SELF
                                                   : LinkStatus::CONFLICT_ALREADY_LINKED;
                }
                octi::LinkResult lr = store->linkIdentity(pid[who], makeClaims(prov, sub));
                RC_ASSERT(lr.status == expected);
                if (expected == LinkStatus::LINKED) {
                    owner[key] = who;
                }
            } else {
                int matching = 0;
                int total = 0;
                for (std::map<std::pair<std::string, std::string>, int>::iterator it =
                         owner.begin();
                     it != owner.end(); ++it) {
                    if (it->second == who) {
                        ++total;
                        if (it->first.first == prov) {
                            ++matching;
                        }
                    }
                }
                UnlinkStatus expected = UnlinkStatus::UNLINKED;
                if (matching == 0) {
                    expected = UnlinkStatus::NOT_LINKED;
                } else if (total - matching <= 0) {
                    expected = UnlinkStatus::LAST_IDENTITY_FORBIDDEN;
                }
                UnlinkStatus got = store->unlinkIdentity(pid[who], prov);
                RC_ASSERT(got == expected);
                if (expected == UnlinkStatus::UNLINKED) {
                    for (std::map<std::pair<std::string, std::string>, int>::iterator it =
                             owner.begin();
                         it != owner.end();) {
                        if (it->second == who && it->first.first == prov) {
                            owner.erase(it++);
                        } else {
                            ++it;
                        }
                    }
                }
            }
            /* invariants: every player reachable, store matches the model */
            std::set<std::pair<std::string, std::string>> seenKeys;
            for (int i = 0; i < kPlayers; ++i) {
                std::vector<octi::LinkedIdentity> ids;
                RC_ASSERT(store->listLinkedIdentities(pid[i], ids));
                RC_ASSERT(!ids.empty());
                for (size_t j = 0; j < ids.size(); ++j) {
                    std::pair<std::string, std::string> key(ids[j].provider,
                                                            ids[j].providerSub);
                    RC_ASSERT(seenKeys.insert(key).second);
                    std::map<std::pair<std::string, std::string>, int>::iterator it =
                        owner.find(key);
                    RC_ASSERT(it != owner.end());
                    RC_ASSERT(it->second == i);
                }
            }
            RC_ASSERT(seenKeys.size() == owner.size());
        }
    });
    CHECK_TRUE(passed);
}

TEST(AccountStoreTest, RapidCheckArbitraryNamesYieldValidUniqueHandles)
{
    int iteration = 0;
    bool passed = rc::check("any provider name maps to a valid unique handle", [&]() {
        std::string name = *rc::gen::arbitrary<std::string>();
        std::string email = *rc::gen::arbitrary<std::string>();
        VerifiedClaims c = makeClaims("google", "hn-" + std::to_string(++iteration), name, email);
        c.picture = *rc::gen::arbitrary<std::string>();
        LoginResult r = store->loginWithSso(c, *rc::gen::arbitrary<std::string>());
        RC_ASSERT(r.ok);
        RC_ASSERT(AccountStore::isValidHandle(r.profile.handle));
        RC_ASSERT(!r.profile.displayName.empty());
        RC_ASSERT(r.profile.displayName.size() <= 64);
        RC_ASSERT(r.profile.avatarUrl.empty() ||
                  r.profile.avatarUrl.compare(0, 8, "https://") == 0);
        PlayerProfile p;
        RC_ASSERT(store->getProfileById(r.profile.playerId, p));
        RC_ASSERT(p.handle == r.profile.handle);
        RC_ASSERT(store->validateSession(r.sessionToken).valid);
    });
    CHECK_TRUE(passed);
}

TEST(AccountStoreTest, LiveRoomCrudAndListing)
{
    std::string err;
    std::vector<octi::LiveRoomRow> rooms;
    CHECK_TRUE(store->listLiveRooms(rooms, err));
    LONGS_EQUAL(0, rooms.size());

    clockNow = 1000;
    CHECK_TRUE(store->upsertLiveRoom("room-c", "{\"state\":\"c\"}", err));
    CHECK_TRUE(store->upsertLiveRoom("room-a", "{\"state\":\"a\"}", err));
    CHECK_TRUE(store->upsertLiveRoom("room-b", "{\"state\":\"b\"}", err));

    CHECK_TRUE(store->listLiveRooms(rooms, err));
    LONGS_EQUAL(3, rooms.size());
    STRCMP_EQUAL("room-a", rooms[0].roomId.c_str());
    STRCMP_EQUAL("{\"state\":\"a\"}", rooms[0].stateJson.c_str());
    LONGS_EQUAL(1000, rooms[0].updatedAt);
    STRCMP_EQUAL("room-b", rooms[1].roomId.c_str());
    STRCMP_EQUAL("room-c", rooms[2].roomId.c_str());

    /* Updating room-b with a later timestamp overwrites existing row */
    clockNow = 2500;
    CHECK_TRUE(store->upsertLiveRoom("room-b", "{\"state\":\"b2\"}", err));
    CHECK_TRUE(store->listLiveRooms(rooms, err));
    LONGS_EQUAL(3, rooms.size());
    STRCMP_EQUAL("room-b", rooms[1].roomId.c_str());
    STRCMP_EQUAL("{\"state\":\"b2\"}", rooms[1].stateJson.c_str());
    LONGS_EQUAL(2500, rooms[1].updatedAt);

    /* Deletion */
    CHECK_TRUE(store->deleteLiveRoom("room-a", err));
    CHECK_TRUE(store->listLiveRooms(rooms, err));
    LONGS_EQUAL(2, rooms.size());
    STRCMP_EQUAL("room-b", rooms[0].roomId.c_str());
    STRCMP_EQUAL("room-c", rooms[1].roomId.c_str());

    /* Idempotent delete on non-existent room returns true */
    CHECK_TRUE(store->deleteLiveRoom("room-nonexistent", err));
    CHECK_TRUE(store->listLiveRooms(rooms, err));
    LONGS_EQUAL(2, rooms.size());
}

TEST(AccountStoreTest, LiveMetaSetAndGet)
{
    std::string err;
    std::string meta;

    /* Non-existent key returns true with empty string */
    CHECK_TRUE(store->getLiveMeta("lobby_state", meta, err));
    STRCMP_EQUAL("", meta.c_str());

    /* Set and get */
    CHECK_TRUE(store->setLiveMeta("lobby_state", "{\"seq\":42}", err));
    CHECK_TRUE(store->getLiveMeta("lobby_state", meta, err));
    STRCMP_EQUAL("{\"seq\":42}", meta.c_str());

    /* Overwrite */
    CHECK_TRUE(store->setLiveMeta("lobby_state", "{\"seq\":43}", err));
    CHECK_TRUE(store->getLiveMeta("lobby_state", meta, err));
    STRCMP_EQUAL("{\"seq\":43}", meta.c_str());
}

TEST(AccountStoreTest, LivePersistenceBoundaryAndFaults)
{
    std::string err;

    /* Boundary limits */
    std::string valid64Key(64, 'k');
    std::string tooLongKey(65, 'k');
    CHECK_TRUE(store->upsertLiveRoom(valid64Key, "{}", err));
    CHECK_FALSE(store->upsertLiveRoom(tooLongKey, "{}", err));
    CHECK_FALSE(store->upsertLiveRoom("", "{}", err));
    CHECK_FALSE(store->upsertLiveRoom("valid-id", "", err));

    std::string maxJson(1024 * 1024, 'x');
    CHECK_TRUE(store->upsertLiveRoom("max-room", maxJson, err));
    std::string oversizedJson((1024 * 1024) + 1, 'x');
    CHECK_FALSE(store->upsertLiveRoom("oversized-room", oversizedJson, err));

    CHECK_FALSE(store->deleteLiveRoom("", err));
    CHECK_FALSE(store->deleteLiveRoom(tooLongKey, err));

    /* Meta boundaries */
    CHECK_TRUE(store->setLiveMeta(valid64Key, "{}", err));
    CHECK_FALSE(store->setLiveMeta(tooLongKey, "{}", err));
    CHECK_FALSE(store->setLiveMeta("", "{}", err));
    CHECK_FALSE(store->setLiveMeta("valid-meta", "", err));
    CHECK_TRUE(store->setLiveMeta("max-meta", maxJson, err));
    CHECK_FALSE(store->setLiveMeta("oversized-meta", oversizedJson, err));

    CHECK_FALSE(store->getLiveMeta("", maxJson, err));
    CHECK_FALSE(store->getLiveMeta(tooLongKey, maxJson, err));

    /* Fault: closed database */
    AccountStore closedStore;
    std::vector<octi::LiveRoomRow> rows;
    CHECK_FALSE(closedStore.upsertLiveRoom("r", "{}", err));
    CHECK_FALSE(closedStore.deleteLiveRoom("r", err));
    CHECK_FALSE(closedStore.listLiveRooms(rows, err));
    CHECK_FALSE(closedStore.setLiveMeta("m", "{}", err));
    CHECK_FALSE(closedStore.getLiveMeta("m", maxJson, err));
}

TEST(AccountStoreTest, LegacyV207GamesSchemaMigration)
{
    std::string legacyPath = dbPath() + "-legacy-v207.db";
    ::unlink(legacyPath.c_str());

    sqlite3 *raw = nullptr;
    CHECK_EQUAL(SQLITE_OK, sqlite3_open(legacyPath.c_str(), &raw));
    const char *legacySql =
        "CREATE TABLE games ("
        " game_id TEXT PRIMARY KEY,"
        " game_mode TEXT NOT NULL,"
        " game_bases INTEGER NOT NULL,"
        " super_prongs INTEGER NOT NULL,"
        " edgeless INTEGER NOT NULL,"
        " time_control TEXT NOT NULL DEFAULT 'untimed',"
        " p1_player_id INTEGER,"
        " p2_player_id INTEGER,"
        " p3_player_id INTEGER,"
        " p4_player_id INTEGER,"
        " p1_handle TEXT NOT NULL,"
        " p2_handle TEXT NOT NULL,"
        " p3_handle TEXT NOT NULL DEFAULT '',"
        " p4_handle TEXT NOT NULL DEFAULT '',"
        " result TEXT NOT NULL,"
        " winner_seat INTEGER NOT NULL DEFAULT 0,"
        " termination TEXT NOT NULL,"
        " ply_count INTEGER NOT NULL,"
        " octi_transcript TEXT NOT NULL,"
        " final_fen TEXT NOT NULL,"
        " started_at INTEGER NOT NULL,"
        " ended_at INTEGER NOT NULL"
        ");"
        "INSERT INTO games VALUES ("
        " 'g-legacy-1', '2P', 1, 0, 0, 'untimed', NULL, NULL, NULL, NULL,"
        " 'Alice', 'Bob', '', '', 'P1_WIN', 1, 'BASE_CAPTURE', 2,"
        " '{\"format\":\"octi.transcript.v3\"}', 'fen', 1000, 1010"
        ");";
    CHECK_EQUAL(SQLITE_OK, sqlite3_exec(raw, legacySql, nullptr, nullptr, nullptr));
    sqlite3_close(raw);

    AccountStore migrated;
    std::string err;
    CHECK_TEXT(migrated.openOrCreate(legacyPath, err), err.c_str());

    octi::GameRecord rec;
    CHECK_TRUE(migrated.getArchivedGame("g-legacy-1", rec));
    STRCMP_EQUAL("online", rec.source.c_str());
    STRCMP_EQUAL("{\"format\":\"octi.transcript.v3\"}", rec.octiTranscript.c_str());

    migrated.close();
    ::unlink(legacyPath.c_str());
    ::unlink((legacyPath + "-wal").c_str());
    ::unlink((legacyPath + "-shm").c_str());
}

int main(int argc, char **argv)
{
    MemoryLeakWarningPlugin::turnOffNewDeleteOverloads();
    return CommandLineTestRunner::RunAllTests(argc, argv);
}

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
