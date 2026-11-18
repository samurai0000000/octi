/*
 * TestAccountAndOidc.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "AccountStore.hxx"
#include "BanManager.hxx"
#include "EloCalculator.hxx"
#include "OctiBoard.hxx"
#include "OctiGame.hxx"
#include "OctiHttpServer.hxx"
#include "OidcVerifier.hxx"

#include <nlohmann/json.hpp>
#include <openssl/bn.h>
#include <openssl/core_names.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <rapidcheck.h>
#include <sqlite3.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
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
using octi::ArchiveFilter;
using octi::ArchiveStatus;
using octi::CrashReportFilter;
using octi::CrashReportRecord;
using octi::CreateRobotResult;
using octi::GameOutcome;
using octi::GameRecord;
using octi::LinkResult;
using octi::LinkStatus;
using octi::LobbyChatLogRow;
using octi::LoginResult;
using octi::OidcVerifier;
using octi::PlayerProfile;
using octi::ProfileUpdate;
using octi::RobotRecord;
using octi::RobotStatus;
using octi::RobotTokenValidation;
using octi::UnlinkStatus;
using octi::UpdateStatus;
using octi::UserDirectoryEntry;
using octi::UserDirectoryFilter;
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
    c["email_verified"] = true;
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
    case UpdateStatus::HANDLE_RATE_LIMITED:
        return "HANDLE_RATE_LIMITED";
    case UpdateStatus::INVALID_DISPLAY_NAME:
        return "INVALID_DISPLAY_NAME";
    case UpdateStatus::INVALID_AVATAR_URL:
        return "INVALID_AVATAR_URL";
    case UpdateStatus::INVALID_BIO:
        return "INVALID_BIO";
    case UpdateStatus::INVALID_LOCATION:
        return "INVALID_LOCATION";
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
    c.emailVerified = !email.empty();
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
    CHECK_EQUAL(27, countRows("SELECT COUNT(*) FROM pragma_table_info('players')"));
    CHECK_EQUAL(4, countRows("SELECT COUNT(*) FROM pragma_table_info('player_installs')"));
    CHECK_EQUAL(3, countRows("SELECT COUNT(*) FROM pragma_table_info('install_tokens')"));
    CHECK_EQUAL(9, countRows(
                       "SELECT COUNT(*) FROM pragma_table_info('linked_identities')"));
    CHECK_EQUAL(7, countRows("SELECT COUNT(*) FROM pragma_table_info('sessions')"));
    CHECK_EQUAL(37, countRows("SELECT COUNT(*) FROM pragma_table_info('games')"));
    CHECK_EQUAL(24, countRows("SELECT COUNT(*) FROM pragma_table_info('robots')"));
    CHECK_TRUE(countRows("SELECT COUNT(*) FROM sqlite_master WHERE type='index'"
                         " AND name='idx_games_ended_at'") == 1);
    CHECK_TRUE(countRows("SELECT COUNT(*) FROM sqlite_master WHERE type='index'"
                         " AND name='idx_games_elo_pool_ended'") == 1);

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
        clockNow += 86400;
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
    clockNow += 86400;
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

    /* 4P is casual-only (not an official rated ladder); 2P_1B and OFK remain 1200 */
    CHECK_FALSE(store->leaderboard("4P", 10, lb));
    CHECK_TRUE(store->leaderboard("2P_3B", 10, lb));
    CHECK_EQUAL(1300, lb[0].elo);
    CHECK_TRUE(store->leaderboard("2P_1B", 10, lb));
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
    CHECK_FALSE(store->applyRatingUpdates("4P", std::vector<octi::RatingUpdate>(), err));
    CHECK_TRUE(store->applyRatingUpdates("2P_1B", std::vector<octi::RatingUpdate>(), err));
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
    CHECK_TRUE(got.octiTranscript.find('\n') == std::string::npos);
    CHECK_TRUE(got.octiTranscript.find('\r') == std::string::npos);
    CHECK_TRUE(got.octiTranscript.find("\"license\":\"CC-BY-4.0\"") !=
               std::string::npos);
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
    CHECK_TRUE(transcript == got.octiTranscript);
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

TEST(AccountStoreTest, BioLocationAndDataUriAvatar)
{
    int64_t pid = newPlayer("sub-bio-loc", "Bio Player");
    PlayerProfile prof;
    CHECK_TRUE(store->getProfile(pid, prof));
    STRCMP_EQUAL("", prof.bio.c_str());
    STRCMP_EQUAL("", prof.location.c_str());

    // Validation rules
    CHECK_TRUE(AccountStore::validBio("Hello, world! I love Octi."));
    CHECK_TRUE(AccountStore::validBio(std::string(500, 'x')));
    CHECK_FALSE(AccountStore::validBio(std::string(501, 'x')));
    CHECK_FALSE(AccountStore::validBio("Bad\x01char"));
    CHECK_TRUE(AccountStore::validBio("Line 1\nLine 2\r\nLine 3\tTab"));

    CHECK_TRUE(AccountStore::validLocation("Tokyo, Japan"));
    CHECK_TRUE(AccountStore::validLocation(std::string(64, 'y')));
    CHECK_FALSE(AccountStore::validLocation(std::string(65, 'y')));
    CHECK_FALSE(AccountStore::validLocation("Bad\x05Location"));

    // Avatar URL validation
    CHECK_TRUE(AccountStore::validAvatarUrl(""));
    CHECK_TRUE(AccountStore::validAvatarUrl("https://example.com/pic.png"));
    CHECK_TRUE(AccountStore::validAvatarUrl("data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg=="));
    CHECK_TRUE(AccountStore::validAvatarUrl("data:image/jpeg;base64,/9j/4AAQSkZJRg=="));
    CHECK_TRUE(AccountStore::validAvatarUrl("data:image/webp;base64,UklGRiQAAABXRUJQVlA4IBgAAAAwAQCdASoBAAEAD8D+JaQAA3AA/ua1AAA="));
    CHECK_TRUE(AccountStore::validAvatarUrl("data:image/gif;base64,R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7"));
    CHECK_FALSE(AccountStore::validAvatarUrl("data:image/svg+xml;base64,PHN2Zz48L3N2Zz4="));
    CHECK_FALSE(AccountStore::validAvatarUrl("data:image/unknown;base64,abc"));
    CHECK_FALSE(AccountStore::validAvatarUrl("data:image/png;base64,"));
    CHECK_FALSE(AccountStore::validAvatarUrl("data:image/png;base64,bad!char@"));
    CHECK_TRUE(AccountStore::validAvatarUrl("data:image/png;base64," + std::string(32769, 'A')));
    CHECK_FALSE(AccountStore::validAvatarUrl("data:image/png;base64," + std::string(270000, 'A')));
    CHECK_TRUE(AccountStore::validAvatarUrl("/api/v1/avatars/42.png"));
    CHECK_TRUE(AccountStore::validAvatarUrl("/avatar/42.webp"));
    CHECK_FALSE(AccountStore::validAvatarUrl("/api/v1/avatars/../hack.png"));
    CHECK_FALSE(AccountStore::validAvatarUrl("/api/v1/avatars/bad file.png"));

    // Rejections via updateProfile
    ProfileUpdate badBio;
    badBio.setBio = true;
    badBio.bio = std::string(501, 'x');
    CHECK_EQUAL((int)UpdateStatus::INVALID_BIO, (int)store->updateProfile(pid, badBio));

    ProfileUpdate badLoc;
    badLoc.setLocation = true;
    badLoc.location = "Bad\x07Location";
    CHECK_EQUAL((int)UpdateStatus::INVALID_LOCATION, (int)store->updateProfile(pid, badLoc));

    ProfileUpdate badAv;
    badAv.setAvatarUrl = true;
    badAv.avatarUrl = "http://insecure.example.com/avatar.png";
    CHECK_EQUAL((int)UpdateStatus::INVALID_AVATAR_URL, (int)store->updateProfile(pid, badAv));

    // Successful update
    const std::string valid1x1PngUri =
        "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==";
    ProfileUpdate upd;
    upd.setBio = true;
    upd.bio = "Master Octi strategist.";
    upd.setLocation = true;
    upd.location = "San Francisco, CA";
    upd.setAvatarUrl = true;
    upd.avatarUrl = valid1x1PngUri;
    PlayerProfile updated;
    CHECK_EQUAL((int)UpdateStatus::OK, (int)store->updateProfile(pid, upd, &updated));
    STRCMP_EQUAL("Master Octi strategist.", updated.bio.c_str());
    STRCMP_EQUAL("San Francisco, CA", updated.location.c_str());
    STRCMP_EQUAL(valid1x1PngUri.c_str(), updated.avatarUrl.c_str());

    // Verify lookup by id, uuid, handle
    PlayerProfile p1, p2, p3;
    CHECK_TRUE(store->getProfile(pid, p1));
    STRCMP_EQUAL("Master Octi strategist.", p1.bio.c_str());
    STRCMP_EQUAL("San Francisco, CA", p1.location.c_str());

    CHECK_TRUE(store->getProfileByUuid(updated.uuid, p2));
    STRCMP_EQUAL("San Francisco, CA", p2.location.c_str());

    CHECK_TRUE(store->getProfileByHandle(updated.handle, p3));
    STRCMP_EQUAL("Master Octi strategist.", p3.bio.c_str());
    STRCMP_EQUAL(valid1x1PngUri.c_str(), p3.avatarUrl.c_str());

    CHECK_FALSE(store->getProfileByHandle("non-existent-handle-xyz", p3));
}

TEST(AccountStoreTest, ListUsersDirectory)
{
    int64_t p1 = newPlayer("sub-dir-1", "Alice Alpha");
    int64_t p2 = newPlayer("sub-dir-2", "Bob Beta");
    int64_t p3 = newPlayer("sub-dir-3", "Charlie Gamma");

    ProfileUpdate u1;
    u1.setBio = true;
    u1.bio = "Alpha player from NY";
    u1.setLocation = true;
    u1.location = "New York, USA";
    CHECK_EQUAL((int)UpdateStatus::OK, (int)store->updateProfile(p1, u1));

    ProfileUpdate u2;
    u2.setBio = true;
    u2.bio = "Beta tactician in LA";
    u2.setLocation = true;
    u2.location = "Los Angeles, CA";
    CHECK_EQUAL((int)UpdateStatus::OK, (int)store->updateProfile(p2, u2));

    ProfileUpdate u3;
    u3.setBio = true;
    u3.bio = "Gamma solver in London";
    u3.setLocation = true;
    u3.location = "London, UK";
    CHECK_EQUAL((int)UpdateStatus::OK, (int)store->updateProfile(p3, u3));

    // List all
    UserDirectoryFilter f;
    std::vector<UserDirectoryEntry> entries;
    int64_t total = 0;
    CHECK_TRUE(store->listUsers(f, entries, &total));
    CHECK_EQUAL(3, (int)total);
    CHECK_EQUAL(3, (int)entries.size());

    // Substring queries (case-insensitive across handle, display_name, location, bio)
    f.query = "alpha";
    CHECK_TRUE(store->listUsers(f, entries, &total));
    CHECK_EQUAL(1, (int)total);
    CHECK_EQUAL(1, (int)entries.size());
    STRCMP_EQUAL("Alice Alpha", entries[0].profile.displayName.c_str());

    f.query = "LONDON";
    CHECK_TRUE(store->listUsers(f, entries, &total));
    CHECK_EQUAL(1, (int)total);
    STRCMP_EQUAL("Charlie Gamma", entries[0].profile.displayName.c_str());

    f.query = "tactician";
    CHECK_TRUE(store->listUsers(f, entries, &total));
    CHECK_EQUAL(1, (int)total);
    STRCMP_EQUAL("Bob Beta", entries[0].profile.displayName.c_str());

    // Pagination
    f.query.clear();
    f.limit = 2;
    f.offset = 0;
    CHECK_TRUE(store->listUsers(f, entries, &total));
    CHECK_EQUAL(3, (int)total);
    CHECK_EQUAL(2, (int)entries.size());

    f.offset = 2;
    CHECK_TRUE(store->listUsers(f, entries, &total));
    CHECK_EQUAL(3, (int)total);
    CHECK_EQUAL(1, (int)entries.size());

    // Sorting
    f.offset = 0;
    f.limit = 50;
    f.sortField = "display_name";
    f.sortAsc = true;
    CHECK_TRUE(store->listUsers(f, entries, &total));
    CHECK_EQUAL(3, (int)entries.size());
    STRCMP_EQUAL("Alice Alpha", entries[0].profile.displayName.c_str());
    STRCMP_EQUAL("Bob Beta", entries[1].profile.displayName.c_str());
    STRCMP_EQUAL("Charlie Gamma", entries[2].profile.displayName.c_str());

    // Active robots count
    CHECK_EQUAL(0, entries[0].activeRobotsCount);
    CreateRobotResult cr = store->createRobot(p1, "alpha-bot-1", "Test bot");
    CHECK_EQUAL((int)RobotStatus::OK, (int)cr.status);

    CHECK_TRUE(store->listUsers(f, entries, &total));
    CHECK_EQUAL(1, entries[0].activeRobotsCount);

    // Banned player exclusion
    CHECK_TRUE(store->setBanned(p2, true));
    f.sortField = "display_name";
    f.sortAsc = true;
    CHECK_TRUE(store->listUsers(f, entries, &total));
    CHECK_EQUAL(2, (int)total);
    CHECK_EQUAL(2, (int)entries.size());
    STRCMP_EQUAL("Alice Alpha", entries[0].profile.displayName.c_str());
    STRCMP_EQUAL("Charlie Gamma", entries[1].profile.displayName.c_str());
}

TEST(AccountStoreTest, RobotCrudAndTokenValidation)
{
    int64_t pid = newPlayer("sub-robot-owner", "Bot Master");

    // Validators
    CHECK_FALSE(AccountStore::validRobotName("a"));
    CHECK_FALSE(AccountStore::validRobotName("ab"));
    CHECK_TRUE(AccountStore::validRobotName("abc"));
    CHECK_TRUE(AccountStore::validRobotName("My-Robot_01"));
    CHECK_FALSE(AccountStore::validRobotName("super-long-name-exceeding-24-chars-now"));
    CHECK_FALSE(AccountStore::validRobotName("has spaces"));
    CHECK_FALSE(AccountStore::validRobotName("has$invalid@chars"));

    CHECK_TRUE(AccountStore::validRobotDescription(std::string(256, 'x')));
    CHECK_FALSE(AccountStore::validRobotDescription(std::string(257, 'x')));
    CHECK_FALSE(AccountStore::validRobotDescription("bad\003desc"));

    // Create robot 1
    CreateRobotResult r1 = store->createRobot(pid, "bot-one", "First bot");
    CHECK_EQUAL((int)RobotStatus::OK, (int)r1.status);
    CHECK_TRUE(!r1.rawToken.empty());
    CHECK_EQUAL(0, r1.rawToken.rfind("octi_bot_", 0));
    STRCMP_EQUAL("bot-one", r1.robot.robotName.c_str());
    STRCMP_EQUAL("First bot", r1.robot.description.c_str());
    CHECK_EQUAL(pid, r1.robot.ownerPlayerId);

    // Active name collision (case-insensitive)
    CreateRobotResult rDup = store->createRobot(pid, "BOT-ONE", "Duplicate");
    CHECK_EQUAL((int)RobotStatus::ROBOT_NAME_TAKEN, (int)rDup.status);

    // Non-existent owner
    CreateRobotResult rMissing = store->createRobot(999999, "bot-missing", "No owner");
    CHECK_EQUAL((int)RobotStatus::NOT_FOUND, (int)rMissing.status);

    // Create 2 more
    CreateRobotResult r2 = store->createRobot(pid, "bot-two", "Second bot");
    CHECK_EQUAL((int)RobotStatus::OK, (int)r2.status);
    CreateRobotResult r3 = store->createRobot(pid, "bot-three", "Third bot");
    CHECK_EQUAL((int)RobotStatus::OK, (int)r3.status);

    // Create 29 more to hit max limit (32)
    std::vector<std::string> extraUuids;
    for (int i = 4; i <= 32; ++i) {
        std::string name = "bot-" + std::to_string(i);
        CreateRobotResult ri = store->createRobot(pid, name, "Bot " + std::to_string(i));
        CHECK_EQUAL((int)RobotStatus::OK, (int)ri.status);
        extraUuids.push_back(ri.robot.robotUuid);
    }

    // 33rd robot rejected by quota limit (max 32)
    CreateRobotResult r33 = store->createRobot(pid, "bot-33", "Thirty-third bot");
    CHECK_EQUAL((int)RobotStatus::ROBOT_LIMIT_REACHED, (int)r33.status);

    // Test getRobotById & getRobotByUuid
    RobotRecord fetchedById;
    CHECK_TRUE(store->getRobotById(r1.robot.robotId, fetchedById));
    STRCMP_EQUAL("bot-one", fetchedById.robotName.c_str());
    CHECK_EQUAL(pid, fetchedById.ownerPlayerId);
    STRCMP_EQUAL(r1.robot.robotUuid.c_str(), fetchedById.robotUuid.c_str());

    RobotRecord fetchedByUuid;
    CHECK_TRUE(store->getRobotByUuid(r1.robot.robotUuid, fetchedByUuid));
    STRCMP_EQUAL("bot-one", fetchedByUuid.robotName.c_str());
    CHECK_EQUAL(r1.robot.robotId, fetchedByUuid.robotId);

    // Test recordRobotGameOutcome
    CHECK_EQUAL(0, fetchedById.gamesPlayed);
    CHECK_EQUAL(0, fetchedById.wins);
    CHECK_EQUAL(0, fetchedById.losses);
    CHECK_EQUAL(0, fetchedById.draws);

    CHECK_TRUE(store->recordRobotGameOutcome(r1.robot.robotId, GameOutcome::WIN));
    CHECK_TRUE(store->recordRobotGameOutcome(r1.robot.robotId, GameOutcome::LOSS));
    CHECK_TRUE(store->recordRobotGameOutcome(r1.robot.robotId, GameOutcome::DRAW));

    CHECK_TRUE(store->getRobotById(r1.robot.robotId, fetchedById));
    CHECK_EQUAL(3, fetchedById.gamesPlayed);
    CHECK_EQUAL(1, fetchedById.wins);
    CHECK_EQUAL(1, fetchedById.losses);
    CHECK_EQUAL(1, fetchedById.draws);

    // Update description
    CHECK_EQUAL((int)RobotStatus::OK,
                (int)store->updateRobotDescription(pid, r1.robot.robotUuid, "Updated bot description"));

    // Owner list contains full token
    std::vector<RobotRecord> ownerBots;
    CHECK_TRUE(store->listRobots(pid, ownerBots));
    CHECK_EQUAL(32, (int)ownerBots.size());
    CHECK_TRUE(!ownerBots[0].token.empty());
    CHECK_TRUE(!ownerBots[0].tokenPrefix.empty());

    // Public list strips token and tokenPrefix
    std::vector<RobotRecord> pubBots;
    CHECK_TRUE(store->listPublicRobotsByOwner(pid, pubBots));
    CHECK_EQUAL(32, (int)pubBots.size());
    for (size_t i = 0; i < pubBots.size(); ++i) {
        CHECK_TRUE(pubBots[i].token.empty());
        CHECK_TRUE(pubBots[i].tokenPrefix.empty());
    }

    // Token validation
    RobotTokenValidation v1 = store->validateRobotToken(r1.rawToken);
    CHECK_TRUE(v1.valid);
    STRCMP_EQUAL("bot-one", v1.robot.robotName.c_str());
    CHECK_EQUAL(pid, v1.owner.playerId);

    // Invalid token validation
    RobotTokenValidation vBad = store->validateRobotToken("octi_bot_bogus_token_1234567890");
    CHECK_FALSE(vBad.valid);

    // Revocation
    CHECK_EQUAL((int)RobotStatus::OK, (int)store->revokeRobot(pid, r1.robot.robotUuid));
    CHECK_EQUAL((int)RobotStatus::ALREADY_REVOKED, (int)store->revokeRobot(pid, r1.robot.robotUuid));

    // Revoked token no longer validates
    RobotTokenValidation vRevoked = store->validateRobotToken(r1.rawToken);
    CHECK_FALSE(vRevoked.valid);

    // Revoking frees a quota slot -> revoked name can now be reused
    CreateRobotResult rReuse = store->createRobot(pid, "bot-one", "Recreated bot one");
    CHECK_EQUAL((int)RobotStatus::OK, (int)rReuse.status);

    // Active count is now 32 again -> creating 33rd robot hits limit
    CreateRobotResult r33New = store->createRobot(pid, "bot-33", "Thirty-third bot");
    CHECK_EQUAL((int)RobotStatus::ROBOT_LIMIT_REACHED, (int)r33New.status);

    // Banned owner revokes token validation
    CHECK_TRUE(store->setBanned(pid, true));
    RobotTokenValidation vBanned = store->validateRobotToken(r2.rawToken);
    CHECK_FALSE(vBanned.valid);
}

