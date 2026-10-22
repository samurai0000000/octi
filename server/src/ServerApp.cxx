/*
 * ServerApp.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "ServerApp.hxx"
#include "AccountStore.hxx"
#include "GameRoomManager.hxx"
#include "OctiCore.hxx"
#include "OctiHttpServer.hxx"
#include "OidcVerifier.hxx"
#include "Version.hxx"

#include <nlohmann/json.hpp>

#include <sys/stat.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cerrno>
#include <condition_variable>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <ostream>
#include <sstream>
#include <thread>

namespace octi {

namespace {

typedef nlohmann::json Json;

const size_t kMaxOidcConfigBytes = 1024 * 1024;
const size_t kMaxClientIdsPerProvider = 64;
const size_t kMaxClientIdBytes = 512;
const size_t kMaxSecretBytes = 512;
const int64_t kJwksRefreshIntervalSec = 3600;

volatile std::sig_atomic_t g_signalStop = 0;

void onStopSignal(int)
{
    g_signalStop = 1;
}

bool isLoopback(const std::string &addr)
{
    return addr == "127.0.0.1" || addr == "localhost" || addr == "::1";
}

/* A string option value: present and not another option. */
bool takeValue(int argc, const char * const argv[], int &i,
               const char *option, std::string &out, std::string &errMsg)
{
    if (i + 1 >= argc || argv[i + 1] == nullptr || argv[i + 1][0] == '\0' ||
        std::strncmp(argv[i + 1], "--", 2) == 0) {
        errMsg = std::string("Missing value for ") + option;
        return false;
    }
    out = argv[++i];
    return true;
}

bool validBindAddress(const std::string &s)
{
    if (s.empty() || s.size() > 255) {
        return false;
    }
    for (size_t k = 0; k < s.size(); ++k) {
        unsigned char c = static_cast<unsigned char>(s[k]);
        if (c <= 0x20 || c >= 0x7F) {
            return false;
        }
    }
    return true;
}

/* The OIDC config holds a secret: only the owner may read or replace it. */
bool checkSecretFile(const std::string &path, std::string &error)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        error = "cannot stat " + path + ": " + std::strerror(errno);
        return false;
    }
    if (!S_ISREG(st.st_mode)) {
        error = path + " is not a regular file";
        return false;
    }
    if (st.st_uid != geteuid()) {
        error = path + " must be owned by the user running the server";
        return false;
    }
    if ((st.st_mode & (S_IRWXG | S_IRWXO)) != 0) {
        error = path + " holds a secret but is accessible by group/other; "
                       "run: chmod 600 on it";
        return false;
    }
    std::filesystem::path dir = std::filesystem::path(path).parent_path();
    if (dir.empty()) {
        dir = ".";
    }
    struct stat ds;
    if (stat(dir.string().c_str(), &ds) != 0) {
        error = "cannot stat directory " + dir.string() + ": " +
                std::strerror(errno);
        return false;
    }
    if ((ds.st_mode & (S_IWGRP | S_IWOTH)) != 0 && (ds.st_mode & S_ISVTX) == 0) {
        error = "directory " + dir.string() +
                " holding the secret is writable by group/other";
        return false;
    }
    return true;
}

bool readClientIds(const Json &entry, const std::string &name,
                   std::vector<std::string> &ids, std::string &error)
{
    Json::const_iterator one = entry.find("client_id");
    if (one != entry.end()) {
        if (!one->is_string()) {
            error = "provider '" + name + "': client_id must be a string";
            return false;
        }
        ids.push_back(one->get<std::string>());
    }
    Json::const_iterator many = entry.find("client_ids");
    if (many != entry.end()) {
        if (!many->is_array()) {
            error = "provider '" + name + "': client_ids must be an array";
            return false;
        }
        for (Json::const_iterator it = many->begin(); it != many->end(); ++it) {
            if (!it->is_string()) {
                error = "provider '" + name +
                        "': client_ids must contain only strings";
                return false;
            }
            ids.push_back(it->get<std::string>());
        }
    }
    if (ids.empty() || ids.size() > kMaxClientIdsPerProvider) {
        error = "provider '" + name + "' needs 1.." +
                std::to_string(kMaxClientIdsPerProvider) + " client ids";
        return false;
    }
    for (size_t i = 0; i < ids.size(); ++i) {
        if (ids[i].empty() || ids[i].size() > kMaxClientIdBytes) {
            error = "provider '" + name + "': invalid client id length";
            return false;
        }
    }
    return true;
}

} // namespace

ServerApp::ServerApp()
    : _config(), _stopRequested(false)
{
}

ServerApp::~ServerApp()
{
}