TEST(AccountStoreTest, LinkIdentityForceRelinkEmpty)
{
    LoginResult l1 = login("google", "sub-relink-p1", "Player One");
    CHECK_TRUE(l1.ok);
    int64_t p1 = l1.profile.playerId;

    LoginResult l2 = login("facebook", "sub-relink-p2", "Player Two");
    CHECK_TRUE(l2.ok);
    int64_t p2 = l2.profile.playerId;

    // Normal link conflict: p1 tries to link p2's claim
    LinkResult lr1 = store->linkIdentity(p1, makeClaims("facebook", "sub-relink-p2"), false);
    CHECK_FALSE(lr1.ok);
    CHECK_EQUAL((int)LinkStatus::CONFLICT_OTHER_ACCOUNT, (int)lr1.status);
    CHECK_EQUAL(p2, lr1.conflictingPlayerId);
    STRCMP_EQUAL(l2.profile.uuid.c_str(), lr1.conflictingPlayerUuid.c_str());

    // Record a game played by p2 so p2 is not empty
    GameRecord gr = record("game-relink-01", kNow, p2, p1);
    std::string err;
    CHECK_EQUAL((int)ArchiveStatus::OK, (int)store->archiveGame(gr, err));

    // Force relink on non-empty account should STILL fail
    LinkResult lr2 = store->linkIdentity(p1, makeClaims("facebook", "sub-relink-p2"), true);
    CHECK_FALSE(lr2.ok);
    CHECK_EQUAL((int)LinkStatus::CONFLICT_OTHER_ACCOUNT, (int)lr2.status);

    // Now test force relink on a truly empty orphan account
    LoginResult l3 = login("facebook", "sub-relink-empty-p3", "Empty Three");
    CHECK_TRUE(l3.ok);
    int64_t p3 = l3.profile.playerId;

    LinkResult lr3 = store->linkIdentity(p1, makeClaims("facebook", "sub-relink-empty-p3"), true);
    CHECK_TRUE(lr3.ok);
    CHECK_EQUAL((int)LinkStatus::OK, (int)lr3.status);

    // Orphan player p3 was deleted
    PlayerProfile p3Prof;
    CHECK_FALSE(store->getProfile(p3, p3Prof));

    // Logging in with p3's Facebook claim now authenticates as p1
    LoginResult lRelinked = login("facebook", "sub-relink-empty-p3");
    CHECK_TRUE(lRelinked.ok);
    CHECK_EQUAL(p1, lRelinked.profile.playerId);
}

TEST(AccountStoreTest, HandleChangeCooldownEnforcedAfterInitialSetup)
{
    int64_t pid = newPlayer("g-cd", "Cooldown Tester");
    PlayerProfile p;
    CHECK_TRUE(store->getProfileById(pid, p));
    CHECK_EQUAL(0, p.handleChangedAt);

    // Initial handle customization is allowed immediately
    ProfileUpdate u;
    u.setHandle = true;
    u.handle = "first_handle";
    CHECK_EQUAL((int)UpdateStatus::OK, (int)store->updateProfile(pid, u));
    CHECK_TRUE(store->getProfileById(pid, p));
    CHECK_EQUAL(clockNow, p.handleChangedAt);
    STRCMP_EQUAL("first_handle", p.handle.c_str());

    // Subsequent change within 24 hours is rate limited
    u.handle = "second_handle";
    CHECK_EQUAL((int)UpdateStatus::HANDLE_RATE_LIMITED, (int)store->updateProfile(pid, u));

    // Saving profile with same handle (or without setHandle) is not rate limited
    u.handle = "first_handle";
    u.setDisplayName = true;
    u.displayName = "Cooldown Master";
    CHECK_EQUAL((int)UpdateStatus::OK, (int)store->updateProfile(pid, u));
    CHECK_TRUE(store->getProfileById(pid, p));
    STRCMP_EQUAL("Cooldown Master", p.displayName.c_str());

    // After 24 hours, handle change succeeds
    clockNow += AccountStore::kHandleChangeCooldownSec;
    u.handle = "second_handle";
    CHECK_EQUAL((int)UpdateStatus::OK, (int)store->updateProfile(pid, u));
    CHECK_TRUE(store->getProfileById(pid, p));
    STRCMP_EQUAL("second_handle", p.handle.c_str());
    CHECK_EQUAL(clockNow, p.handleChangedAt);
}

TEST(AccountStoreTest, AvatarMaterializationAndDiskCleanup)
{
    std::string avDir = dir->path() + "/test_avatars";
    store->setAvatarDir(avDir);
    STRCMP_EQUAL(avDir.c_str(), store->avatarDir().c_str());

    int64_t pid = newPlayer("g-av", "Avatar Tester");
    std::string pngData = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==";

    ProfileUpdate u;
    u.setAvatarUrl = true;
    u.avatarUrl = pngData;
    CHECK_EQUAL((int)UpdateStatus::OK, (int)store->updateProfile(pid, u));

    PlayerProfile p;
    CHECK_TRUE(store->getProfileById(pid, p));
    std::string expectedUrlPrefix = "/api/v1/avatars/" + std::to_string(pid) + ".png?v=";
    CHECK_TRUE(p.avatarUrl.rfind(expectedUrlPrefix, 0) == 0);

    std::string diskPng = avDir + "/" + std::to_string(pid) + ".png";
    CHECK_EQUAL(0, ::access(diskPng.c_str(), F_OK));

    std::string bytes, mime;
    CHECK_TRUE(store->readAvatarFile(std::to_string(pid) + ".png", bytes, mime));
    STRCMP_EQUAL("image/png", mime.c_str());
    CHECK_TRUE(bytes.size() > 0);

    // Replacing with webp transcodes it to png on disk and updates avatar_url with .png
    std::string webpData = "data:image/webp;base64,UklGRiQAAABXRUJQVlA4IBgAAAAwAQCdASoBAAEAD8D+JaQAA3AA/ua1AAA=";
    u.avatarUrl = webpData;
    CHECK_EQUAL((int)UpdateStatus::OK, (int)store->updateProfile(pid, u));

    CHECK_EQUAL(0, ::access(diskPng.c_str(), F_OK));
    CHECK_TRUE(store->readAvatarFile(std::to_string(pid) + ".png", bytes, mime));
    STRCMP_EQUAL("image/png", mime.c_str());
    CHECK_TRUE(bytes.size() >= 8);
    CHECK_EQUAL(0, std::memcmp(bytes.data(), "\x89PNG\r\n\x1a\n", 8));

    // An existing legacy .webp file on disk is read and transcoded to image/png
    std::string diskWebp = avDir + "/9999.webp";
    std::string rawWebp(
        "\x52\x49\x46\x46\x24\x00\x00\x00\x57\x45\x42\x50\x56\x50\x38\x20"
        "\x18\x00\x00\x00\x30\x01\x00\x9d\x01\x2a\x01\x00\x01\x00\x0f\xc0"
        "\xfe\x25\xa4\x00\x03\x70\x00\xfe\xe6\xb5\x00\x00",
        44);
    {
        std::ofstream out(diskWebp.c_str(), std::ios::binary | std::ios::trunc);
        out.write(rawWebp.data(), rawWebp.size());
    }
    CHECK_TRUE(store->readAvatarFile("9999.webp", bytes, mime));
    STRCMP_EQUAL("image/png", mime.c_str());
    CHECK_TRUE(bytes.size() >= 8);
    CHECK_EQUAL(0, std::memcmp(bytes.data(), "\x89PNG\r\n\x1a\n", 8));

    // Clearing avatar removes the disk file
    u.avatarUrl = "";
    CHECK_EQUAL((int)UpdateStatus::OK, (int)store->updateProfile(pid, u));
    CHECK_TRUE(::access(diskPng.c_str(), F_OK) != 0);

    // Negative tests for readAvatarFile path safety
    CHECK_FALSE(store->readAvatarFile("../secret.png", bytes, mime));
    CHECK_FALSE(store->readAvatarFile("abc/xyz.png", bytes, mime));
    CHECK_FALSE(store->readAvatarFile("notanumber.png", bytes, mime));
}

TEST(AccountStoreTest, RetroactiveInstallIdReconciliation)
{
    int64_t p1 = newPlayer("g-rec", "Samurai User");
    ProfileUpdate u;
    u.setHandle = true;
    u.handle = "samurai";
    CHECK_EQUAL((int)UpdateStatus::OK, (int)store->updateProfile(p1, u));

    // Game 1: device_A (seat 1 human, seat 2 AI)
    GameRecord g1 = record("rec-g1", clockNow, 0, 0);
    g1.source = "client_upload";
    g1.installId = "device_A";
    g1.p1Handle = "Player";
    g1.p2Handle = "AI_Easy";
    std::string err;
    CHECK_EQUAL((int)ArchiveStatus::OK, (int)store->archiveGame(g1, err));

    // Game 2: device_B (unregistered device)
    GameRecord g2 = record("rec-g2", clockNow, 0, 0);
    g2.source = "client_upload";
    g2.installId = "device_B";
    g2.p1Handle = "Player";
    g2.p2Handle = "AI_Easy";
    CHECK_EQUAL((int)ArchiveStatus::OK, (int)store->archiveGame(g2, err));

    // Game 3: device_A (seat 1 AI, seat 2 human)
    GameRecord g3 = record("rec-g3", clockNow, 0, 0);
    g3.source = "client_upload";
    g3.installId = "device_A";
    g3.p1Handle = "AI_Medium";
    g3.p2Handle = "Player";
    CHECK_EQUAL((int)ArchiveStatus::OK, (int)store->archiveGame(g3, err));

    // Bind device_A to samurai
    int reconciled = 0;
    CHECK_TRUE(store->bindInstallToPlayer("device_A", p1, &reconciled));
    CHECK_EQUAL(2, reconciled);

    // Verify games 1 and 3 are attributed, game 2 remains anonymous
    GameRecord out1;
    CHECK_TRUE(store->getArchivedGame("rec-g1", out1));
    CHECK_EQUAL(p1, out1.p1PlayerId);
    STRCMP_EQUAL("samurai", out1.p1Handle.c_str());

    GameRecord out3;
    CHECK_TRUE(store->getArchivedGame("rec-g3", out3));
    CHECK_EQUAL(p1, out3.p2PlayerId);
    STRCMP_EQUAL("samurai", out3.p2Handle.c_str());

    GameRecord out2;
    CHECK_TRUE(store->getArchivedGame("rec-g2", out2));
    CHECK_EQUAL(0, out2.p1PlayerId);
    STRCMP_EQUAL("Player", out2.p1Handle.c_str());

    // Handle change triggers reconciliation on attributed games
    clockNow += AccountStore::kHandleChangeCooldownSec;
    u.handle = "ninja";
    CHECK_EQUAL((int)UpdateStatus::OK, (int)store->updateProfile(p1, u));

    CHECK_TRUE(store->getArchivedGame("rec-g1", out1));
    STRCMP_EQUAL("ninja", out1.p1Handle.c_str());
    CHECK_TRUE(store->getArchivedGame("rec-g3", out3));
    STRCMP_EQUAL("ninja", out3.p2Handle.c_str());

    // Deleting install_id removes installs mapping as well
    CHECK_EQUAL(2, store->deleteGamesByInstallId("device_A"));
    PlayerProfile checkProf;
    CHECK_FALSE(store->getPlayerByInstallId("device_A", checkProf));
}

TEST(AccountStoreTest, DeleteRobotRetentionAndDirectoryFiltering)
{
    LoginResult l1 = login("google", "sub-vet-owner", "Veteran Owner");
    CHECK_TRUE(l1.ok);
    int64_t ownerId = l1.profile.playerId;

    LoginResult l2 = login("google", "sub-opponent", "Human Opponent");
    CHECK_TRUE(l2.ok);
    int64_t opponentId = l2.profile.playerId;

    // Create 3 robots
    CreateRobotResult r1 = store->createRobot(ownerId, "AlphaMaster", "Master alpha engine");
    CHECK_EQUAL((int)RobotStatus::OK, (int)r1.status);
    CHECK_FALSE(r1.robot.tokenDeleted);

    CreateRobotResult r2 = store->createRobot(ownerId, "BetaBlaster", "Fast beta tactician");
    CHECK_EQUAL((int)RobotStatus::OK, (int)r2.status);

    CreateRobotResult r3 = store->createRobot(ownerId, "UnplayedBot", "Never played a game");
    CHECK_EQUAL((int)RobotStatus::OK, (int)r3.status);

    // Test getRobotByName case-insensitive lookup
    RobotRecord byName;
    CHECK_TRUE(store->getRobotByName("alphamaster", byName));
    STRCMP_EQUAL("AlphaMaster", byName.robotName.c_str());
    STRCMP_EQUAL("Master alpha engine", byName.description.c_str());
    CHECK_EQUAL(ownerId, byName.ownerPlayerId);
    STRCMP_EQUAL(l1.profile.uuid.c_str(), byName.ownerUuid.c_str());
    CHECK_TRUE(byName.token.empty()); // Public query clears token

    // deleteRobot on unplayed robot (0 games) physically deletes it
    CHECK_EQUAL((int)RobotStatus::OK, (int)store->deleteRobot(ownerId, r3.robot.robotUuid));
    RobotRecord checkDeleted;
    CHECK_FALSE(store->getRobotById(r3.robot.robotId, checkDeleted));
    CHECK_FALSE(store->getRobotByName("UnplayedBot", checkDeleted));

    // Record games for AlphaMaster (2 games: 1 win, 1 loss)
    CHECK_TRUE(store->recordRobotGameOutcome(r1.robot.robotId, GameOutcome::WIN));
    CHECK_TRUE(store->recordRobotGameOutcome(r1.robot.robotId, GameOutcome::LOSS));

    // Archive 1 finished game for AlphaMaster (Seat 1: AlphaMaster, Seat 2: Human Opponent)
    std::string err;
    GameRecord g1 = record("g-bot-alpha-01", kNow, 0, opponentId);
    g1.p1RobotId = r1.robot.robotId;
    g1.p1Handle = "AlphaMaster";
    g1.p2Handle = "Human Opponent";
    CHECK_EQUAL((int)ArchiveStatus::OK, (int)store->archiveGame(g1, err));

    // Record games for BetaBlaster (1 game: 1 win)
    CHECK_TRUE(store->recordRobotGameOutcome(r2.robot.robotId, GameOutcome::WIN));
    GameRecord g2 = record("g-bot-beta-01", kNow, opponentId, 0);
    g2.p2RobotId = r2.robot.robotId;
    g2.p1Handle = "Human Opponent";
    g2.p2Handle = "BetaBlaster";
    CHECK_EQUAL((int)ArchiveStatus::OK, (int)store->archiveGame(g2, err));

    // listPublicRobotsDirectory:
    // Unplayed bots (gamesPlayed == 0) are excluded
    int64_t totalRobots = 0;
    std::vector<RobotRecord> dir;
    octi::RobotDirectoryFilter filter;
    CHECK_TRUE(store->listPublicRobotsDirectory(filter, dir, &totalRobots));
    CHECK_EQUAL(2, (int)totalRobots);
    CHECK_EQUAL(2, (int)dir.size());

    // Search query on robot_name
    filter.query = "alpha";
    CHECK_TRUE(store->listPublicRobotsDirectory(filter, dir, &totalRobots));
    CHECK_EQUAL(1, (int)totalRobots);
    STRCMP_EQUAL("AlphaMaster", dir[0].robotName.c_str());

    // Search query on description
    filter.query = "tactician";
    CHECK_TRUE(store->listPublicRobotsDirectory(filter, dir, &totalRobots));
    CHECK_EQUAL(1, (int)totalRobots);
    STRCMP_EQUAL("BetaBlaster", dir[0].robotName.c_str());

    // Search query on owner handle
    filter.query = "Veteran";
    CHECK_TRUE(store->listPublicRobotsDirectory(filter, dir, &totalRobots));
    CHECK_EQUAL(2, (int)totalRobots);

    // Sorting by win_rate descending:
    // BetaBlaster has 1 win / 1 game (100%), AlphaMaster has 1 win / 2 games (50%)
    filter.query.clear();
    filter.sortField = "win_rate";
    filter.sortAsc = false;
    CHECK_TRUE(store->listPublicRobotsDirectory(filter, dir, &totalRobots));
    CHECK_EQUAL(2, (int)dir.size());
    STRCMP_EQUAL("BetaBlaster", dir[0].robotName.c_str());
    STRCMP_EQUAL("AlphaMaster", dir[1].robotName.c_str());

    // Sorting by games_played descending:
    filter.sortField = "games_played";
    CHECK_TRUE(store->listPublicRobotsDirectory(filter, dir, &totalRobots));
    STRCMP_EQUAL("AlphaMaster", dir[0].robotName.c_str());
    STRCMP_EQUAL("BetaBlaster", dir[1].robotName.c_str());

    // Sorting by robot_name ascending:
    filter.sortField = "robot_name";
    filter.sortAsc = true;
    CHECK_TRUE(store->listPublicRobotsDirectory(filter, dir, &totalRobots));
    STRCMP_EQUAL("AlphaMaster", dir[0].robotName.c_str());
    STRCMP_EQUAL("BetaBlaster", dir[1].robotName.c_str());

    // deleteRobot on veteran robot (AlphaMaster has 2 games played):
    // Must NOT delete from robots table!
    CHECK_EQUAL((int)RobotStatus::OK, (int)store->deleteRobot(ownerId, r1.robot.robotUuid));

    // Calling deleteRobot a second time returns NOT_FOUND
    CHECK_EQUAL((int)RobotStatus::NOT_FOUND, (int)store->deleteRobot(ownerId, r1.robot.robotUuid));

    // Robot profile still exists in database and stats are intact
    RobotRecord vetRec;
    CHECK_TRUE(store->getRobotById(r1.robot.robotId, vetRec));
    CHECK_TRUE(vetRec.revoked);
    CHECK_TRUE(vetRec.tokenDeleted);
    CHECK_TRUE(vetRec.token.empty());
    CHECK_TRUE(vetRec.tokenPrefix.empty());
    CHECK_EQUAL(2, vetRec.gamesPlayed);
    CHECK_EQUAL(1, vetRec.wins);
    CHECK_EQUAL(1, vetRec.losses);

    // Private owner list (listRobots) HIDES deleted tokens
    std::vector<RobotRecord> ownerList;
    CHECK_TRUE(store->listRobots(ownerId, ownerList));
    CHECK_EQUAL(1, (int)ownerList.size());
    STRCMP_EQUAL("BetaBlaster", ownerList[0].robotName.c_str());

    // Public list by owner (listPublicRobotsByOwner) STILL includes veteran robot
    std::vector<RobotRecord> publicOwnerList;
    CHECK_TRUE(store->listPublicRobotsByOwner(ownerId, publicOwnerList));
    CHECK_EQUAL(2, (int)publicOwnerList.size());

    // Public robots directory STILL includes veteran robot
    filter.sortField = "games_played";
    filter.sortAsc = false;
    CHECK_TRUE(store->listPublicRobotsDirectory(filter, dir, &totalRobots));
    CHECK_EQUAL(2, (int)totalRobots);
    STRCMP_EQUAL("AlphaMaster", dir[0].robotName.c_str());
    CHECK_TRUE(dir[0].tokenDeleted);

    // listArchivedGames filtering by robotId / robotUuid
    std::vector<GameRecord> archivedGames;
    int64_t archiveTotal = 0;
    octi::ArchiveFilter af;
    af.robotId = r1.robot.robotId;
    CHECK_TRUE(store->listArchivedGames(af, archivedGames, &archiveTotal));
    CHECK_EQUAL(1, (int)archiveTotal);
    CHECK_EQUAL(1, (int)archivedGames.size());
    STRCMP_EQUAL("g-bot-alpha-01", archivedGames[0].gameId.c_str());
    CHECK_EQUAL(r1.robot.robotId, archivedGames[0].p1RobotId);

    octi::ArchiveFilter afUuid;
    afUuid.robotUuid = r2.robot.robotUuid;
    CHECK_TRUE(store->listArchivedGames(afUuid, archivedGames, &archiveTotal));
    CHECK_EQUAL(1, (int)archiveTotal);
    CHECK_EQUAL(1, (int)archivedGames.size());
    STRCMP_EQUAL("g-bot-beta-01", archivedGames[0].gameId.c_str());
    CHECK_EQUAL(r2.robot.robotId, archivedGames[0].p2RobotId);
}

TEST(AccountStoreTest, DualPoolThreeLadderMigrationStandingsAndPairCap)
{
    /* 1. Legacy DB idempotent migration: seeds elo_2p_3b = elo_2p exactly once */
    std::string legacyDbPath = dir->path() + "/legacy_dual_pool.db";
    {
        sqlite3 *ldb = nullptr;
        CHECK_EQUAL(SQLITE_OK, sqlite3_open(legacyDbPath.c_str(), &ldb));
        const char *legacySql =
            "CREATE TABLE players ("
            "  player_id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  uuid TEXT NOT NULL UNIQUE,"
            "  handle TEXT NOT NULL UNIQUE COLLATE NOCASE,"
            "  display_name TEXT NOT NULL DEFAULT '',"
            "  avatar_url TEXT NOT NULL DEFAULT '',"
            "  bio TEXT NOT NULL DEFAULT '',"
            "  location TEXT NOT NULL DEFAULT '',"
            "  banned INTEGER NOT NULL DEFAULT 0,"
            "  elo_2p INTEGER NOT NULL DEFAULT 1200,"
            "  elo_4p INTEGER NOT NULL DEFAULT 1200,"
            "  elo_ofk INTEGER NOT NULL DEFAULT 1200,"
            "  games_played INTEGER NOT NULL DEFAULT 0,"
            "  wins INTEGER NOT NULL DEFAULT 0,"
            "  losses INTEGER NOT NULL DEFAULT 0,"
            "  draws INTEGER NOT NULL DEFAULT 0,"
            "  created_at INTEGER NOT NULL,"
            "  last_seen_at INTEGER NOT NULL,"
            "  handle_changed_at INTEGER NOT NULL DEFAULT 0"
            ");"
            "INSERT INTO players (uuid, handle, display_name, elo_2p, elo_4p, elo_ofk, games_played, wins, created_at, last_seen_at)"
            " VALUES ('u-legacy-1', 'LegacyChamp', 'Legacy Champ', 1435, 1310, 1280, 12, 9, 1700000000, 1700000100);";
        char *errMsg = nullptr;
        CHECK_EQUAL(SQLITE_OK, sqlite3_exec(ldb, legacySql, nullptr, nullptr, &errMsg));
        sqlite3_close(ldb);
    }

    AccountStore migStore;
    std::string err;
    CHECK_TRUE_TEXT(migStore.openOrCreate(legacyDbPath, err), err.c_str());
    PlayerProfile legProf;
    CHECK_TRUE(migStore.getProfileByUuid("u-legacy-1", legProf));
    CHECK_EQUAL(1435, legProf.elo2p3b);
    CHECK_EQUAL(1435, legProf.elo2p);
    CHECK_EQUAL(1200, legProf.elo2p1b);
    CHECK_EQUAL(1280, legProf.eloOfk);
    CHECK_EQUAL(1200, legProf.arenaElo2p3b);
    CHECK_EQUAL(1200, legProf.arenaElo2p1b);
    CHECK_EQUAL(1200, legProf.arenaEloOfk);
    CHECK_EQUAL(0, legProf.arenaGamesPlayed);

    /* Update elo_2p_3b, reopen DB, confirm idempotent open does not overwrite */
    std::vector<octi::RatingUpdate> migUp(1);
    migUp[0].playerId = legProf.playerId;
    migUp[0].newElo = 1460;
    migUp[0].outcome = GameOutcome::WIN;
    CHECK_TRUE(migStore.applyRatingUpdates("2P_3B", migUp, err));
    migStore.close();
    CHECK_TRUE_TEXT(migStore.openOrCreate(legacyDbPath, err), err.c_str());
    CHECK_TRUE(migStore.getProfileByUuid("u-legacy-1", legProf));
    CHECK_EQUAL(1460, legProf.elo2p3b);
    CHECK_EQUAL(1460, legProf.elo2p);
    migStore.close();

    /* 2. Pool 1 (Human-Only) 3 independent ladders: 2P_3B, 2P_1B, OFK */
    int64_t h1 = newPlayer("g-h1", "HumanOne");
    int64_t h2 = newPlayer("g-h2", "HumanTwo");
    int64_t h3 = newPlayer("g-h3", "HumanThree");

    GameRecord g3b = record("g-pool1-3b", clockNow, h1, h2, "2P");
    g3b.gameBases = 3;
    g3b.rated = 1;
    g3b.eloPool = 1;
    std::vector<octi::RatingUpdate> u3b(2);
    u3b[0] = {h1, 0, 1216, GameOutcome::WIN};
    u3b[1] = {h2, 0, 1184, GameOutcome::LOSS};
    CHECK_EQUAL((int)ArchiveStatus::OK,
                (int)store->archiveGameWithRatings(g3b, u3b, {}, err));

    GameRecord g1b = record("g-pool1-1b", clockNow + 10, h1, h2, "2P");
    g1b.gameBases = 1;
    g1b.rated = 1;
    g1b.eloPool = 1;
    std::vector<octi::RatingUpdate> u1b(2);
    u1b[0] = {h1, 0, 1184, GameOutcome::LOSS};
    u1b[1] = {h2, 0, 1216, GameOutcome::WIN};
    CHECK_EQUAL((int)ArchiveStatus::OK,
                (int)store->archiveGameWithRatings(g1b, u1b, {}, err));

    GameRecord gOfk = record("g-pool1-ofk", clockNow + 20, h1, h2, "OFK");
    gOfk.gameBases = 1;
    gOfk.rated = 1;
    gOfk.eloPool = 1;
    std::vector<octi::RatingUpdate> uOfk(2);
    uOfk[0] = {h1, 0, 1232, GameOutcome::WIN};
    uOfk[1] = {h2, 0, 1168, GameOutcome::LOSS};
    CHECK_EQUAL((int)ArchiveStatus::OK,
                (int)store->archiveGameWithRatings(gOfk, uOfk, {}, err));

    PlayerProfile p1, p2;
    CHECK_TRUE(store->getProfileById(h1, p1));
    CHECK_TRUE(store->getProfileById(h2, p2));
    CHECK_EQUAL(1216, p1.elo2p3b);
    CHECK_EQUAL(1216, p1.elo2p);
    CHECK_EQUAL(1184, p1.elo2p1b);
    CHECK_EQUAL(1232, p1.eloOfk);
    CHECK_EQUAL(3, p1.gamesPlayed);
    CHECK_EQUAL(2, p1.wins);
    CHECK_EQUAL(1, p1.losses);
    CHECK_EQUAL(1200, p1.arenaElo2p3b);
    CHECK_EQUAL(0, p1.arenaGamesPlayed);

    GameRecord got3b;
    CHECK_TRUE(store->getArchivedGame("g-pool1-3b", got3b));
    CHECK_EQUAL(1, got3b.rated);
    CHECK_EQUAL(1, got3b.eloPool);

    /* 3. Pool 2 (Arena) single-tx archiveGameWithRatings + rollback atomicity + pair cap */
    CreateRobotResult b1 = store->createRobot(h2, "ArenaBotAlpha", "Alpha arena bot");
    CreateRobotResult b2 = store->createRobot(h2, "ArenaBotBeta", "Beta arena bot");
    CreateRobotResult bZero = store->createRobot(h2, "ActiveZeroGames", "Should hide until first game");
    CreateRobotResult bBannedOwner = store->createRobot(h3, "BannedOwnerBot", "Should hide when owner banned");
    CHECK_EQUAL((int)octi::RobotStatus::OK, (int)b1.status);
    CHECK_EQUAL((int)octi::RobotStatus::OK, (int)b2.status);
    CHECK_EQUAL((int)octi::RobotStatus::OK, (int)bZero.status);
    CHECK_EQUAL((int)octi::RobotStatus::OK, (int)bBannedOwner.status);
    CHECK_TRUE(store->recordRobotGameOutcome(b2.robot.robotId, GameOutcome::DRAW));
    CHECK_TRUE(store->recordRobotGameOutcome(bBannedOwner.robot.robotId, GameOutcome::WIN));

    /* Rollback check: invalid ghost robotId aborts game archive, human arena update, and robot outcome */
    GameRecord gBad = record("g-pool2-bad", clockNow + 30, h1, 0, "2P");
    gBad.p2RobotId = b1.robot.robotId;
    gBad.p2Handle = b1.robot.robotName;
    gBad.rated = 1;
    gBad.eloPool = 2;
    std::vector<octi::RatingUpdate> badUp(2);
    badUp[0] = {h1, 0, 1250, GameOutcome::WIN};
    badUp[1] = {0, 999999, 1150, GameOutcome::LOSS};
    std::vector<octi::RobotOutcomeUpdate> badOut = {{b1.robot.robotId, GameOutcome::LOSS}};
    CHECK_TRUE(store->archiveGameWithRatings(gBad, badUp, badOut, err) != ArchiveStatus::OK);
    GameRecord missing;
    CHECK_FALSE(store->getArchivedGame("g-pool2-bad", missing));
    RobotRecord rCheck;
    CHECK_TRUE(store->getRobotById(b1.robot.robotId, rCheck));
    CHECK_EQUAL(0, rCheck.gamesPlayed);
    CHECK_TRUE(store->getProfileById(h1, p1));
    CHECK_EQUAL(0, p1.arenaGamesPlayed);

    /* Valid Pool 2 Human vs Robot game in both seat orders + window test */
    clockNow += 100;
    GameRecord gOld = record("g-pool2-old", clockNow - 90000, h1, 0, "2P");
    gOld.p2RobotId = b1.robot.robotId;
    gOld.p2Handle = b1.robot.robotName;
    gOld.rated = 1;
    gOld.eloPool = 2;
    std::vector<octi::RatingUpdate> uArena1 = {
        {h1, 0, 1216, GameOutcome::WIN},
        {0, b1.robot.robotId, 1184, GameOutcome::LOSS}
    };
    std::vector<octi::RobotOutcomeUpdate> ro1 = {{b1.robot.robotId, GameOutcome::LOSS}};
    CHECK_EQUAL((int)ArchiveStatus::OK,
                (int)store->archiveGameWithRatings(gOld, uArena1, ro1, err));

    GameRecord gRecentA = record("g-pool2-rec1", clockNow - 3600, h1, 0, "2P");
    gRecentA.p2RobotId = b1.robot.robotId;
    gRecentA.p2Handle = b1.robot.robotName;
    gRecentA.rated = 1;
    gRecentA.eloPool = 2;
    std::vector<octi::RatingUpdate> uArena2 = {
        {h1, 0, 1230, GameOutcome::WIN},
        {0, b1.robot.robotId, 1170, GameOutcome::LOSS}
    };
    CHECK_EQUAL((int)ArchiveStatus::OK,
                (int)store->archiveGameWithRatings(gRecentA, uArena2, ro1, err));

    /* Seat-swapped (Robot seat 1, Human seat 2) */
    GameRecord gRecentB = record("g-pool2-rec2", clockNow - 1800, 0, h1, "2P");
    gRecentB.p1RobotId = b1.robot.robotId;
    gRecentB.p1Handle = b1.robot.robotName;
    gRecentB.rated = 1;
    gRecentB.eloPool = 2;
    std::vector<octi::RatingUpdate> uArena3 = {
        {0, b1.robot.robotId, 1188, GameOutcome::WIN},
        {h1, 0, 1212, GameOutcome::LOSS}
    };
    std::vector<octi::RobotOutcomeUpdate> roWin = {{b1.robot.robotId, GameOutcome::WIN}};
    CHECK_EQUAL((int)ArchiveStatus::OK,
                (int)store->archiveGameWithRatings(gRecentB, uArena3, roWin, err));

    /* Unrated Pool 0 game with same pair must NOT count toward rated pair cap */
    GameRecord gUnratedPair = record("g-pool0-pair", clockNow - 600, h1, 0, "2P");
    gUnratedPair.p2RobotId = b1.robot.robotId;
    gUnratedPair.p2Handle = b1.robot.robotName;
    gUnratedPair.rated = 1;
    gUnratedPair.eloPool = 0;
    CHECK_EQUAL((int)ArchiveStatus::OK,
                (int)store->archiveGameWithRatings(gUnratedPair, {}, ro1, err));

    CHECK_EQUAL(2, store->countRecentArenaRatedPairGames(
                       h1, 0, 0, b1.robot.robotId, 24LL * 3600));
    CHECK_EQUAL(2, store->countRecentArenaRatedPairGames(
                       0, b1.robot.robotId, h1, 0, 24LL * 3600));

    CHECK_TRUE(store->getProfileById(h1, p1));
    CHECK_EQUAL(1216, p1.elo2p3b);        /* Pool 1 untouched by Arena! */
    CHECK_EQUAL(3, p1.gamesPlayed);       /* Pool 1 gamesPlayed untouched! */
    CHECK_EQUAL(1212, p1.arenaElo2p3b);
    CHECK_EQUAL(3, p1.arenaGamesPlayed);
    CHECK_EQUAL(2, p1.arenaWins);
    CHECK_EQUAL(1, p1.arenaLosses);

    CHECK_TRUE(store->getRobotById(b1.robot.robotId, rCheck));
    CHECK_EQUAL(1188, rCheck.elo2p3b);
    CHECK_EQUAL(4, rCheck.gamesPlayed);
    CHECK_EQUAL(1, rCheck.wins);
    CHECK_EQUAL(3, rCheck.losses);

    /* 4. listArenaStandings: all | robots | humans + banned/revoked exclusions + sorts */
    CHECK_TRUE(store->setBanned(h3, true));
    octi::ArenaStandingsFilter asf;
    asf.participantType = "all";
    asf.sortField = "elo_2p_3b";
    std::vector<octi::ArenaStandingEntry> standings;
    int64_t totalStandings = 0;
    CHECK_TRUE(store->listArenaStandings(asf, standings, &totalStandings));
    /* Expected: h1 (human, 3 rated + 1 unrated vs-bot game -> gamesPlayed=4, games2p3b=3, elo2p3b=1212),
     * b1 (4-game robot with 3 rated 2P_3B games, games2p3b=3, elo2p3b=1188),
     * b2 (casual-only 1-game robot with 0 rated 2P_3B games, games2p3b=0, elo2p3b=1200 sorted after ranked!).
     * Excluded: h2 (0 vs-bot games), h3 (banned), bZero (0 games), bBannedOwner (owner h3 banned). */
    CHECK_EQUAL(3, (int)totalStandings);
    CHECK_EQUAL(3, (int)standings.size());
    STRCMP_EQUAL("human", standings[0].entryType.c_str());
    STRCMP_EQUAL("HumanOne", standings[0].name.c_str());
    CHECK_EQUAL(1212, standings[0].elo2p3b);
    CHECK_EQUAL(3, standings[0].games2p3b);
    CHECK_EQUAL(4, standings[0].gamesPlayed);
    CHECK_EQUAL(3, standings[0].wins);
    CHECK_EQUAL(1, standings[0].losses);
    STRCMP_EQUAL("robot", standings[1].entryType.c_str());
    STRCMP_EQUAL("ArenaBotAlpha", standings[1].name.c_str());
    CHECK_EQUAL(1188, standings[1].elo2p3b);
    CHECK_EQUAL(3, standings[1].games2p3b);
    STRCMP_EQUAL("robot", standings[2].entryType.c_str());
    STRCMP_EQUAL("ArenaBotBeta", standings[2].name.c_str());
    CHECK_EQUAL(1200, standings[2].elo2p3b);
    CHECK_EQUAL(0, standings[2].games2p3b);

    asf.participantType = "robots";
    CHECK_TRUE(store->listArenaStandings(asf, standings, &totalStandings));
    CHECK_EQUAL(2, (int)totalStandings);
    for (size_t i = 0; i < standings.size(); ++i) {
        STRCMP_EQUAL("robot", standings[i].entryType.c_str());
    }

    asf.participantType = "humans";
    CHECK_TRUE(store->listArenaStandings(asf, standings, &totalStandings));
    CHECK_EQUAL(1, (int)totalStandings);
    STRCMP_EQUAL("human", standings[0].entryType.c_str());
    STRCMP_EQUAL("HumanOne", standings[0].name.c_str());
    CHECK_EQUAL(4, standings[0].gamesPlayed);
}