bool ServerApp::parseArgs(int argc, const char * const argv[], std::string &errMsg)
{
    errMsg.clear();
    _config = ServerConfig();

    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];
        if (arg == nullptr) {
            errMsg = "Null argument encountered";
            return false;
        }
        if (std::strcmp(arg, "--help") == 0 || std::strcmp(arg, "-h") == 0) {
            _config.showHelp = true;
        } else if (std::strcmp(arg, "--version") == 0 || std::strcmp(arg, "-v") == 0) {
            _config.showVersion = true;
        } else if (std::strcmp(arg, "--port") == 0 || std::strcmp(arg, "-p") == 0) {
            if (i + 1 >= argc || argv[i + 1] == nullptr) {
                errMsg = "Missing value for --port";
                return false;
            }
            const char *val = argv[++i];
            if (val[0] == '\0' || val[0] == '-') {
                errMsg = std::string("Invalid port value: ") + val;
                return false;
            }
            char *endPtr = nullptr;
            unsigned long parsed = std::strtoul(val, &endPtr, 10);
            if (endPtr == val || *endPtr != '\0' || parsed > 65535UL) {
                errMsg = std::string("Invalid port value: ") + val;
                return false;
            }
            _config.port = static_cast<uint16_t>(parsed);
        } else if (std::strcmp(arg, "--bind") == 0) {
            std::string val;
            if (!takeValue(argc, argv, i, "--bind", val, errMsg)) {
                return false;
            }
            if (!validBindAddress(val)) {
                errMsg = "Invalid bind address: " + val;
                return false;
            }
            _config.bindAddress = val;
        } else if (std::strcmp(arg, "--db") == 0) {
            if (!takeValue(argc, argv, i, "--db", _config.dbPath, errMsg)) {
                return false;
            }
        } else if (std::strcmp(arg, "--oidc-config") == 0) {
            if (!takeValue(argc, argv, i, "--oidc-config",
                           _config.oidcConfigPath, errMsg)) {
                return false;
            }
        } else {
            errMsg = std::string("Unknown option: ") + arg;
            return false;
        }
    }

    return true;
}

bool ServerApp::loadOidcConfig(const std::string &path, OidcConfig &out,
                               std::string &error)
{
    out.clear();
    error.clear();
    bool hasSecret = false;

    if (!path.empty()) {
        struct stat st;
        if (stat(path.c_str(), &st) != 0) {
            error = "cannot read OIDC config " + path + ": " +
                    std::strerror(errno);
            return false;
        }
        if (!S_ISREG(st.st_mode) ||
            static_cast<uint64_t>(st.st_size) > kMaxOidcConfigBytes) {
            error = "OIDC config " + path +
                    " is not a regular file of at most 1 MiB";
            return false;
        }
        std::ifstream in(path.c_str(), std::ios::binary);
        if (!in) {
            error = "cannot open OIDC config " + path;
            return false;
        }
        std::ostringstream buf;
        buf << in.rdbuf();
        Json doc = Json::parse(buf.str(), nullptr, false);
        if (doc.is_discarded() || !doc.is_object()) {
            error = "OIDC config " + path + " is not a valid JSON object";
            return false;
        }
        Json::const_iterator provs = doc.find("providers");
        if (provs == doc.end() || !provs->is_object()) {
            error = "OIDC config needs a \"providers\" object";
            return false;
        }
        for (Json::const_iterator it = provs->begin(); it != provs->end(); ++it) {
            const std::string name = it.key();
            if (!OidcVerifier::isSupportedProvider(name)) {
                error = "unsupported provider '" + name + "' in OIDC config";
                return false;
            }
            if (!it->is_object()) {
                error = "provider '" + name + "' must be an object";
                return false;
            }
            OidcProviderConfig pc;
            if (!readClientIds(*it, name, pc.clientIds, error)) {
                return false;
            }
            Json::const_iterator sec = it->find("app_secret");
            if (sec != it->end()) {
                if (name != "facebook" || !sec->is_string() ||
                    sec->get<std::string>().size() > kMaxSecretBytes) {
                    error = "app_secret is only valid as a string for facebook";
                    return false;
                }
                pc.appSecret = sec->get<std::string>();
                hasSecret = hasSecret || !pc.appSecret.empty();
            }
            out[name] = pc;
        }
        if (hasSecret && !checkSecretFile(path, error)) {
            out.clear();
            return false;
        }
    }

    const char *envSecret = std::getenv("OCTI_FACEBOOK_APP_SECRET");
    if (envSecret != nullptr && envSecret[0] != '\0') {
        std::string s = envSecret;
        if (s.size() > kMaxSecretBytes) {
            error = "OCTI_FACEBOOK_APP_SECRET is too long";
            out.clear();
            return false;
        }
        out["facebook"].appSecret = s;
    }
    return true;
}

void ServerApp::requestStop()
{
    _stopRequested = true;
}