TEST(AccountStoreTest, ArenaVsRobotUnratedHumans1v1And4pAndCasualBotCounts)
{
    int64_t owner = newPlayer("g-vb-owner", "BotOwner");
    int64_t h1v1 = newPlayer("g-vb-1v1", "Unrated1v1Human");
    int64_t h4pOpp1 = newPlayer("g-vb-4p1", "Team13HumanA");
    int64_t h4pOpp3 = newPlayer("g-vb-4p3", "Team13HumanC");
    int64_t h4pAlly4 = newPlayer("g-vb-4p4", "Team24AllyOfBot");

    CreateRobotResult botRes = store->createRobot(owner, "CasualAnd4pBot", "Bot");
    CHECK_EQUAL((int)octi::RobotStatus::OK, (int)botRes.status);
    int64_t botId = botRes.robot.robotId;

    std::string err;
    /* 1. Unrated 1v1 game (casual 2P_3B_SP) between h1v1 (seat 1) and botId (seat 2) */
    GameRecord g1v1 = record("g-vb-unrated-1v1", clockNow + 10, h1v1, 0, "2P");
    g1v1.gameBases = 3;
    g1v1.superProngs = true;
    g1v1.p2RobotId = botId;
    g1v1.p2Handle = botRes.robot.robotName;
    g1v1.rated = 0;
    g1v1.eloPool = 0;
    g1v1.result = "P1_WIN";
    g1v1.winnerSeat = 1;
    g1v1.winner = "A";
    CHECK_EQUAL((int)ArchiveStatus::OK,
                (int)store->archiveGameWithRatings(
                    g1v1, {}, {{botId, GameOutcome::LOSS}}, err));

    /* 2. 4P partnership game: Team (1,3) = (h4pOpp1, h4pOpp3) vs Team (2,4) = (botId, h4pAlly4).
     *    Seats 1 and 3 oppose robot seat 2; seat 4 is partnered WITH robot seat 2 and opposes only humans! */
    GameRecord g4p = record("g-vb-4p-match", clockNow + 20, h4pOpp1, 0, "4P");
    g4p.gameBases = 3;
    g4p.p2RobotId = botId;
    g4p.p2Handle = botRes.robot.robotName;
    g4p.p3PlayerId = h4pOpp3;
    g4p.p3Handle = "Team13HumanC";
    g4p.p4PlayerId = h4pAlly4;
    g4p.p4Handle = "Team24AllyOfBot";
    g4p.rated = 0;
    g4p.eloPool = 0;
    g4p.result = "TEAM_13_WIN";
    g4p.winnerSeat = 1;
    g4p.winner = "A";
    CHECK_EQUAL((int)ArchiveStatus::OK,
                (int)store->archiveGameWithRatings(
                    g4p, {}, {{botId, GameOutcome::LOSS}}, err));

    /* Casual-only bot has gamesPlayed == 2, elo2p3b == 1200, and games2p3b/1b/ofk == 0 */
    RobotRecord rGot;
    CHECK_TRUE(store->getRobotById(botId, rGot));
    CHECK_EQUAL(2, rGot.gamesPlayed);
    CHECK_EQUAL(1200, rGot.elo2p3b);
    CHECK_EQUAL(0, rGot.games2p3b);
    CHECK_EQUAL(0, rGot.games2p1b);
    CHECK_EQUAL(0, rGot.gamesOfk);

    octi::ArenaStandingsFilter asf;
    asf.participantType = "humans";
    asf.sortField = "games_played";
    std::vector<octi::ArenaStandingEntry> humans;
    int64_t totalHumans = 0;
    CHECK_TRUE(store->listArenaStandings(asf, humans, &totalHumans));
    /* h1v1, h4pOpp1, and h4pOpp3 opposed a robot seat; owner and h4pAlly4 did NOT */
    CHECK_EQUAL(3, (int)totalHumans);
    std::set<std::string> seenHandles;
    for (size_t i = 0; i < humans.size(); ++i) {
        seenHandles.insert(humans[i].name);
        CHECK_EQUAL(1, humans[i].gamesPlayed);
        CHECK_EQUAL(1, humans[i].wins);
        CHECK_EQUAL(0, humans[i].losses);
        CHECK_EQUAL(0, humans[i].games2p3b);
    }
    CHECK_TRUE(seenHandles.count("Unrated1v1Human") == 1);
    CHECK_TRUE(seenHandles.count("Team13HumanA") == 1);
    CHECK_TRUE(seenHandles.count("Team13HumanC") == 1);
    CHECK_TRUE(seenHandles.count("Team24AllyOfBot") == 0);
}

TEST(AccountStoreTest, RankedFirstSortingAcrossLeaderboardArenaAndListUsers)
{
    int64_t pUnplayed = newPlayer("g-rf-unplayed", "Unplayed1200");
    int64_t pActiveHigh = newPlayer("g-rf-high", "Active1216");
    int64_t pActiveLow = newPlayer("g-rf-low", "Active1184");
    (void)pUnplayed;

    std::string err;
    GameRecord g1 = record("g-rf-p1", clockNow + 10, pActiveHigh, pActiveLow, "2P");
    g1.gameBases = 3;
    g1.rated = 1;
    g1.eloPool = 1;
    std::vector<octi::RatingUpdate> u1 = {
        {pActiveHigh, 0, 1216, GameOutcome::WIN},
        {pActiveLow, 0, 1184, GameOutcome::LOSS}
    };
    CHECK_EQUAL((int)ArchiveStatus::OK,
                (int)store->archiveGameWithRatings(g1, u1, {}, err));

    /* 1. Pool 1 leaderboard("2P_3B") puts both ranked players (1216, 1184) ahead of unplayed 1200 (rank=0) */
    std::vector<octi::LeaderboardEntry> lb;
    CHECK_TRUE(store->leaderboard("2P_3B", 10, lb));
    CHECK_EQUAL(3, (int)lb.size());
    STRCMP_EQUAL("Active1216", lb[0].handle.c_str());
    CHECK_EQUAL(1, lb[0].rank);
    CHECK_EQUAL(1, lb[0].ladderGames);
    STRCMP_EQUAL("Active1184", lb[1].handle.c_str());
    CHECK_EQUAL(2, lb[1].rank);
    CHECK_EQUAL(1, lb[1].ladderGames);
    STRCMP_EQUAL("Unplayed1200", lb[2].handle.c_str());
    CHECK_EQUAL(0, lb[2].rank);
    CHECK_EQUAL(0, lb[2].ladderGames);
    CHECK_EQUAL(1200, lb[2].elo);

    /* 2. listUsers DESC and ASC both keep ranked players ahead of 0-game 1200 */
    octi::UserDirectoryFilter uf;
    uf.sortField = "elo_2p_3b";
    uf.sortAsc = false;
    std::vector<octi::UserDirectoryEntry> usersDesc;
    CHECK_TRUE(store->listUsers(uf, usersDesc, nullptr));
    CHECK_EQUAL(3, (int)usersDesc.size());
    STRCMP_EQUAL("Active1216", usersDesc[0].profile.handle.c_str());
    STRCMP_EQUAL("Active1184", usersDesc[1].profile.handle.c_str());
    STRCMP_EQUAL("Unplayed1200", usersDesc[2].profile.handle.c_str());
    CHECK_EQUAL(1, usersDesc[0].profile.games2p3b);
    CHECK_EQUAL(1, usersDesc[1].profile.games2p3b);
    CHECK_EQUAL(0, usersDesc[2].profile.games2p3b);

    uf.sortAsc = true;
    std::vector<octi::UserDirectoryEntry> usersAsc;
    CHECK_TRUE(store->listUsers(uf, usersAsc, nullptr));
    CHECK_EQUAL(3, (int)usersAsc.size());
    STRCMP_EQUAL("Active1184", usersAsc[0].profile.handle.c_str());
    STRCMP_EQUAL("Active1216", usersAsc[1].profile.handle.c_str());
    STRCMP_EQUAL("Unplayed1200", usersAsc[2].profile.handle.c_str());
}

TEST(AccountStoreTest, RecomputeAllRatingsAndPurgeArchivedGameReconciliation)
{
    int64_t h1 = newPlayer("g-rc-h1", "RecompH1");
    int64_t h2 = newPlayer("g-rc-h2", "RecompH2");

    CreateRobotResult b1aRes = store->createRobot(h1, "RecompBot1A", "Owner1 Bot A");
    CreateRobotResult b1bRes = store->createRobot(h1, "RecompBot1B", "Owner1 Bot B");
    CreateRobotResult b2Res = store->createRobot(h2, "RecompBot2", "Owner2 Bot");
    CHECK_EQUAL((int)octi::RobotStatus::OK, (int)b1aRes.status);
    CHECK_EQUAL((int)octi::RobotStatus::OK, (int)b1bRes.status);
    CHECK_EQUAL((int)octi::RobotStatus::OK, (int)b2Res.status);
    int64_t b1a = b1aRes.robot.robotId;
    int64_t b1b = b1bRes.robot.robotId;
    int64_t b2 = b2Res.robot.robotId;

    std::string err;
    int64_t tBase = clockNow - 500000; /* Well in the past relative to wall clock */

    /* 1. Pool 1 2P_3B rated game (h1 beats h2 at tBase + 100) */
    GameRecord gP1 = record("g-rc-pool1-3b", tBase + 100, h1, h2, "2P");
    gP1.gameBases = 3;
    gP1.rated = 1;
    gP1.eloPool = 0; /* Simulate legacy/unrecomputed row */
    gP1.plyCount = 12;
    gP1.result = "P1_WIN";
    gP1.winnerSeat = 1;
    gP1.winner = "A";
    CHECK_EQUAL((int)ArchiveStatus::OK, (int)store->archiveGame(gP1, err));

    /* 2. Early resignation (< 8 plies) with rated = 1 -> must stay eloPool = 0 */
    GameRecord gEarly = record("g-rc-early-resign", tBase + 200, h1, h2, "2P");
    gEarly.gameBases = 3;
    gEarly.rated = 1;
    gEarly.eloPool = 1; /* Even if previously marked 1, recompute resets and skips < 8 ply resignation */
    gEarly.status = "RESIGNED";
    gEarly.termination = "RESIGNATION";
    gEarly.plyCount = 6;
    gEarly.result = "P2_WIN";
    gEarly.winnerSeat = 2;
    gEarly.winner = "B";
    CHECK_EQUAL((int)ArchiveStatus::OK, (int)store->archiveGame(gEarly, err));

    /* 3. Human-vs-Bot (h1 vs b2 on 2P_1B): 5 games within 24h -> ALL 5 rated (K = 32, unlimited!) */
    for (int i = 0; i < 5; ++i) {
        GameRecord gHB = record("g-rc-hb-" + std::to_string(i), tBase + 300 + i * 60, h1, 0, "2P");
        gHB.gameBases = 1;
        gHB.p2RobotId = b2;
        gHB.p2Handle = b2Res.robot.robotName;
        gHB.rated = 1;
        gHB.eloPool = 0;
        gHB.plyCount = 10;
        gHB.result = "P1_WIN";
        gHB.winnerSeat = 1;
        gHB.winner = "A";
        CHECK_EQUAL((int)ArchiveStatus::OK, (int)store->archiveGame(gHB, err));
    }

    /* 4. Same-owner Bot-vs-Bot (b1a vs b1b on OFK): K = 16 */
    GameRecord gSameOwner = record("g-rc-bb-same", tBase + 1000, 0, 0, "OFK");
    gSameOwner.gameBases = 1;
    gSameOwner.p1RobotId = b1a;
    gSameOwner.p1Handle = b1aRes.robot.robotName;
    gSameOwner.p2RobotId = b1b;
    gSameOwner.p2Handle = b1bRes.robot.robotName;
    gSameOwner.rated = 1;
    gSameOwner.eloPool = 0;
    gSameOwner.plyCount = 10;
    gSameOwner.result = "P1_WIN";
    gSameOwner.winnerSeat = 1;
    gSameOwner.winner = "A";
    CHECK_EQUAL((int)ArchiveStatus::OK, (int)store->archiveGame(gSameOwner, err));

    /* 5. Cross-owner Bot-vs-Bot (b1a vs b2 on 2P_3B): 5 games within 86400s + 1 game at +86401s */
    for (int i = 0; i < 5; ++i) {
        bool swap = (i % 2 == 1);
        GameRecord gBB = record("g-rc-bb-" + std::to_string(i), tBase + 2000 + i * 100, 0, 0, "2P");
        gBB.gameBases = 3;
        gBB.p1RobotId = swap ? b2 : b1a;
        gBB.p1Handle = swap ? b2Res.robot.robotName : b1aRes.robot.robotName;
        gBB.p2RobotId = swap ? b1a : b2;
        gBB.p2Handle = swap ? b1aRes.robot.robotName : b2Res.robot.robotName;
        gBB.rated = 1;
        gBB.eloPool = 0;
        gBB.plyCount = 12;
        /* b1a always wins */
        gBB.result = swap ? "P2_WIN" : "P1_WIN";
        gBB.winnerSeat = swap ? 2 : 1;
        gBB.winner = swap ? "B" : "A";
        CHECK_EQUAL((int)ArchiveStatus::OK, (int)store->archiveGame(gBB, err));
    }
    GameRecord gBBAfterWindow = record("g-rc-bb-after-24h", tBase + 2400 + 86401, 0, 0, "2P");
    gBBAfterWindow.gameBases = 3;
    gBBAfterWindow.p1RobotId = b1a;
    gBBAfterWindow.p1Handle = b1aRes.robot.robotName;
    gBBAfterWindow.p2RobotId = b2;
    gBBAfterWindow.p2Handle = b2Res.robot.robotName;
    gBBAfterWindow.rated = 1;
    gBBAfterWindow.eloPool = 0;
    gBBAfterWindow.plyCount = 12;
    gBBAfterWindow.result = "P1_WIN";
    gBBAfterWindow.winnerSeat = 1;
    gBBAfterWindow.winner = "A";
    CHECK_EQUAL((int)ArchiveStatus::OK, (int)store->archiveGame(gBBAfterWindow, err));

    octi::RecomputeRatingsResult rRes;
    CHECK_TRUE_TEXT(store->recomputeAllRatings(rRes, err), err.c_str());
    /* Scanned: 1 (gP1) + 1 (gEarly) + 5 (gHB) + 1 (gSameOwner) + 6 (gBB) = 14 */
    CHECK_EQUAL(14, rRes.gamesScanned);
    CHECK_EQUAL(1, rRes.pool1GamesRated);
    /* Pool 2 rated: 5 (gHB) + 1 (gSameOwner) + 4 (first 4 gBB) + 1 (gBBAfterWindow) = 11 */
    CHECK_EQUAL(11, rRes.pool2GamesRated);
    CHECK_EQUAL(2, rRes.playersUpdated);
    CHECK_EQUAL(3, rRes.robotsUpdated);

    /* Verify early resignation and 5th Bot-vs-Bot game within 24h have eloPool == 0 */
    GameRecord chkEarly, chkBB3, chkBB4, chkBBAfter;
    CHECK_TRUE(store->getArchivedGame("g-rc-early-resign", chkEarly));
    CHECK_EQUAL(0, chkEarly.eloPool);
    CHECK_TRUE(store->getArchivedGame("g-rc-bb-3", chkBB3));
    CHECK_EQUAL(2, chkBB3.eloPool);
    CHECK_TRUE(store->getArchivedGame("g-rc-bb-4", chkBB4));
    CHECK_EQUAL(0, chkBB4.eloPool);
    CHECK_TRUE(store->getArchivedGame("g-rc-bb-after-24h", chkBBAfter));
    CHECK_EQUAL(2, chkBBAfter.eloPool);

    /* Verify Pool 1 Elo & elo_2p mirror + Same-Owner K=16 on OFK */
    PlayerProfile pH1, pH2;
    CHECK_TRUE(store->getProfileById(h1, pH1));
    CHECK_TRUE(store->getProfileById(h2, pH2));
    CHECK_EQUAL(1216, pH1.elo2p3b);
    CHECK_EQUAL(1216, pH1.elo2p);
    CHECK_EQUAL(1, pH1.games2p3b);
    CHECK_EQUAL(1, pH1.gamesPlayed);
    CHECK_EQUAL(5, pH1.arenaGames2p1b);
    CHECK_EQUAL(5, pH1.arenaGamesPlayed);
    CHECK_EQUAL(1184, pH2.elo2p3b);
    CHECK_EQUAL(1184, pH2.elo2p);

    RobotRecord rB1a, rB1b;
    CHECK_TRUE(store->getRobotById(b1a, rB1a));
    CHECK_TRUE(store->getRobotById(b1b, rB1b));
    CHECK_EQUAL(1208, rB1a.eloOfk);
    CHECK_EQUAL(1192, rB1b.eloOfk);
    CHECK_EQUAL(1, rB1a.gamesOfk);
    CHECK_EQUAL(5, rB1a.games2p3b);
    CHECK_EQUAL(7, rB1a.gamesPlayed); /* 1 OFK + 6 2P_3B (5 rated + 1 capped) */

    /* Purging g-rc-bb-0 promotes g-rc-bb-4 into the 4/24h rated window on replay! */
    CHECK_TRUE_TEXT(store->purgeArchivedGame("g-rc-bb-0", err), err.c_str());
    CHECK_TRUE(store->getArchivedGame("g-rc-bb-4", chkBB4));
    CHECK_EQUAL(2, chkBB4.eloPool);
    CHECK_TRUE(store->getRobotById(b1a, rB1a));
    CHECK_EQUAL(6, rB1a.gamesPlayed);
    CHECK_EQUAL(5, rB1a.games2p3b);

    /* Purging g-rc-pool1-3b resets Pool 1 Elos back to 1200 and games_played to 0 */
    CHECK_TRUE_TEXT(store->purgeArchivedGame("g-rc-pool1-3b", err), err.c_str());
    CHECK_TRUE(store->getProfileById(h1, pH1));
    CHECK_EQUAL(1200, pH1.elo2p3b);
    CHECK_EQUAL(1200, pH1.elo2p);
    CHECK_EQUAL(0, pH1.games2p3b);
    CHECK_EQUAL(0, pH1.gamesPlayed);
}

TEST(AccountStoreTest, PerVariantLeaderboardOutcomeIsolationAndNoProgressDraw)
{
    int64_t h1 = newPlayer("g-pv-h1", "VariantAlice");
    int64_t h2 = newPlayer("g-pv-h2", "VariantBob");
    int64_t h3 = newPlayer("g-pv-h3", "VariantCarol");
    int64_t hUnplayed = newPlayer("g-pv-h4", "VariantUnplayed");
    (void)hUnplayed;

    std::string err;

    /* 1. Pool 1 2P_3B win: h1 defeats h2 */
    GameRecord g3bWin = record("g-pv-3b-win", clockNow + 10, h1, h2, "2P");
    g3bWin.gameBases = 3;
    g3bWin.rated = 1;
    g3bWin.eloPool = 1;
    g3bWin.result = "P1_WIN";
    g3bWin.winnerSeat = 1;
    g3bWin.winner = "A";
    CHECK_EQUAL(static_cast<int>(ArchiveStatus::OK),
                static_cast<int>(store->archiveGameWithRatings(
                    g3bWin,
                    {{h1, 0, 1216, GameOutcome::WIN},
                     {h2, 0, 1184, GameOutcome::LOSS}},
                    {}, err)));

    /* 2. Pool 1 2P_3B R7 NO_PROGRESS_DRAW: h1 draws h2 */
    GameRecord g3bDraw = record("g-pv-3b-r7draw", clockNow + 20, h1, h2, "2P");
    g3bDraw.gameBases = 3;
    g3bDraw.rated = 1;
    g3bDraw.eloPool = 1;
    g3bDraw.result = "DRAW";
    g3bDraw.winnerSeat = 0;
    g3bDraw.winner = "NONE";
    g3bDraw.termination = "NO_PROGRESS_DRAW";
    g3bDraw.status = "GAME_OVER";
    CHECK_EQUAL(static_cast<int>(ArchiveStatus::OK),
                static_cast<int>(store->archiveGameWithRatings(
                    g3bDraw,
                    {{h1, 0, 1215, GameOutcome::DRAW},
                     {h2, 0, 1185, GameOutcome::DRAW}},
                    {}, err)));

    /* 3. Pool 1 2P_1B win: h2 defeats h3 */
    GameRecord g1bWin = record("g-pv-1b-win", clockNow + 30, h2, h3, "2P");
    g1bWin.gameBases = 1;
    g1bWin.rated = 1;
    g1bWin.eloPool = 1;
    g1bWin.result = "P1_WIN";
    g1bWin.winnerSeat = 1;
    g1bWin.winner = "A";
    CHECK_EQUAL(static_cast<int>(ArchiveStatus::OK),
                static_cast<int>(store->archiveGameWithRatings(
                    g1bWin,
                    {{h2, 0, 1216, GameOutcome::WIN},
                     {h3, 0, 1184, GameOutcome::LOSS}},
                    {}, err)));

    /* 4. Pool 1 OFK R7 NO_PROGRESS_DRAW: h3 draws h1 */
    GameRecord gOfkDraw = record("g-pv-ofk-r7draw", clockNow + 40, h3, h1, "OFK");
    gOfkDraw.gameBases = 1;
    gOfkDraw.rated = 1;
    gOfkDraw.eloPool = 1;
    gOfkDraw.result = "DRAW";
    gOfkDraw.winnerSeat = 0;
    gOfkDraw.winner = "NONE";
    gOfkDraw.termination = "NO_PROGRESS_DRAW";
    gOfkDraw.status = "GAME_OVER";
    CHECK_EQUAL(static_cast<int>(ArchiveStatus::OK),
                static_cast<int>(store->archiveGameWithRatings(
                    gOfkDraw,
                    {{h3, 0, 1200, GameOutcome::DRAW},
                     {h1, 0, 1200, GameOutcome::DRAW}},
                    {}, err)));

    /* Query 2P_3B: must reflect ONLY 2P_3B games (h1: 1-0-1, h2: 0-1-1, h3: 0-0-0) */
    std::vector<octi::LeaderboardEntry> lb3b;
    CHECK_TRUE(store->leaderboard("2P_3B", 10, lb3b));
    CHECK_EQUAL(4, static_cast<int>(lb3b.size()));
    STRCMP_EQUAL("VariantAlice", lb3b[0].handle.c_str());
    CHECK_EQUAL(1, lb3b[0].rank);
    CHECK_EQUAL(2, lb3b[0].ladderGames);
    CHECK_EQUAL(2, lb3b[0].gamesPlayed);
    CHECK_EQUAL(1, lb3b[0].wins);
    CHECK_EQUAL(0, lb3b[0].losses);
    CHECK_EQUAL(1, lb3b[0].draws);

    STRCMP_EQUAL("VariantBob", lb3b[1].handle.c_str());
    CHECK_EQUAL(2, lb3b[1].rank);
    CHECK_EQUAL(2, lb3b[1].ladderGames);
    CHECK_EQUAL(2, lb3b[1].gamesPlayed);
    CHECK_EQUAL(0, lb3b[1].wins);
    CHECK_EQUAL(1, lb3b[1].losses);
    CHECK_EQUAL(1, lb3b[1].draws);

    /* Unplayed in 2P_3B (h3 played 2P_1B + OFK; hUnplayed played 0) must have 0 variant stats */
    for (size_t i = 2; i < lb3b.size(); ++i) {
        CHECK_EQUAL(0, lb3b[i].rank);
        CHECK_EQUAL(0, lb3b[i].ladderGames);
        CHECK_EQUAL(0, lb3b[i].gamesPlayed);
        CHECK_EQUAL(0, lb3b[i].wins);
        CHECK_EQUAL(0, lb3b[i].losses);
        CHECK_EQUAL(0, lb3b[i].draws);
    }

    /* Query 2P_1B: h2 is 1-0-0, h3 is 0-1-0, h1 is unplayed (0-0-0) */
    std::vector<octi::LeaderboardEntry> lb1b;
    CHECK_TRUE(store->leaderboard("2P_1B", 10, lb1b));
    STRCMP_EQUAL("VariantBob", lb1b[0].handle.c_str());
    CHECK_EQUAL(1, lb1b[0].gamesPlayed);
    CHECK_EQUAL(1, lb1b[0].wins);
    CHECK_EQUAL(0, lb1b[0].losses);
    CHECK_EQUAL(0, lb1b[0].draws);

    STRCMP_EQUAL("VariantCarol", lb1b[1].handle.c_str());
    CHECK_EQUAL(1, lb1b[1].gamesPlayed);
    CHECK_EQUAL(0, lb1b[1].wins);
    CHECK_EQUAL(1, lb1b[1].losses);
    CHECK_EQUAL(0, lb1b[1].draws);

    /* Query OFK: h1 and h3 are each 0-0-1 from NO_PROGRESS_DRAW */
    std::vector<octi::LeaderboardEntry> lbOfk;
    CHECK_TRUE(store->leaderboard("OFK", 10, lbOfk));
    CHECK_EQUAL(1, lbOfk[0].gamesPlayed);
    CHECK_EQUAL(0, lbOfk[0].wins);
    CHECK_EQUAL(0, lbOfk[0].losses);
    CHECK_EQUAL(1, lbOfk[0].draws);
    DOUBLES_EQUAL(0.0, lbOfk[0].winRate, 0.01);
    CHECK_EQUAL(1, lbOfk[1].gamesPlayed);
    CHECK_EQUAL(0, lbOfk[1].wins);
    CHECK_EQUAL(0, lbOfk[1].losses);
    CHECK_EQUAL(1, lbOfk[1].draws);

    /* Direct SQL playedOnly=true + limit=1 check on Pool 1 leaderboard */
    std::vector<octi::LeaderboardEntry> lb3bPlayedOnly;
    int64_t total3bPlayed = 0;
    CHECK_TRUE(store->leaderboard("2P_3B", 1, lb3bPlayedOnly, true, &total3bPlayed));
    CHECK_EQUAL(2, static_cast<int>(total3bPlayed));
    CHECK_EQUAL(1, static_cast<int>(lb3bPlayedOnly.size()));
    STRCMP_EQUAL("VariantAlice", lb3bPlayedOnly[0].handle.c_str());
    DOUBLES_EQUAL(50.0, lb3bPlayedOnly[0].winRate, 0.01);

    /* Pool 1 leaderboardOverall: excludes hUnplayed (0 games), supports sortField + sortAsc */
    std::vector<octi::LeaderboardEntry> lbOverall;
    int64_t totalOverall = 0;
    CHECK_TRUE(store->leaderboardOverall("elo_2p_1b", false, 10, lbOverall, &totalOverall));
    CHECK_EQUAL(3, static_cast<int>(totalOverall));
    CHECK_EQUAL(3, static_cast<int>(lbOverall.size()));
    /* h2 (1216, 1 game in 2P_1B) and h3 (1184, 1 game in 2P_1B) precede h1 (1200, 0 games in 2P_1B) */
    STRCMP_EQUAL("VariantBob", lbOverall[0].handle.c_str());
    STRCMP_EQUAL("VariantCarol", lbOverall[1].handle.c_str());
    STRCMP_EQUAL("VariantAlice", lbOverall[2].handle.c_str());
    CHECK_EQUAL(3, lbOverall[2].gamesPlayed);
    CHECK_EQUAL(2, lbOverall[2].games2p3b);
    CHECK_EQUAL(0, lbOverall[2].games2p1b);
    CHECK_EQUAL(1, lbOverall[2].gamesOfk);
    CHECK_EQUAL(1, lbOverall[2].wins);
    CHECK_EQUAL(0, lbOverall[2].losses);
    CHECK_EQUAL(2, lbOverall[2].draws);
    CHECK_FALSE(store->leaderboardOverall("bad_sort", false, 10, lbOverall, &totalOverall));

    /* Pool 2 listArenaStandings per-variant W/L/D + SQL playedOnly + ladder filter */
    CreateRobotResult botARes = store->createRobot(h1, "PvArenaBotA", "Bot A");
    CreateRobotResult botBRes = store->createRobot(h2, "PvArenaBotB", "Bot B");
    CHECK_EQUAL(static_cast<int>(octi::RobotStatus::OK), static_cast<int>(botARes.status));
    CHECK_EQUAL(static_cast<int>(octi::RobotStatus::OK), static_cast<int>(botBRes.status));
    int64_t botA = botARes.robot.robotId;
    int64_t botB = botBRes.robot.robotId;

    /* Bot A beats h3 in 2P_3B, and Bot A draws Bot B via NO_PROGRESS_DRAW in OFK */
    GameRecord gArena3b = record("g-pv-arena-3b", clockNow + 50, h3, 0, "2P");
    gArena3b.gameBases = 3;
    gArena3b.p2RobotId = botA;
    gArena3b.p2Handle = "PvArenaBotA";
    gArena3b.rated = 1;
    gArena3b.eloPool = 2;
    gArena3b.result = "P2_WIN";
    gArena3b.winnerSeat = 2;
    gArena3b.winner = "B";
    CHECK_EQUAL(static_cast<int>(ArchiveStatus::OK),
                static_cast<int>(store->archiveGameWithRatings(
                    gArena3b,
                    {{h3, 0, 1184, GameOutcome::LOSS},
                     {0, botA, 1216, GameOutcome::WIN}},
                    {{botA, GameOutcome::WIN}}, err)));

    GameRecord gArenaOfk = record("g-pv-arena-ofk-draw", clockNow + 60, 0, 0, "OFK");
    gArenaOfk.gameBases = 1;
    gArenaOfk.p1RobotId = botA;
    gArenaOfk.p1Handle = "PvArenaBotA";
    gArenaOfk.p2RobotId = botB;
    gArenaOfk.p2Handle = "PvArenaBotB";
    gArenaOfk.rated = 1;
    gArenaOfk.eloPool = 2;
    gArenaOfk.result = "DRAW";
    gArenaOfk.winnerSeat = 0;
    gArenaOfk.winner = "NONE";
    gArenaOfk.termination = "NO_PROGRESS_DRAW";
    gArenaOfk.status = "GAME_OVER";
    CHECK_EQUAL(static_cast<int>(ArchiveStatus::OK),
                static_cast<int>(store->archiveGameWithRatings(
                    gArenaOfk,
                    {{0, botA, 1200, GameOutcome::DRAW},
                     {0, botB, 1200, GameOutcome::DRAW}},
                    {{botA, GameOutcome::DRAW},
                     {botB, GameOutcome::DRAW}}, err)));

    octi::ArenaStandingsFilter af;
    af.ladder = "2P_3B";
    af.playedOnly = true;
    af.limit = 1;
    std::vector<octi::ArenaStandingEntry> arena3b;
    int64_t arenaTotal = 0;
    CHECK_TRUE(store->listArenaStandings(af, arena3b, &arenaTotal));
    /* BotB only played OFK, so 2P_3B playedOnly=true has total == 2 (BotA + h3), limit=1 returns BotA with 1-0-0 */
    CHECK_EQUAL(2, static_cast<int>(arenaTotal));
    CHECK_EQUAL(1, static_cast<int>(arena3b.size()));
    STRCMP_EQUAL("PvArenaBotA", arena3b[0].name.c_str());
    CHECK_EQUAL(1, arena3b[0].rank);
    CHECK_EQUAL(1, arena3b[0].gamesPlayed);
    CHECK_EQUAL(1, arena3b[0].wins);
    CHECK_EQUAL(0, arena3b[0].losses);
    CHECK_EQUAL(0, arena3b[0].draws);
    CHECK_EQUAL(1, arena3b[0].drawsOfk);
    DOUBLES_EQUAL(100.0, arena3b[0].winRate, 0.01);

    /* Overall (empty ladder) preserves aggregate counts (BotA: 2 games, 1-0-1) */
    af.ladder.clear();
    af.playedOnly = false;
    af.limit = 10;
    af.sortField = "games_played";
    std::vector<octi::ArenaStandingEntry> arenaAll;
    CHECK_TRUE(store->listArenaStandings(af, arenaAll, &arenaTotal));
    CHECK_EQUAL(3, static_cast<int>(arenaTotal));
    STRCMP_EQUAL("PvArenaBotA", arenaAll[0].name.c_str());
    CHECK_EQUAL(2, arenaAll[0].gamesPlayed);
    CHECK_EQUAL(1, arenaAll[0].wins);
    CHECK_EQUAL(0, arenaAll[0].losses);
    CHECK_EQUAL(1, arenaAll[0].draws);
}