int ServerApp::run(std::ostream &out, std::ostream &err)
{
    if (_config.showVersion) {
        out << versionBanner();
        return 0;
    }
    if (_config.showHelp) {
        out << usageBanner("octi_server");
        return 0;
    }
    if (_config.dbPath.empty()) {
        err << "Error: --db <path> is required to start the server\n";
        return 2;
    }

    std::string error;
    OidcConfig oidc;
    if (!loadOidcConfig(_config.oidcConfigPath, oidc, error)) {
        err << "Error: " << error << "\n";
        return 2;
    }

    AccountStore store;
    if (!store.openOrCreate(_config.dbPath, error)) {
        err << "Error: cannot open database: " << error << "\n";
        return 2;
    }

    OidcVerifier verifier;
    HttpServerConfig http;
    http.bindAddress = _config.bindAddress;
    http.port = _config.port;
    for (OidcConfig::const_iterator it = oidc.begin(); it != oidc.end(); ++it) {
        if (!it->second.clientIds.empty()) {
            if (!verifier.setClientIds(it->first, it->second.clientIds)) {
                err << "Error: cannot configure provider " << it->first << "\n";
                return 2;
            }
            http.providerClientIds[it->first] = it->second.clientIds;
        }
        if (it->first == "facebook" && !it->second.appSecret.empty()) {
            verifier.setFacebookAppSecret(it->second.appSecret);
        }
    }
    /* Real HTTPS JWKS / Graph access with certificate verification. */
    verifier.setHttpFetcher(defaultHttpFetcher());

    GameRoomManager rooms(store);
    OctiHttpServer server(store, verifier, rooms, http);

    g_signalStop = 0;
    typedef void (*SignalHandler)(int);
    SignalHandler prevInt = std::signal(SIGINT, onStopSignal);
    SignalHandler prevTerm = std::signal(SIGTERM, onStopSignal);

    if (!server.start(error)) {
        std::signal(SIGINT, prevInt);
        std::signal(SIGTERM, prevTerm);
        err << "Error: " << error << "\n";
        return 2;
    }

    /*
     * JWKS refresh: warm every enabled provider's key set now (off the
     * main thread so a slow network never delays startup) and renew it
     * hourly. The verifier also refreshes lazily when it meets an unknown
     * key.
     */
    std::mutex jwksMutex;
    std::condition_variable jwksCv;
    bool jwksStop = false;
    std::thread jwksThread([&]() {
        std::vector<std::string> names = verifier.enabledProviders();
        std::unique_lock<std::mutex> lock(jwksMutex);
        while (!jwksStop) {
            lock.unlock();
            for (size_t i = 0; i < names.size(); ++i) {
                {
                    std::lock_guard<std::mutex> check(jwksMutex);
                    if (jwksStop) {
                        break;
                    }
                }
                verifier.refreshKeySet(names[i]);
            }
            lock.lock();
            jwksCv.wait_for(lock, std::chrono::seconds(kJwksRefreshIntervalSec),
                            [&]() { return jwksStop; });
        }
    });

    std::vector<std::string> enabled = verifier.enabledProviders();
    std::string list;
    for (size_t i = 0; i < enabled.size(); ++i) {
        list += (i ? "," : "") + enabled[i];
    }
    out << "octi_server v" << OctiCore::versionString()
        << " ready (listening on " << server.bindAddress() << ":"
        << server.port() << ", providers: " << (list.empty() ? "none" : list)
        << ")" << std::endl;
    if (enabled.empty()) {
        err << "Warning: no SSO provider is configured (--oidc-config); "
               "nobody can sign in\n";
    }
    if (!isLoopback(_config.bindAddress)) {
        err << "Warning: serving plain HTTP on a non-loopback address; put a "
               "TLS terminating proxy in front\n";
    }

    while (!_stopRequested.load() && g_signalStop == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    server.stop();
    {
        std::lock_guard<std::mutex> lock(jwksMutex);
        jwksStop = true;
    }
    jwksCv.notify_all();
    jwksThread.join();
    std::signal(SIGINT, prevInt);
    std::signal(SIGTERM, prevTerm);
    _stopRequested = false;
    out << "octi_server stopped" << std::endl;
    return 0;
}

const ServerConfig &ServerApp::config() const
{
    return _config;
}

std::string ServerApp::versionBanner()
{
    return OctiCore::buildBanner("octi_server");
}

std::string ServerApp::usageBanner(const char *progName)
{
    const char *name = (progName != nullptr && progName[0] != '\0')
        ? progName : "octi_server";
    std::ostringstream oss;
    oss << "Usage: " << name << " --db <path> [options]\n"
        << "Options:\n"
        << "  -p, --port <port>      Listen port (0..65535, 0 = ephemeral, default: 6874)\n"
        << "      --bind <address>   Listen address (default: 127.0.0.1)\n"
        << "      --db <path>        SQLite database file (required; created 0600)\n"
        << "      --oidc-config <f>  JSON file with per-provider OIDC client ids\n"
        << "                         (0600 required when it holds an app_secret)\n"
        << "  -v, --version          Print version information and exit\n"
        << "  -h, --help             Print this help message and exit\n"
        << "Environment:\n"
        << "  OCTI_FACEBOOK_APP_SECRET  Facebook app secret (overrides the file)\n"
        << "Stop with SIGINT or SIGTERM for a clean shutdown.\n";
    return oss.str();
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