TEST(OidcVerifierTest, GoogleUnverifiedOrMissingEmailVerifiedClearsEmail)
{
    /* Nominal: Google verified email + nonce preserved */
    Json cOk = claimsFor("google", "sub-g-verified");
    cOk["email"] = "admin@selfso.com";
    cOk["email_verified"] = true;
    cOk["nonce"] = "oidc-nonce-xyz-123";
    VerifyResult rOk = verifier->verifyIdToken(
        "google", jwtSign(rsHeader(), cOk, keys().rsa.get()));
    CHECK_STATUS(OK, rOk);
    STRCMP_EQUAL("admin@selfso.com", rOk.claims.email.c_str());
    CHECK_TRUE(rOk.claims.emailVerified);
    STRCMP_EQUAL("oidc-nonce-xyz-123", rOk.claims.nonce.c_str());

    /* Fault 1: Google email_verified == false clears email */
    Json cFalse = claimsFor("google", "sub-g-unverified");
    cFalse["email"] = "admin@selfso.com";
    cFalse["email_verified"] = false;
    VerifyResult rFalse = verifier->verifyIdToken(
        "google", jwtSign(rsHeader(), cFalse, keys().rsa.get()));
    CHECK_STATUS(OK, rFalse);
    STRCMP_EQUAL("", rFalse.claims.email.c_str());
    CHECK_FALSE(rFalse.claims.emailVerified);

    /* Fault 2: Google missing email_verified clears email */
    Json cMissing = claimsFor("google", "sub-g-missing-ev");
    cMissing["email"] = "admin@selfso.com";
    cMissing.erase("email_verified");
    VerifyResult rMissing = verifier->verifyIdToken(
        "google", jwtSign(rsHeader(), cMissing, keys().rsa.get()));
    CHECK_STATUS(OK, rMissing);
    STRCMP_EQUAL("", rMissing.claims.email.c_str());
    CHECK_FALSE(rMissing.claims.emailVerified);

    /* Fault 3: Google string "true" is non-boolean -> rejected as unverified */
    Json cStrTrue = claimsFor("google", "sub-g-str-ev");
    cStrTrue["email"] = "admin@selfso.com";
    cStrTrue["email_verified"] = "true";
    VerifyResult rStrTrue = verifier->verifyIdToken(
        "google", jwtSign(rsHeader(), cStrTrue, keys().rsa.get()));
    CHECK_STATUS(OK, rStrTrue);
    STRCMP_EQUAL("", rStrTrue.claims.email.c_str());
    CHECK_FALSE(rStrTrue.claims.emailVerified);

    /* Apple string "true" vs "false", and Microsoft explicit false vs absent */
    Json cAppleTrue = claimsFor("apple", "sub-a-str-true");
    cAppleTrue["email_verified"] = "true";
    VerifyResult rAppleTrue = verifier->verifyIdToken(
        "apple", jwtSign(rsHeader(), cAppleTrue, keys().rsa.get()));
    CHECK_STATUS(OK, rAppleTrue);
    CHECK_TRUE(rAppleTrue.claims.emailVerified);
    STRCMP_EQUAL("user@example.com", rAppleTrue.claims.email.c_str());

    Json cAppleFalse = claimsFor("apple", "sub-a-str-false");
    cAppleFalse["email_verified"] = "false";
    VerifyResult rAppleFalse = verifier->verifyIdToken(
        "apple", jwtSign(rsHeader(), cAppleFalse, keys().rsa.get()));
    CHECK_STATUS(OK, rAppleFalse);
    CHECK_FALSE(rAppleFalse.claims.emailVerified);
    STRCMP_EQUAL("", rAppleFalse.claims.email.c_str());

    Json cMsFalse = claimsFor("microsoft", "sub-ms-false");
    cMsFalse["email_verified"] = false;
    VerifyResult rMsFalse = verifier->verifyIdToken(
        "microsoft", jwtSign(rsHeader(), cMsFalse, keys().rsa.get()));
    CHECK_STATUS(OK, rMsFalse);
    CHECK_FALSE(rMsFalse.claims.emailVerified);
    STRCMP_EQUAL("", rMsFalse.claims.email.c_str());

    Json cMsAbsent = claimsFor("microsoft", "sub-ms-absent");
    cMsAbsent.erase("email_verified");
    VerifyResult rMsAbsent = verifier->verifyIdToken(
        "microsoft", jwtSign(rsHeader(), cMsAbsent, keys().rsa.get()));
    CHECK_STATUS(OK, rMsAbsent);
    CHECK_FALSE(rMsAbsent.claims.emailVerified);
    STRCMP_EQUAL("user@example.com", rMsAbsent.claims.email.c_str());
}

TEST(AccountStoreTest, VerifiedEmailAdminGateNonHijackBindAndOnlineOnlyRobotStats)
{
    OidcVerifier v;
    v.setClock([]() -> int64_t { return kNow; });
    v.setClientId("google", "client-google");
    v.setKeySet("google", defaultJwks(), kNow);

    store->setAdminGoogleIdentities({"sub-real-admin", "admin@selfso.com"});

    /* 1a. Unverified Google email via OidcVerifier must NOT grant admin */
    Json cUnver = claimsFor("google", "sub-attacker-unverified");
    cUnver["email"] = "admin@selfso.com";
    cUnver["email_verified"] = false;
    VerifyResult vrUnver = v.verifyIdToken(
        "google", jwtSign(rsHeader(), cUnver, keys().rsa.get()));
    CHECK_TRUE(vrUnver.ok());
    LoginResult lrUnver = store->loginWithSso(vrUnver.claims, "test");
    CHECK_TRUE(lrUnver.ok);
    CHECK_FALSE(lrUnver.profile.isAdmin);
    CHECK_FALSE(store->isPlayerAdmin(lrUnver.profile.playerId));

    /* 1b. Direct VerifiedClaims with emailVerified=false stored in DB must NOT grant admin */
    VerifiedClaims rawUnver = makeClaims("google", "sub-direct-unverified",
                                         "Direct Unverified", "admin@selfso.com");
    rawUnver.emailVerified = false;
    LoginResult lrDirectUnver = store->loginWithSso(rawUnver, "test");
    CHECK_TRUE(lrDirectUnver.ok);
    CHECK_FALSE(lrDirectUnver.profile.isAdmin);
    CHECK_FALSE(store->isPlayerAdmin(lrDirectUnver.profile.playerId));
    CHECK_FALSE(lrDirectUnver.identities.empty());
    CHECK_FALSE(lrDirectUnver.identities[0].emailVerified);

    /* 1c. Verified Google email and matching Google sub both grant admin */
    Json cVer = claimsFor("google", "sub-verified-email-admin");
    cVer["email"] = "ADMIN@SELFSO.COM";
    cVer["email_verified"] = true;
    VerifyResult vrVer = v.verifyIdToken(
        "google", jwtSign(rsHeader(), cVer, keys().rsa.get()));
    CHECK_TRUE(vrVer.ok());
    LoginResult lrVer = store->loginWithSso(vrVer.claims, "test");
    CHECK_TRUE(lrVer.ok);
    CHECK_TRUE(lrVer.profile.isAdmin);
    CHECK_TRUE(store->isPlayerAdmin(lrVer.profile.playerId));
    CHECK_TRUE(lrVer.identities[0].emailVerified);

    Json cSubAdmin = claimsFor("google", "sub-real-admin");
    cSubAdmin["email"] = "other@example.com";
    cSubAdmin["email_verified"] = false;
    VerifyResult vrSub = v.verifyIdToken(
        "google", jwtSign(rsHeader(), cSubAdmin, keys().rsa.get()));
    CHECK_TRUE(vrSub.ok());
    LoginResult lrSub = store->loginWithSso(vrSub.claims, "test");
    CHECK_TRUE(lrSub.ok);
    CHECK_TRUE(lrSub.profile.isAdmin);
    CHECK_TRUE(store->isPlayerAdmin(lrSub.profile.playerId));

    /* 2. Legacy linked_identities schema migration backfills email_verified=1 once */
    {
        TempDir migDir;
        std::string migDb = migDir.path() + "/legacy_mig.db";
        std::string migErr;
        {
            AccountStore bootstrap;
            CHECK_TRUE(bootstrap.openOrCreate(migDb, migErr));
            bootstrap.close();
        }
        sqlite3 *raw = nullptr;
        CHECK_EQUAL(SQLITE_OK, sqlite3_open(migDb.c_str(), &raw));
        const char *legacySql =
            "DROP TABLE linked_identities;"
            "CREATE TABLE linked_identities ("
            " identity_id INTEGER PRIMARY KEY AUTOINCREMENT,"
            " player_id INTEGER NOT NULL,"
            " provider TEXT NOT NULL,"
            " provider_sub TEXT NOT NULL,"
            " provider_email TEXT NOT NULL DEFAULT '',"
            " display_hint TEXT NOT NULL DEFAULT '',"
            " linked_at INTEGER NOT NULL,"
            " last_login_at INTEGER NOT NULL,"
            " UNIQUE(provider, provider_sub));"
            "INSERT INTO players(player_id, uuid, handle, display_name, created_at, last_seen_at)"
            " VALUES(1, 'u-leg-1', 'legadmin', 'Legacy Admin', 100, 100);"
            "INSERT INTO linked_identities(player_id, provider, provider_sub, provider_email, linked_at, last_login_at)"
            " VALUES(1, 'google', 'sub-leg-1', 'legacy-admin@selfso.com', 100, 100);";
        CHECK_EQUAL(SQLITE_OK, sqlite3_exec(raw, legacySql, nullptr, nullptr, nullptr));
        sqlite3_close(raw);

        AccountStore migStore;
        CHECK_TRUE(migStore.openOrCreate(migDb, migErr));
        migStore.setAdminGoogleIdentities({"legacy-admin@selfso.com"});
        CHECK_TRUE(migStore.isPlayerAdmin(1));

        VerifiedClaims postMigUnver = makeClaims("google", "sub-post-mig-unver",
                                                 "PostMig", "legacy-admin@selfso.com");
        postMigUnver.emailVerified = false;
        LoginResult lrPost = migStore.loginWithSso(postMigUnver, "test");
        CHECK_TRUE(lrPost.ok);
        CHECK_FALSE(migStore.isPlayerAdmin(lrPost.profile.playerId));
        int64_t postPlayerId = lrPost.profile.playerId;
        migStore.close();

        /* Re-opening must NOT overwrite email_verified=0 on post-migration rows */
        AccountStore migStore2;
        CHECK_TRUE(migStore2.openOrCreate(migDb, migErr));
        migStore2.setAdminGoogleIdentities({"legacy-admin@selfso.com"});
        CHECK_TRUE(migStore2.isPlayerAdmin(1));
        CHECK_FALSE(migStore2.isPlayerAdmin(postPlayerId));
    }

    /* 3. bindInstallToPlayer must not let a second user hijack an existing binding */
    LoginResult lrAlice = login("google", "sub-inst-alice", "Alice");
    LoginResult lrBob = login("google", "sub-inst-bob", "Bob");
    CHECK_TRUE(lrAlice.ok && lrBob.ok);
    CHECK_TRUE(store->bindInstallToPlayer("inst-shared-01", lrAlice.profile.playerId));
    CHECK_TRUE(store->bindInstallToPlayer("inst-shared-01", lrAlice.profile.playerId));
    CHECK_FALSE(store->bindInstallToPlayer("inst-shared-01", lrBob.profile.playerId));
    PlayerProfile ownerProf;
    CHECK_TRUE(store->getPlayerByInstallId("inst-shared-01", ownerProf));
    CHECK_EQUAL(lrAlice.profile.playerId, ownerProf.playerId);

    /* 4. install_tokens registration, boundary lengths, auth checks & atomic delete */
    CHECK_FALSE(AccountStore::isValidInstallToken(""));
    CHECK_FALSE(AccountStore::isValidInstallToken("short15chars123"));
    CHECK_FALSE(AccountStore::isValidInstallToken("has space inside token 1234567890"));
    CHECK_FALSE(AccountStore::isValidInstallToken(std::string(257, 'a')));
    CHECK_TRUE(AccountStore::isValidInstallToken(std::string(16, 'a')));
    const std::string tok32 = "0123456789abcdef0123456789abcdef";
    CHECK_EQUAL(32, static_cast<int>(tok32.size()));
    CHECK_TRUE(AccountStore::isValidInstallToken(tok32));
    CHECK_TRUE(AccountStore::isValidInstallToken(std::string(256, 'z')));

    CHECK_FALSE(store->hasInstallToken("inst-tok-01"));
    CHECK_TRUE(store->registerOrVerifyInstallToken("inst-tok-01", tok32));
    CHECK_TRUE(store->hasInstallToken("inst-tok-01"));
    CHECK_TRUE(store->registerOrVerifyInstallToken("inst-tok-01", tok32));
    CHECK_FALSE(store->registerOrVerifyInstallToken("inst-tok-01", "fedcba9876543210fedcba9876543210"));

    GameRecord gInst = record("g-inst-del-01", clockNow, 0, 0);
    gInst.source = "client_upload";
    gInst.installId = "inst-tok-01";
    std::string err;
    CHECK_EQUAL(static_cast<int>(ArchiveStatus::OK),
                static_cast<int>(store->archiveGame(gInst, err)));
    octi::CrashReportRecord crInst;
    crInst.crashId = "cr-inst-del-01";
    crInst.createdAt = clockNow;
    crInst.platform = "linux-x86_64";
    crInst.exceptionType = "std::runtime_error";
    crInst.installId = "inst-tok-01";
    CHECK_EQUAL(static_cast<int>(ArchiveStatus::STORED),
                static_cast<int>(store->recordCrashReport(crInst, err)));
    CHECK_TRUE(store->bindInstallToPlayer("inst-tok-01", lrAlice.profile.playerId));

    CHECK_EQUAL(static_cast<int>(octi::InstallDeleteAuthStatus::UNAUTHORIZED),
                static_cast<int>(store->verifyInstallDeleteAuth("inst-tok-01", "", 0)));
    CHECK_EQUAL(static_cast<int>(octi::InstallDeleteAuthStatus::FORBIDDEN),
                static_cast<int>(store->verifyInstallDeleteAuth(
                    "inst-tok-01", "fedcba9876543210fedcba9876543210", 0)));
    CHECK_EQUAL(static_cast<int>(octi::InstallDeleteAuthStatus::FORBIDDEN),
                static_cast<int>(store->verifyInstallDeleteAuth(
                    "inst-tok-01", "", lrBob.profile.playerId)));
    CHECK_EQUAL(static_cast<int>(octi::InstallDeleteAuthStatus::AUTHORIZED),
                static_cast<int>(store->verifyInstallDeleteAuth(
                    "inst-tok-01", "", lrAlice.profile.playerId)));
    CHECK_EQUAL(static_cast<int>(octi::InstallDeleteAuthStatus::AUTHORIZED),
                static_cast<int>(store->verifyInstallDeleteAuth("inst-tok-01", tok32, 0)));

    CHECK_EQUAL(2, store->deleteGamesByInstallId("inst-tok-01"));
    /* Re-deleting purged install succeeds with valid 32-char token or admin (0 deleted) */
    CHECK_EQUAL(static_cast<int>(octi::InstallDeleteAuthStatus::AUTHORIZED),
                static_cast<int>(store->verifyInstallDeleteAuth("inst-tok-01", tok32, 0)));
    CHECK_EQUAL(static_cast<int>(octi::InstallDeleteAuthStatus::AUTHORIZED),
                static_cast<int>(store->verifyInstallDeleteAuth(
                    "inst-tok-01", "", lrVer.profile.playerId)));
    CHECK_EQUAL(0, store->deleteGamesByInstallId("inst-tok-01"));

    /* 5. Anonymous client_upload naming a robot must NOT pollute robot stats */
    CreateRobotResult botRes = store->createRobot(lrAlice.profile.playerId, "ArenaShieldBot");
    CHECK_EQUAL(static_cast<int>(RobotStatus::OK), static_cast<int>(botRes.status));

    GameRecord spoofUpload = record("g-spoof-upload-bot", clockNow, 0, 0);
    spoofUpload.source = "client_upload";
    spoofUpload.p1Handle = "ArenaShieldBot";
    spoofUpload.p2Handle = "AI_Hard";
    spoofUpload.result = "P1_WIN";
    spoofUpload.winnerSeat = 1;
    spoofUpload.winner = "A";
    CHECK_EQUAL(static_cast<int>(ArchiveStatus::OK),
                static_cast<int>(store->archiveGame(spoofUpload, err)));
    CHECK_TRUE(store->reconcileRobotStats());
    octi::RecomputeRatingsResult rcRes;
    CHECK_TRUE(store->recomputeAllRatings(rcRes, err));

    RobotRecord botCheck;
    CHECK_TRUE(store->getRobotById(botRes.robot.robotId, botCheck));
    CHECK_EQUAL(0, botCheck.gamesPlayed);
    CHECK_EQUAL(0, botCheck.wins);
    CHECK_EQUAL(0, botCheck.losses);
    GameRecord spoofStored;
    CHECK_TRUE(store->getArchivedGame("g-spoof-upload-bot", spoofStored));
    CHECK_EQUAL(0, spoofStored.p1RobotId);
}

namespace {

std::string stdBase64EncodeBytes(const std::string &raw)
{
    if (raw.empty()) {
        return std::string();
    }
    std::string out(4 * ((raw.size() + 2) / 3), '\0');
    int written = EVP_EncodeBlock(
        reinterpret_cast<unsigned char *>(&out[0]),
        reinterpret_cast<const unsigned char *>(raw.data()),
        static_cast<int>(raw.size()));
    if (written <= 0) {
        return std::string();
    }
    out.resize(static_cast<size_t>(written));
    return out;
}

std::string makePngHeaderBytes(uint32_t width, uint32_t height)
{
    std::string b;
    b.append("\x89PNG\r\n\x1a\n", 8);
    b.append("\x00\x00\x00\x0d", 4);
    b.append("IHDR", 4);
    unsigned char dims[8] = {
        static_cast<unsigned char>((width >> 24) & 0xFF),
        static_cast<unsigned char>((width >> 16) & 0xFF),
        static_cast<unsigned char>((width >> 8) & 0xFF),
        static_cast<unsigned char>(width & 0xFF),
        static_cast<unsigned char>((height >> 24) & 0xFF),
        static_cast<unsigned char>((height >> 16) & 0xFF),
        static_cast<unsigned char>((height >> 8) & 0xFF),
        static_cast<unsigned char>(height & 0xFF)
    };
    b.append(reinterpret_cast<const char *>(dims), 8);
    /* bitDepth=8, colorType=6 (RGBA), compression=0, filter=0, interlace=0 + 4B CRC + IEND */
    b.append("\x08\x06\x00\x00\x00\x00\x00\x00\x00", 9);
    b.append("\x00\x00\x00\x00IEND\xae\x42\x60\x82", 12);
    return b;
}

std::string makeJpegSof0HeaderBytes(uint16_t width, uint16_t height)
{
    std::string b;
    /* SOI + APP0 (JFIF) + SOF0 + EOI */
    b.append("\xff\xd8\xff\xe0\x00\x10JFIF\x00\x01\x01\x00\x00\x01\x00\x01\x00\x00", 20);
    unsigned char sof0[11] = {
        0xFF, 0xC0,
        0x00, 0x0B, /* length = 11 (2 len + 1 prec + 2 H + 2 W + 1 Nf + 3 comp) */
        0x08,       /* precision = 8 */
        static_cast<unsigned char>((height >> 8) & 0xFF),
        static_cast<unsigned char>(height & 0xFF),
        static_cast<unsigned char>((width >> 8) & 0xFF),
        static_cast<unsigned char>(width & 0xFF),
        0x01,       /* 1 component */
        0x01
    };
    b.append(reinterpret_cast<const char *>(sof0), 11);
    b.append("\x11\x00\xff\xd9", 4);
    return b;
}

std::string makeGif89aHeaderBytes(uint16_t width, uint16_t height)
{
    std::string b("GIF89a", 6);
    unsigned char lsd[8] = {
        static_cast<unsigned char>(width & 0xFF),
        static_cast<unsigned char>((width >> 8) & 0xFF),
        static_cast<unsigned char>(height & 0xFF),
        static_cast<unsigned char>((height >> 8) & 0xFF),
        0x00, 0x00, 0x00,
        0x3B /* GIF trailer */
    };
    b.append(reinterpret_cast<const char *>(lsd), 8);
    return b;
}

std::string makeWebpVp8lHeaderBytes(uint16_t width, uint16_t height)
{
    /* RFC 9649 VP8L header: 'RIFF' + size(20) + 'WEBP' + 'VP8L' + chunkSize(8) + 0x2F + 4B packed dims */
    const uint32_t wMinus1 = (width > 0 ? static_cast<uint32_t>(width - 1) : 0u) & 0x3FFFu;
    const uint32_t hMinus1 = (height > 0 ? static_cast<uint32_t>(height - 1) : 0u) & 0x3FFFu;
    const uint32_t packed = wMinus1 | (hMinus1 << 14);
    unsigned char raw[28] = {
        'R', 'I', 'F', 'F',
        20, 0, 0, 0,
        'W', 'E', 'B', 'P',
        'V', 'P', '8', 'L',
        8, 0, 0, 0,
        0x2F,
        static_cast<unsigned char>(packed & 0xFFu),
        static_cast<unsigned char>((packed >> 8) & 0xFFu),
        static_cast<unsigned char>((packed >> 16) & 0xFFu),
        static_cast<unsigned char>((packed >> 24) & 0xFFu),
        0x00, 0x00, 0x00
    };
    return std::string(reinterpret_cast<const char *>(raw), sizeof(raw));
}

} // namespace

TEST(AccountStoreTest, Envelope3AvatarSvgRejectionAndRasterWebpBounds)
{
    std::string avDir = dir->path() + "/e3_avatars";
    store->setAvatarDir(avDir);
    int64_t pid = newPlayer("sub-e3-av", "Envelope3 Avatar User");

    /* 1. SVG data URI rejected in both validAvatarUrl and updateProfile */
    const std::string svgXssUri =
        "data:image/svg+xml;base64," +
        stdBase64EncodeBytes("<svg xmlns='http://www.w3.org/2000/svg'><script>alert(1)</script></svg>");
    CHECK_FALSE(AccountStore::validAvatarUrl(svgXssUri));

    ProfileUpdate u;
    u.setAvatarUrl = true;
    u.avatarUrl = svgXssUri;
    CHECK_EQUAL(static_cast<int>(UpdateStatus::INVALID_AVATAR_URL),
                static_cast<int>(store->updateProfile(pid, u)));

    /* 2. Fake PNG data URI wrapping HTML/script payload rejected */
    u.avatarUrl = "data:image/png;base64," +
                  stdBase64EncodeBytes("<script>alert('xss')</script>");
    CHECK_EQUAL(static_cast<int>(UpdateStatus::INVALID_AVATAR_URL),
                static_cast<int>(store->updateProfile(pid, u)));

    /* 3. Boundary: 1024x1024 raster headers accepted vs 1025x1024 / 0x10 / 16383x16383 rejected */
    u.avatarUrl = "data:image/png;base64," + stdBase64EncodeBytes(makePngHeaderBytes(1024, 1024));
    CHECK_EQUAL(static_cast<int>(UpdateStatus::OK),
                static_cast<int>(store->updateProfile(pid, u)));

    u.avatarUrl = "data:image/png;base64," + stdBase64EncodeBytes(makePngHeaderBytes(1025, 1024));
    CHECK_EQUAL(static_cast<int>(UpdateStatus::INVALID_AVATAR_URL),
                static_cast<int>(store->updateProfile(pid, u)));

    u.avatarUrl = "data:image/png;base64," + stdBase64EncodeBytes(makePngHeaderBytes(0, 64));
    CHECK_EQUAL(static_cast<int>(UpdateStatus::INVALID_AVATAR_URL),
                static_cast<int>(store->updateProfile(pid, u)));

    u.avatarUrl = "data:image/jpeg;base64," + stdBase64EncodeBytes(makeJpegSof0HeaderBytes(1024, 1024));
    CHECK_EQUAL(static_cast<int>(UpdateStatus::OK),
                static_cast<int>(store->updateProfile(pid, u)));

    u.avatarUrl = "data:image/jpeg;base64," + stdBase64EncodeBytes(makeJpegSof0HeaderBytes(1024, 1025));
    CHECK_EQUAL(static_cast<int>(UpdateStatus::INVALID_AVATAR_URL),
                static_cast<int>(store->updateProfile(pid, u)));

    u.avatarUrl = "data:image/gif;base64," + stdBase64EncodeBytes(makeGif89aHeaderBytes(1024, 1024));
    CHECK_EQUAL(static_cast<int>(UpdateStatus::OK),
                static_cast<int>(store->updateProfile(pid, u)));

    u.avatarUrl = "data:image/gif;base64," + stdBase64EncodeBytes(makeGif89aHeaderBytes(1025, 1024));
    CHECK_EQUAL(static_cast<int>(UpdateStatus::INVALID_AVATAR_URL),
                static_cast<int>(store->updateProfile(pid, u)));

    /* SEC-15: 16383x16383 WebP bomb header rejected before WebPDecodeRGBA */
    const std::string webpBomb = makeWebpVp8lHeaderBytes(16383, 16383);
    std::string outPng;
    CHECK_FALSE(AccountStore::validateRasterAvatarBytes("webp", webpBomb));
    CHECK_FALSE(AccountStore::transcodeWebpToPng(webpBomb, outPng));
    u.avatarUrl = "data:image/webp;base64," + stdBase64EncodeBytes(webpBomb);
    CHECK_EQUAL(static_cast<int>(UpdateStatus::INVALID_AVATAR_URL),
                static_cast<int>(store->updateProfile(pid, u)));

    /* Direct validateRasterAvatarBytes nominal, boundary & format-confusion checks */
    CHECK_TRUE(AccountStore::validateRasterAvatarBytes("webp", makeWebpVp8lHeaderBytes(1, 1)));
    CHECK_TRUE(AccountStore::validateRasterAvatarBytes("webp", makeWebpVp8lHeaderBytes(1024, 1024)));
    CHECK_FALSE(AccountStore::validateRasterAvatarBytes("webp", makeWebpVp8lHeaderBytes(1025, 1024)));
    CHECK_FALSE(AccountStore::validateRasterAvatarBytes("webp", makeWebpVp8lHeaderBytes(1024, 1025)));
    CHECK_FALSE(AccountStore::validateRasterAvatarBytes("svg", "<svg></svg>"));
    CHECK_FALSE(AccountStore::validateRasterAvatarBytes("png", makeGif89aHeaderBytes(16, 16)));
    CHECK_FALSE(AccountStore::validateRasterAvatarBytes("jpg", makePngHeaderBytes(16, 16)));
    CHECK_FALSE(AccountStore::validateRasterAvatarBytes("webp", webpBomb.substr(12))); /* raw VP8L without RIFF..WEBP */
    CHECK_FALSE(AccountStore::validateRasterAvatarBytes("png", ""));
    CHECK_FALSE(AccountStore::validateRasterAvatarBytes("png", std::string(AccountStore::kMaxAvatarDataUriBytes + 1, 'x')));

    /* Legacy >1024x1024 .webp file on disk rejected safely by readAvatarFile */
    {
        std::ofstream bombOut((avDir + "/77777.webp").c_str(),
                              std::ios::binary | std::ios::trunc);
        bombOut.write(webpBomb.data(), webpBomb.size());
    }
    std::string readBytes;
    std::string readMime;
    CHECK_FALSE(store->readAvatarFile("77777.webp", readBytes, readMime));
}

TEST(AccountStoreTest, Envelope3DisallowedProbeAddressUnitAndRapidCheck)
{
    using octi::OctiHttpServer;
    std::string reason;

    /* 1. Explicit SSRF bogon vectors (string & sockaddr_in/sockaddr_in6) */
    const char *mustReject[] = {
        "127.0.0.1",
        "127.255.255.254",
        "0.0.0.0",
        "0.1.2.3",
        "10.0.0.1",
        "100.64.0.1",
        "100.127.255.254",
        "169.254.169.254",
        "172.16.0.1",
        "172.31.255.254",
        "192.0.0.1",
        "192.0.2.1",
        "192.88.99.1",
        "192.168.1.1",
        "198.18.0.1",
        "198.51.100.1",
        "203.0.113.1",
        "224.0.0.1",
        "239.255.255.250",
        "240.0.0.1",
        "255.255.255.255",
        "::",
        "::1",
        "::ffff:127.0.0.1",
        "::ffff:169.254.169.254",
        "::ffff:8.8.8.8",
        "::127.0.0.1",
        "64:ff9b::7f00:1",
        "64:ff9b::a9fe:a9fe",
        "64:ff9b:1::7f00:1",
        "100::1",
        "2001:0000:4136:e378:8000:63bf:3fff:fdd2",
        "2001:2::1",
        "2001:10::1",
        "2001:db8::1",
        "2002:7f00:0001::1",
        "2002:a9fe:a9fe::1",
        "fc00::1",
        "fd12:3456:789a::1",
        "fe80::1",
        "fe80::1%eth0",
        "fec0::1",
        "ff02::1",
        "",
        "not-an-ip-literal"
    };
    for (size_t i = 0; i < sizeof(mustReject) / sizeof(mustReject[0]); ++i) {
        reason.clear();
        CHECK_TRUE_TEXT(
            OctiHttpServer::isDisallowedProbeAddress(mustReject[i], reason, false),
            mustReject[i]);
        CHECK_FALSE(reason.empty());
    }

    /* Loopback test flag allows ONLY native 127.0.0.0/8 and ::1, NEVER ::ffff:/NAT64/6to4 */
    CHECK_TRUE(OctiHttpServer::isDisallowedProbeAddress("127.0.0.1", reason, false));
    CHECK_FALSE(OctiHttpServer::isDisallowedProbeAddress("127.0.0.1", reason, true));
    CHECK_TRUE(OctiHttpServer::isDisallowedProbeAddress("::1", reason, false));
    CHECK_FALSE(OctiHttpServer::isDisallowedProbeAddress("::1", reason, true));
    CHECK_TRUE(OctiHttpServer::isDisallowedProbeAddress("::ffff:127.0.0.1", reason, true));
    CHECK_TRUE(OctiHttpServer::isDisallowedProbeAddress("64:ff9b::7f00:1", reason, true));
    CHECK_TRUE(OctiHttpServer::isDisallowedProbeAddress("2002:7f00:0001::1", reason, true));

    /* Global unicast IPv4 and IPv6 addresses are permitted */
    const char *mustAllow[] = {
        "1.1.1.1",
        "8.8.8.8",
        "93.184.216.34",
        "100.63.255.255",
        "100.128.0.1",
        "172.15.255.254",
        "172.32.0.1",
        "223.255.255.254",
        "2606:4700:4700::1111",
        "2001:4860:4860::8888",
        "2a00:1450:4001:81b::200e"
    };
    for (size_t i = 0; i < sizeof(mustAllow) / sizeof(mustAllow[0]); ++i) {
        CHECK_FALSE_TEXT(
            OctiHttpServer::isDisallowedProbeAddress(mustAllow[i], reason, false),
            mustAllow[i]);
    }

    /* Truncated / null / bad-family sockaddr fail closed */
    CHECK_TRUE(OctiHttpServer::isDisallowedProbeAddress(nullptr, 0, reason, false));
    struct sockaddr_in shortSin;
    std::memset(&shortSin, 0, sizeof(shortSin));
    shortSin.sin_family = AF_INET;
    inet_pton(AF_INET, "8.8.8.8", &shortSin.sin_addr);
    CHECK_TRUE(OctiHttpServer::isDisallowedProbeAddress(
        reinterpret_cast<const struct sockaddr *>(&shortSin),
        sizeof(shortSin) - 1, reason, false));
    struct sockaddr_in6 scopedSin6;
    std::memset(&scopedSin6, 0, sizeof(scopedSin6));
    scopedSin6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "2606:4700:4700::1111", &scopedSin6.sin6_addr);
    scopedSin6.sin6_scope_id = 2;
    CHECK_TRUE(OctiHttpServer::isDisallowedProbeAddress(
        reinterpret_cast<const struct sockaddr *>(&scopedSin6),
        sizeof(scopedSin6), reason, false));

    /* 2. RapidCheck property suites on IPv4/IPv6/mapped/NAT64/6to4/Teredo bogons */
    bool v4BogonOk = rc::check(
        "IPv4 private, loopback, CGNAT, link-local, multicast, and reserved blocks always disallowed",
        []() {
            int bucket = *rc::gen::inRange(0, 8);
            uint32_t rawHost = *rc::gen::arbitrary<uint32_t>();
            uint32_t ip = 0;
            switch (bucket) {
            case 0: ip = 0x00000000u | (rawHost & 0x00FFFFFFu); break; /* 0.0.0.0/8 */
            case 1: ip = 0x0A000000u | (rawHost & 0x00FFFFFFu); break; /* 10.0.0.0/8 */
            case 2: ip = 0x64400000u | (rawHost & 0x003FFFFFu); break; /* 100.64.0.0/10 */
            case 3: ip = 0x7F000000u | (rawHost & 0x00FFFFFFu); break; /* 127.0.0.0/8 */
            case 4: ip = 0xA9FE0000u | (rawHost & 0x0000FFFFu); break; /* 169.254.0.0/16 */
            case 5: ip = 0xAC100000u | (rawHost & 0x000FFFFFu); break; /* 172.16.0.0/12 */
            case 6: ip = 0xC0A80000u | (rawHost & 0x0000FFFFu); break; /* 192.168.0.0/16 */
            default: ip = 0xE0000000u | (rawHost & 0x1FFFFFFFu); break; /* >= 224.0.0.0 */
            }
            struct sockaddr_in sin;
            std::memset(&sin, 0, sizeof(sin));
            sin.sin_family = AF_INET;
            sin.sin_addr.s_addr = htonl(ip);
            std::string err;
            RC_ASSERT(OctiHttpServer::isDisallowedProbeAddress(
                reinterpret_cast<const struct sockaddr *>(&sin),
                sizeof(sin), err, false));
        });
    CHECK_TRUE(v4BogonOk);

    bool v6TransitionOk = rc::check(
        "IPv4-mapped (::ffff:0:0/96), NAT64 (64:ff9b::/96 & 64:ff9b:1::/48), 6to4 (2002::/16), and Teredo (2001::/32) always disallowed",
        []() {
            int kind = *rc::gen::inRange(0, 5);
            uint32_t v4 = *rc::gen::arbitrary<uint32_t>();
            uint64_t tail = *rc::gen::arbitrary<uint64_t>();
            struct sockaddr_in6 sin6;
            std::memset(&sin6, 0, sizeof(sin6));
            sin6.sin6_family = AF_INET6;
            unsigned char *b = sin6.sin6_addr.s6_addr;

            if (kind == 0) {
                /* ::ffff:0:0/96 */
                b[10] = 0xFF;
                b[11] = 0xFF;
                b[12] = static_cast<unsigned char>((v4 >> 24) & 0xFF);
                b[13] = static_cast<unsigned char>((v4 >> 16) & 0xFF);
                b[14] = static_cast<unsigned char>((v4 >> 8) & 0xFF);
                b[15] = static_cast<unsigned char>(v4 & 0xFF);
            } else if (kind == 1) {
                /* 64:ff9b::/96 */
                b[0] = 0x00; b[1] = 0x64; b[2] = 0xFF; b[3] = 0x9B;
                b[12] = static_cast<unsigned char>((v4 >> 24) & 0xFF);
                b[13] = static_cast<unsigned char>((v4 >> 16) & 0xFF);
                b[14] = static_cast<unsigned char>((v4 >> 8) & 0xFF);
                b[15] = static_cast<unsigned char>(v4 & 0xFF);
            } else if (kind == 2) {
                /* 64:ff9b:1::/48 */
                b[0] = 0x00; b[1] = 0x64; b[2] = 0xFF; b[3] = 0x9B;
                b[4] = 0x00; b[5] = 0x01;
                b[12] = static_cast<unsigned char>((v4 >> 24) & 0xFF);
                b[13] = static_cast<unsigned char>((v4 >> 16) & 0xFF);
                b[14] = static_cast<unsigned char>((v4 >> 8) & 0xFF);
                b[15] = static_cast<unsigned char>(v4 & 0xFF);
            } else if (kind == 3) {
                /* 2002::/16 */
                b[0] = 0x20; b[1] = 0x02;
                b[2] = static_cast<unsigned char>((v4 >> 24) & 0xFF);
                b[3] = static_cast<unsigned char>((v4 >> 16) & 0xFF);
                b[4] = static_cast<unsigned char>((v4 >> 8) & 0xFF);
                b[5] = static_cast<unsigned char>(v4 & 0xFF);
                for (int i = 0; i < 8; ++i) {
                    b[8 + i] = static_cast<unsigned char>((tail >> (i * 8)) & 0xFF);
                }
            } else {
                /* 2001::/32 Teredo */
                b[0] = 0x20; b[1] = 0x01; b[2] = 0x00; b[3] = 0x00;
                b[4] = static_cast<unsigned char>((v4 >> 24) & 0xFF);
                b[5] = static_cast<unsigned char>((v4 >> 16) & 0xFF);
                b[6] = static_cast<unsigned char>((v4 >> 8) & 0xFF);
                b[7] = static_cast<unsigned char>(v4 & 0xFF);
                for (int i = 0; i < 8; ++i) {
                    b[8 + i] = static_cast<unsigned char>((tail >> (i * 8)) & 0xFF);
                }
            }
            std::string err;
            RC_ASSERT(OctiHttpServer::isDisallowedProbeAddress(
                reinterpret_cast<const struct sockaddr *>(&sin6),
                sizeof(sin6), err, true));
        });
    CHECK_TRUE(v6TransitionOk);

    bool v6LocalAndDocOk = rc::check(
        "IPv6 ULA, link-local, documentation, and multicast blocks always disallowed",
        []() {
            int kind = *rc::gen::inRange(0, 4);
            std::array<uint8_t, 16> raw;
            for (size_t i = 0; i < 16; ++i) {
                raw[i] = *rc::gen::arbitrary<uint8_t>();
            }
            if (kind == 0) {
                raw[0] = static_cast<uint8_t>(0xFC | (raw[0] & 0x01)); /* fc00::/7 */
            } else if (kind == 1) {
                raw[0] = 0xFE;
                raw[1] = static_cast<uint8_t>(0x80 | (raw[1] & 0x3F)); /* fe80::/10 */
            } else if (kind == 2) {
                raw[0] = 0x20; raw[1] = 0x01; raw[2] = 0x0D; raw[3] = 0xB8; /* 2001:db8::/32 */
            } else {
                raw[0] = 0xFF; /* ff00::/8 */
            }
            struct sockaddr_in6 sin6;
            std::memset(&sin6, 0, sizeof(sin6));
            sin6.sin6_family = AF_INET6;
            std::memcpy(sin6.sin6_addr.s6_addr, raw.data(), 16);
            std::string err;
            RC_ASSERT(OctiHttpServer::isDisallowedProbeAddress(
                reinterpret_cast<const struct sockaddr *>(&sin6),
                sizeof(sin6), err, false));
        });
    CHECK_TRUE(v6LocalAndDocOk);

    bool fuzzStringsOk = rc::check(
        "isDisallowedProbeAddress string overload never crashes and fails closed on non-IPs",
        []() {
            std::string s = *rc::gen::arbitrary<std::string>();
            std::string err;
            bool disallowed = OctiHttpServer::isDisallowedProbeAddress(s, err, false);
            if (!disallowed) {
                RC_ASSERT(!OctiHttpServer::normalizeIpLiteral(s).empty());
            } else {
                RC_ASSERT(!err.empty());
            }
        });
    CHECK_TRUE(fuzzStringsOk);
}

TEST(AccountStoreTest, Envelope4SessionCapEvictsOldestBeyond32)
{
    std::vector<LoginResult> logins;
    for (int i = 0; i < 35; ++i) {
        clockNow = kNow + i;
        LoginResult r = login("google", "g-cap-user");
        CHECK_TRUE(r.ok);
        logins.push_back(r);
    }
    int64_t pid = logins[0].profile.playerId;
    CHECK_EQUAL(32, store->countActiveSessions(pid));
    CHECK_FALSE(store->validateSession(logins[0].sessionToken, clockNow).valid);
    CHECK_FALSE(store->validateSession(logins[1].sessionToken, clockNow).valid);
    CHECK_FALSE(store->validateSession(logins[2].sessionToken, clockNow).valid);
    for (size_t i = 3; i < logins.size(); ++i) {
        CHECK_TRUE(store->validateSession(logins[i].sessionToken, clockNow).valid);
    }
}

TEST(AccountStoreTest, Envelope4KeysetCursorPaginationAndCrashChatRetention)
{
    /* 1. Verify idx_games_export_cursor exists */
    CHECK_EQUAL(1, countRows("SELECT COUNT(*) FROM sqlite_master"
                             " WHERE type='index' AND name='idx_games_export_cursor'"));

    /* 2. Insert 7 games including tied ended_at timestamps + client.extended metadata */
    int64_t pA = newPlayer("g-cur-A", "CurAlpha");
    int64_t pB = newPlayer("g-cur-B", "CurBeta");
    const int64_t endTimes[7] = {
        kNow + 10, kNow + 20, kNow + 30, kNow + 30, kNow + 30, kNow + 40, kNow + 50
    };
    const char *ids[7] = {
        "g-cur-01", "g-cur-02", "g-cur-03a", "g-cur-03b", "g-cur-03c", "g-cur-04", "g-cur-05"
    };
    for (int i = 0; i < 7; ++i) {
        GameRecord g = record(ids[i], endTimes[i], pA, pB, "2P");
        Json tj = Json::parse(g.octiTranscript);
        tj["client"] = Json{
            {"appVersion", "2.6.3"},
            {"platform", "linux"},
            {"extended", Json{{"installId", "priv-inst-secret"}}}};
        /* Pass pretty-printed multi-line JSON with client.extended */
        g.octiTranscript = tj.dump(2);
        std::string err;
        CHECK_EQUAL(static_cast<int>(ArchiveStatus::OK),
                    static_cast<int>(store->archiveGame(g, err)));
    }

    /* Walk DESC with limit=2 via keyset cursor (afterEndedAt, afterGameId) */
    std::vector<std::string> walkedDesc;
    ArchiveFilter fDesc;
    fDesc.includeTranscript = true;
    fDesc.limit = 2;
    fDesc.maxLimit = 2000;
    while (true) {
        std::vector<GameRecord> page;
        CHECK_TRUE(store->listArchivedGames(fDesc, page));
        if (page.empty()) {
            break;
        }
        for (size_t i = 0; i < page.size(); ++i) {
            CHECK_TRUE(page[i].octiTranscript.find('\n') == std::string::npos);
            CHECK_TRUE(page[i].octiTranscript.find('\r') == std::string::npos);
            CHECK_TRUE(page[i].octiTranscript.find("priv-inst-secret") ==
                       std::string::npos);
            CHECK_TRUE(page[i].octiTranscript.find("\"extended\"") ==
                       std::string::npos);
            CHECK_TRUE(page[i].octiTranscript.find("\"license\":\"CC-BY-4.0\"") !=
                       std::string::npos);
            walkedDesc.push_back(page[i].gameId);
        }
        if (static_cast<int>(page.size()) < fDesc.limit) {
            break;
        }
        fDesc.hasCursor = true;
        fDesc.afterEndedAt = page.back().endedAt;
        fDesc.afterGameId = page.back().gameId;
    }
    CHECK_EQUAL(7, static_cast<int>(walkedDesc.size()));
    const char *expectedDesc[7] = {
        "g-cur-05", "g-cur-04", "g-cur-03c", "g-cur-03b", "g-cur-03a", "g-cur-02", "g-cur-01"
    };
    for (int i = 0; i < 7; ++i) {
        STRCMP_EQUAL(expectedDesc[i], walkedDesc[static_cast<size_t>(i)].c_str());
    }

    /* Walk ASC with limit=3 via keyset cursor */
    std::vector<std::string> walkedAsc;
    ArchiveFilter fAsc;
    fAsc.includeTranscript = true;
    fAsc.sortAsc = true;
    fAsc.limit = 3;
    while (true) {
        std::vector<GameRecord> page;
        CHECK_TRUE(store->listArchivedGames(fAsc, page));
        if (page.empty()) {
            break;
        }
        for (size_t i = 0; i < page.size(); ++i) {
            walkedAsc.push_back(page[i].gameId);
        }
        if (static_cast<int>(page.size()) < fAsc.limit) {
            break;
        }
        fAsc.hasCursor = true;
        fAsc.afterEndedAt = page.back().endedAt;
        fAsc.afterGameId = page.back().gameId;
    }
    CHECK_EQUAL(7, static_cast<int>(walkedAsc.size()));
    for (int i = 0; i < 7; ++i) {
        STRCMP_EQUAL(ids[i], walkedAsc[static_cast<size_t>(i)].c_str());
    }

    /* 3. purgeOldCrashReports enforces both cutoffSec and maxRows */
    for (int i = 0; i < 8; ++i) {
        clockNow = kNow + i * 100;
        CrashReportRecord cr;
        cr.crashId = "cr-ret-" + std::to_string(i);
        cr.createdAt = clockNow;
        cr.platform = "linux";
        cr.appVersion = "2.6.3";
        cr.exceptionType = "SIGSEGV";
        cr.exceptionMessage = "crash " + std::to_string(i);
        std::string err;
        CHECK_EQUAL(static_cast<int>(ArchiveStatus::STORED),
                    static_cast<int>(store->recordCrashReport(cr, err)));
    }
    /* 3 rows have uploaded_at < kNow + 300; remaining 5 get capped to maxRows = 3 -> 5 deleted */
    int purgedCrashes = store->purgeOldCrashReports(kNow + 300, 3);
    CHECK_EQUAL(5, purgedCrashes);
    CrashReportFilter cf;
    cf.limit = 20;
    std::vector<CrashReportRecord> remCrashes;
    int64_t totalCrashes = 0;
    CHECK_TRUE(store->listCrashReports(cf, remCrashes, &totalCrashes));
    CHECK_EQUAL(3, static_cast<int>(totalCrashes));
    STRCMP_EQUAL("cr-ret-7", remCrashes[0].crashId.c_str());
    STRCMP_EQUAL("cr-ret-6", remCrashes[1].crashId.c_str());
    STRCMP_EQUAL("cr-ret-5", remCrashes[2].crashId.c_str());

    /* 4. purgeLobbyChatLogsBefore enforces both cutoffEpochMs and maxRows */
    for (int i = 1; i <= 8; ++i) {
        CHECK_TRUE(store->appendLobbyChatLog(
            static_cast<uint64_t>(i), "u-1", "alice",
            "chat " + std::to_string(i), 1000LL * i));
    }
    CHECK_TRUE(store->purgeLobbyChatLogsBefore(4000LL, 3));
    std::vector<LobbyChatLogRow> chatRows;
    CHECK_TRUE(store->listLobbyChatLogs(chatRows, 20));
    CHECK_EQUAL(3, static_cast<int>(chatRows.size()));
    CHECK_EQUAL(6u, static_cast<unsigned>(chatRows[0].seq));
    CHECK_EQUAL(7u, static_cast<unsigned>(chatRows[1].seq));
    CHECK_EQUAL(8u, static_cast<unsigned>(chatRows[2].seq));
}

TEST(AccountStoreTest, Envelope5BannedPlayerSessionAndSsoEnforcement)
{
    LoginResult lr1 = login("google", "g-e5-banned-user", "BannedUser");
    CHECK_TRUE(lr1.ok);
    const int64_t pid = lr1.profile.playerId;
    CHECK_TRUE(store->validateSession(lr1.sessionToken, clockNow).valid);

    CreateRobotResult bot = store->createRobot(pid, "e5_banned_owner_bot", "Owner bot");
    CHECK_EQUAL(static_cast<int>(RobotStatus::OK), static_cast<int>(bot.status));
    CHECK_TRUE(store->validateRobotToken(bot.rawToken).valid);

    /* Banning player must immediately invalidate existing sessions, SSO logins, and robot tokens */
    CHECK_TRUE(store->setBanned(pid, true));

    octi::SessionValidation svBanned = store->validateSession(lr1.sessionToken, clockNow);
    CHECK_FALSE(svBanned.valid);
    CHECK_TRUE(svBanned.banned);
    STRCMP_EQUAL("banned", svBanned.reason.c_str());

    /* Attempting SSO login while banned must fail closed without minting a session */
    LoginResult lrBanned = login("google", "g-e5-banned-user", "BannedUser");
    CHECK_FALSE(lrBanned.ok);
    CHECK_TRUE(lrBanned.banned);
    CHECK_TRUE(lrBanned.sessionToken.empty());

    RobotTokenValidation rvBanned = store->validateRobotToken(bot.rawToken);
    CHECK_FALSE(rvBanned.valid);
    CHECK_TRUE(rvBanned.banned);

    /* Even after revokeAllSessions, a banned player's session reports banned=true */
    store->revokeAllSessions(pid);
    octi::SessionValidation svRevokedAndBanned =
        store->validateSession(lr1.sessionToken, clockNow);
    CHECK_FALSE(svRevokedAndBanned.valid);
    CHECK_TRUE(svRevokedAndBanned.banned);
    STRCMP_EQUAL("banned", svRevokedAndBanned.reason.c_str());

    /* Unbanning restores SSO login and robot token validity */
    CHECK_TRUE(store->setBanned(pid, false));
    CHECK_TRUE(store->validateRobotToken(bot.rawToken).valid);
    LoginResult lrRestored = login("google", "g-e5-banned-user", "BannedUser");
    CHECK_TRUE(lrRestored.ok);
    CHECK_FALSE(lrRestored.sessionToken.empty());
    CHECK_TRUE(store->validateSession(lrRestored.sessionToken, clockNow).valid);
}

TEST(AccountStoreTest, Envelope5TempBansAuditLogAndAbuseDetectorUnit)
{
    LoginResult lrTemp = login("google", "g-e5-temp-acct", "TempAcct");
    LoginResult lrPerm = login("apple", "a-e5-perm-acct", "PermAcct");
    LoginResult lrAdmin = login("google", "g-e5-unit-admin", "UnitAdmin");
    CHECK_TRUE(lrTemp.ok && lrPerm.ok && lrAdmin.ok);
    store->setAdminGoogleIdentities({"g-e5-unit-admin"});
    CHECK_TRUE(store->isPlayerAdmin(lrAdmin.profile.playerId));

    CreateRobotResult rBot =
        store->createRobot(lrPerm.profile.playerId, "e5_unit_bot", "Bot");
    CHECK_EQUAL(static_cast<int>(RobotStatus::OK), static_cast<int>(rBot.status));

    /* 1. temp_bans persistence + purgeExpiredTempBans respects permanentAccountUuids */
    CHECK_TRUE(store->setBanned(lrTemp.profile.playerId, true));
    CHECK_TRUE(store->setBanned(lrPerm.profile.playerId, true));

    octi::TempBanRow row1;
    row1.banId = "ban_24h_unit_1";
    row1.targetType = "account";
    row1.targetValue = lrTemp.profile.uuid;
    row1.displayLabel = "@" + lrTemp.profile.handle;
    row1.reason = "temp violation";
    row1.source = "auto";
    row1.createdBy = "system";
    row1.createdAt = clockNow;
    row1.expiresAt = clockNow + 86400;
    CHECK_TRUE(store->upsertTempBan(row1));

    octi::TempBanRow row2 = row1;
    row2.banId = "ban_24h_unit_2";
    row2.targetValue = lrPerm.profile.uuid;
    row2.displayLabel = "@" + lrPerm.profile.handle;
    CHECK_TRUE(store->upsertTempBan(row2));

    std::vector<octi::TempBanRow> listed;
    CHECK_TRUE(store->listTempBans(listed));
    CHECK_EQUAL(2, static_cast<int>(listed.size()));

    /* Before expiry: nothing purged */
    CHECK_EQUAL(0, store->purgeExpiredTempBans(clockNow + 86399, {lrPerm.profile.uuid}));

    /* At expiry: both temp rows purged; lrTemp unbanned, lrPerm remains banned due to perm list */
    CHECK_EQUAL(2, store->purgeExpiredTempBans(clockNow + 86400, {lrPerm.profile.uuid}));
    PlayerProfile pTempAfter;
    PlayerProfile pPermAfter;
    CHECK_TRUE(store->getProfileById(lrTemp.profile.playerId, pTempAfter));
    CHECK_TRUE(store->getProfileById(lrPerm.profile.playerId, pPermAfter));
    CHECK_FALSE(pTempAfter.banned);
    CHECK_TRUE(pPermAfter.banned);
    CHECK_TRUE(store->setBanned(lrPerm.profile.playerId, false));

    /* 2. BanManager operator hand-edited bans.json (@handle & robot_name resolution) */
    const std::string banJsonPath = dir->path() + "/operator_bans.json";
    {
        std::ofstream ofs(banJsonPath, std::ios::binary | std::ios::trunc);
        ofs << "{\n"
            << "  \"version\": 1,\n"
            << "  \"updated_at\": " << clockNow << ",\n"
            << "  \"entries\": [\n"
            << "    {\"ban_id\":\"op1\",\"target_type\":\"account\",\"target_value\":\"@"
            << lrPerm.profile.handle << "\",\"reason\":\"hand-edited handle\"},\n"
            << "    {\"ban_id\":\"op2\",\"target_type\":\"robot\",\"target_value\":\""
            << rBot.robot.robotName << "\",\"reason\":\"hand-edited bot\"}\n"
            << "  ]\n"
            << "}\n";
    }
    octi::BanManager bm;
    bm.configure(banJsonPath, "127.0.0.1", {"127.0.0.1", "::1"});
    std::string loadErr;
    CHECK_TRUE_TEXT(bm.loadFromStorage(*store, clockNow, loadErr), loadErr.c_str());

    octi::BanMatch matchAcct;
    octi::BanMatch matchBot;
    CHECK_TRUE(bm.matchAccount(lrPerm.profile.uuid, clockNow, matchAcct));
    CHECK_TRUE(bm.matchRobot(rBot.robot.robotUuid, clockNow, matchBot));
    CHECK_TRUE(store->getProfileById(lrPerm.profile.playerId, pPermAfter));
    CHECK_TRUE(pPermAfter.banned);

    /* 3. AbusePatternDetector bounded memory (kMaxTrackedKeys) + all 4 escalation families */
    octi::BanThresholds thr;
    thr.authFailuresIn300s = 4;
    thr.passcodeFailuresIn300s = 3;
    thr.crawlerUaIn300s = 3;
    thr.anonEnumPathsIn300s = 5;
    thr.exploitProbesIn300s = 3;
    bm.configure(dir->path() + "/detector_bans.json", "127.0.0.1", {"127.0.0.1", "::1"}, thr);

    /* Flood > kMaxTrackedKeys distinct IPs below threshold -> bounded eviction, zero crash */
    for (int i = 0; i < 16500; ++i) {
        const std::string synthIp =
            "198.18." + std::to_string((i / 250) % 256) + "." +
            std::to_string((i % 250) + 1);
        octi::AbuseActorContext c;
        c.remoteIp = synthIp;
        bm.recordAuthFailure(*store, c, clockNow);
    }

    /* Admin IP & loopback immunity under detector floods */
    bm.noteVerifiedAdminIp("203.0.113.200", clockNow);
    for (int i = 0; i < 10; ++i) {
        bm.recordCrawlerUaHit(*store, "203.0.113.200", clockNow);
        bm.recordCrawlerUaHit(*store, "127.0.0.1", clockNow);
    }
    octi::BanMatch mImmune;
    CHECK_FALSE(bm.matchIp("203.0.113.200", clockNow, mImmune));
    CHECK_FALSE(bm.matchIp("127.0.0.1", clockNow, mImmune));

    /* Anonymous distinct-path enumeration escalates at 5 distinct paths (not duplicate hits) */
    for (int i = 0; i < 10; ++i) {
        bm.recordAnonymousEnumeration(*store, "203.0.113.101",
                                      "/api/v1/archive/games/same", 404, clockNow);
    }
    CHECK_FALSE(bm.matchIp("203.0.113.101", clockNow, mImmune));
    for (int i = 0; i < 5; ++i) {
        bm.recordAnonymousEnumeration(
            *store, "203.0.113.101",
            "/api/v1/archive/games/g" + std::to_string(i), 404, clockNow);
    }
    CHECK_TRUE(bm.matchIp("203.0.113.101", clockNow, mImmune));
    LONGS_EQUAL(clockNow + 86400, mImmune.expiresAt);

    /* Verify ban_audit_log recorded the automatic 24h escalation */
    std::vector<octi::BanAuditLogRow> logs;
    CHECK_TRUE(store->listBanAuditLogs(logs, 20));
    CHECK_FALSE(logs.empty());
    STRCMP_EQUAL("auto_ban_24h", logs[0].action.c_str());
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
