/*
 * OctiCrashReporter.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <dbghelp.h>
#else
#include <cxxabi.h>
#include <execinfo.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>
#endif

#include "OctiCrashReporter.hxx"
#include "ClientConfigStore.hxx"
#include "OctiCore.hxx"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <random>
#include <sstream>

namespace octi {
namespace desktop {

namespace {

using Json = nlohmann::json;

std::mutex s_mutex;
std::string s_storageRootDir;
std::string s_crashesDir;
std::string s_pendingUploadsDir;

std::mutex s_gameContextMutex;
std::string s_gameMatchMode = "LOCAL";
int s_gamePlyCount = 0;
std::string s_gameFen = "";

std::terminate_handler s_previousTerminateHandler = nullptr;
#ifdef _WIN32
LPTOP_LEVEL_EXCEPTION_FILTER s_previousSehFilter = nullptr;
#else
char s_crashesDirPathBuffer[1024] = {0};
struct sigaction s_prevSigsegv;
struct sigaction s_prevSigfpe;
struct sigaction s_prevSigill;
struct sigaction s_prevSigbus;
struct sigaction s_prevSigabrt;
#endif

std::atomic<bool> s_handlersInstalled{false};

std::string generateRandomHex(std::size_t numChars = 8)
{
    static const char hexChars[] = "0123456789abcdef";
    std::string out;
    out.reserve(numChars);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, 15);
    for (std::size_t i = 0; i < numChars; ++i) {
        out.push_back(hexChars[dis(gen)]);
    }
    return out;
}

std::string sanitizeFilenamePart(const std::string &in)
{
    std::string out;
    out.reserve(in.size());
    for (char c : in) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_') {
            out.push_back(c);
        } else {
            out.push_back('_');
        }
    }
    return out.empty() ? "game" : out;
}

#ifdef _WIN32
const char *sehCodeToString(DWORD code)
{
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:
        return "EXCEPTION_ACCESS_VIOLATION";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
    case EXCEPTION_BREAKPOINT:
        return "EXCEPTION_BREAKPOINT";
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        return "EXCEPTION_DATATYPE_MISALIGNMENT";
    case EXCEPTION_FLT_DENORMAL_OPERAND:
        return "EXCEPTION_FLT_DENORMAL_OPERAND";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
    case EXCEPTION_FLT_INEXACT_RESULT:
        return "EXCEPTION_FLT_INEXACT_RESULT";
    case EXCEPTION_FLT_INVALID_OPERATION:
        return "EXCEPTION_FLT_INVALID_OPERATION";
    case EXCEPTION_FLT_OVERFLOW:
        return "EXCEPTION_FLT_OVERFLOW";
    case EXCEPTION_FLT_STACK_CHECK:
        return "EXCEPTION_FLT_STACK_CHECK";
    case EXCEPTION_FLT_UNDERFLOW:
        return "EXCEPTION_FLT_UNDERFLOW";
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        return "EXCEPTION_ILLEGAL_INSTRUCTION";
    case EXCEPTION_IN_PAGE_ERROR:
        return "EXCEPTION_IN_PAGE_ERROR";
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        return "EXCEPTION_INT_DIVIDE_BY_ZERO";
    case EXCEPTION_INT_OVERFLOW:
        return "EXCEPTION_INT_OVERFLOW";
    case EXCEPTION_INVALID_DISPOSITION:
        return "EXCEPTION_INVALID_DISPOSITION";
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
    case EXCEPTION_PRIV_INSTRUCTION:
        return "EXCEPTION_PRIV_INSTRUCTION";
    case EXCEPTION_SINGLE_STEP:
        return "EXCEPTION_SINGLE_STEP";
    case EXCEPTION_STACK_OVERFLOW:
        return "EXCEPTION_STACK_OVERFLOW";
    default:
        return "UNKNOWN_SEH_EXCEPTION";
    }
}

LONG WINAPI octiUnhandledExceptionFilter(EXCEPTION_POINTERS *pExceptionInfo)
{
    try {
        std::string crashesDir = OctiCrashReporter::crashesDir();
        std::int64_t epoch = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        std::string hexSuffix = generateRandomHex(8);
        std::string filePath = crashesDir + "/crash_" + std::to_string(epoch) +
                               "_" + hexSuffix + ".json";

        DWORD code = pExceptionInfo && pExceptionInfo->ExceptionRecord
            ? pExceptionInfo->ExceptionRecord->ExceptionCode
            : 0;
        void *faultAddr = pExceptionInfo && pExceptionInfo->ExceptionRecord
            ? pExceptionInfo->ExceptionRecord->ExceptionAddress
            : nullptr;

        char codeHex[32];
        std::snprintf(codeHex, sizeof(codeHex), "0x%08lX", static_cast<unsigned long>(code));
        std::string exType = std::string(sehCodeToString(code)) + " (" + codeHex + ")";

        char msgBuf[128];
        std::snprintf(msgBuf, sizeof(msgBuf), "SEH fault code 0x%08lX at address %p",
                      static_cast<unsigned long>(code), faultAddr);
        std::string exMessage(msgBuf);

        std::string stackTrace = OctiCrashReporter::captureStackTrace(32, 0);

        Json root;
        root["crash_id"] = "crash_" + std::to_string(epoch) + "_" + hexSuffix;
        root["created_at"] = epoch;
        root["platform"] = OctiCrashReporter::currentPlatform();
        root["app_version"] = OctiCore::versionString();
        root["os_version"] = OctiCrashReporter::currentOsVersion();
        root["exception_type"] = exType;
        root["exception_message"] = exMessage;
        root["thread_name"] = "MainSEH";
        root["stack_trace"] = stackTrace;
        root["context"] = Json::parse(OctiCrashReporter::getGameContextJson(), nullptr, false);
        root["install_id"] = "";

        std::ofstream ofs(filePath, std::ios::out | std::ios::trunc);
        if (ofs.is_open()) {
            ofs << root.dump(2);
            ofs.close();
        }

        OctiCrashReporter::pruneDirectory(crashesDir, 32);
    } catch (...) {
    }

    return EXCEPTION_EXECUTE_HANDLER;
}
#else
void octiPosixSignalHandler(int sig, siginfo_t *info, void * /*ucontext*/)
{
    const char *sigName = "SIGUNKNOWN";
    switch (sig) {
    case SIGSEGV: sigName = "SIGSEGV"; break;
    case SIGFPE:  sigName = "SIGFPE";  break;
    case SIGILL:  sigName = "SIGILL";  break;
    case SIGBUS:  sigName = "SIGBUS";  break;
    case SIGABRT: sigName = "SIGABRT"; break;
    }

    if (s_crashesDirPathBuffer[0] != '\0') {
        char filePath[1100];
        long long epoch = static_cast<long long>(time(nullptr));
        std::snprintf(filePath, sizeof(filePath), "%s/crash_%lld_sig.json",
                      s_crashesDirPathBuffer, epoch);

        int fd = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (fd >= 0) {
            char buf[1024];
            void *addr = info ? info->si_addr : nullptr;
            int len = std::snprintf(
                buf, sizeof(buf),
                "{\n"
                "  \"crash_id\": \"crash_%lld_sig\",\n"
                "  \"created_at\": %lld,\n"
                "  \"platform\": \"linux-x86_64\",\n"
                "  \"app_version\": \"%s\",\n"
                "  \"os_version\": \"Linux\",\n"
                "  \"exception_type\": \"%s\",\n"
                "  \"exception_message\": \"Fatal signal %d at address %p\",\n"
                "  \"thread_name\": \"PosixSignalThread\",\n"
                "  \"stack_trace\": \"\",\n"
                "  \"context\": {},\n"
                "  \"install_id\": \"\"\n"
                "}\n",
                epoch, epoch, OctiCore::versionString(),
                sigName, sig, addr);
            if (len > 0) {
                ssize_t written = write(fd, buf, static_cast<size_t>(len));
                (void)written;
            }
            close(fd);
        }
    }

    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sigaction(sig, &sa, nullptr);
    raise(sig);
}
#endif

void octiTerminateHandler()
{
    try {
        std::string exType = "std::terminate";
        std::string exMsg = "Unhandled exception triggered std::terminate";
        std::exception_ptr eptr = std::current_exception();
        if (eptr) {
            try {
                std::rethrow_exception(eptr);
            } catch (const std::exception &ex) {
                exType = typeid(ex).name();
                exMsg = ex.what();
            } catch (...) {
                exType = "unknown_exception";
                exMsg = "Unknown non-std exception triggered terminate";
            }
        }

        std::string crashesDir = OctiCrashReporter::crashesDir();
        std::int64_t epoch = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        std::string hexSuffix = generateRandomHex(8);
        std::string filePath = crashesDir + "/crash_" + std::to_string(epoch) +
                               "_" + hexSuffix + ".json";

        std::string stackTrace = OctiCrashReporter::captureStackTrace(32, 0);

        Json root;
        root["crash_id"] = "crash_" + std::to_string(epoch) + "_" + hexSuffix;
        root["created_at"] = epoch;
        root["platform"] = OctiCrashReporter::currentPlatform();
        root["app_version"] = OctiCore::versionString();
        root["os_version"] = OctiCrashReporter::currentOsVersion();
        root["exception_type"] = exType;
        root["exception_message"] = exMsg;
        root["thread_name"] = "TerminateThread";
        root["stack_trace"] = stackTrace;
        root["context"] = Json::parse(OctiCrashReporter::getGameContextJson(), nullptr, false);
        root["install_id"] = "";

        std::ofstream ofs(filePath, std::ios::out | std::ios::trunc);
        if (ofs.is_open()) {
            ofs << root.dump(2);
            ofs.close();
        }

        OctiCrashReporter::pruneDirectory(crashesDir, 32);
    } catch (...) {
    }

    std::abort();
}

} // namespace

void OctiCrashReporter::setStorageRootDir(const std::string &dir)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    if (dir.empty()) {
        s_storageRootDir = ClientConfigStore::defaultConfigDir();
    } else {
        s_storageRootDir = dir;
    }
    s_crashesDir = s_storageRootDir + "/crashes";
    s_pendingUploadsDir = s_storageRootDir + "/pending_uploads";

    try {
        std::filesystem::create_directories(s_crashesDir);
        std::filesystem::create_directories(s_pendingUploadsDir);
    } catch (...) {
    }

#ifndef _WIN32
    std::snprintf(s_crashesDirPathBuffer, sizeof(s_crashesDirPathBuffer),
                  "%s", s_crashesDir.c_str());
#endif
}

std::string OctiCrashReporter::storageRootDir()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    if (s_storageRootDir.empty()) {
        s_storageRootDir = ClientConfigStore::defaultConfigDir();
        s_crashesDir = s_storageRootDir + "/crashes";
        s_pendingUploadsDir = s_storageRootDir + "/pending_uploads";
        try {
            std::filesystem::create_directories(s_crashesDir);
            std::filesystem::create_directories(s_pendingUploadsDir);
        } catch (...) {
        }
#ifndef _WIN32
        std::snprintf(s_crashesDirPathBuffer, sizeof(s_crashesDirPathBuffer),
                      "%s", s_crashesDir.c_str());
#endif
    }
    return s_storageRootDir;
}

std::string OctiCrashReporter::crashesDir()
{
    storageRootDir();
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_crashesDir;
}

std::string OctiCrashReporter::pendingUploadsDir()
{
    storageRootDir();
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_pendingUploadsDir;
}

void OctiCrashReporter::installGlobalHandlers(const std::string &storageRootDir)
{
    setStorageRootDir(storageRootDir);

    if (s_handlersInstalled.exchange(true)) {
        return;
    }

    s_previousTerminateHandler = std::set_terminate(octiTerminateHandler);

#ifdef _WIN32
    s_previousSehFilter = SetUnhandledExceptionFilter(octiUnhandledExceptionFilter);
#else
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = octiPosixSignalHandler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGSEGV, &sa, &s_prevSigsegv);
    sigaction(SIGFPE,  &sa, &s_prevSigfpe);
    sigaction(SIGILL,  &sa, &s_prevSigill);
    sigaction(SIGBUS,  &sa, &s_prevSigbus);
    sigaction(SIGABRT, &sa, &s_prevSigabrt);
#endif
}

void OctiCrashReporter::uninstallGlobalHandlers()
{
    if (!s_handlersInstalled.exchange(false)) {
        return;
    }

    if (s_previousTerminateHandler) {
        std::set_terminate(s_previousTerminateHandler);
        s_previousTerminateHandler = nullptr;
    }

#ifdef _WIN32
    if (s_previousSehFilter) {
        SetUnhandledExceptionFilter(s_previousSehFilter);
        s_previousSehFilter = nullptr;
    }
#else
    sigaction(SIGSEGV, &s_prevSigsegv, nullptr);
    sigaction(SIGFPE,  &s_prevSigfpe,  nullptr);
    sigaction(SIGILL,  &s_prevSigill,  nullptr);
    sigaction(SIGBUS,  &s_prevSigbus,  nullptr);
    sigaction(SIGABRT, &s_prevSigabrt, nullptr);
#endif
}

void OctiCrashReporter::updateGameContext(const std::string &matchMode,
                                         int plyCount,
                                         const std::string &fen)
{
    std::lock_guard<std::mutex> lock(s_gameContextMutex);
    s_gameMatchMode = matchMode;
    s_gamePlyCount = plyCount;
    s_gameFen = fen;
}

std::string OctiCrashReporter::getGameContextJson()
{
    std::lock_guard<std::mutex> lock(s_gameContextMutex);
    Json j;
    j["match_mode"] = s_gameMatchMode;
    j["ply_count"] = s_gamePlyCount;
    j["fen"] = s_gameFen;
    return j.dump();
}

std::string OctiCrashReporter::currentPlatform()
{
#ifdef _WIN32
    return "windows-x86_64";
#elif defined(__APPLE__)
    return "macos";
#else
    return "linux-x86_64";
#endif
}

std::string OctiCrashReporter::currentOsVersion()
{
#ifdef _WIN32
    return "Windows";
#else
    struct utsname uts;
    if (uname(&uts) == 0) {
        return std::string(uts.sysname) + " " + std::string(uts.release);
    }
    return "Linux";
#endif
}

std::string OctiCrashReporter::captureStackTrace(int maxFrames, int skipFrames)
{
    std::ostringstream oss;
    if (maxFrames <= 0) {
        return oss.str();
    }

#ifdef _WIN32
    std::vector<void *> frames(maxFrames);
    USHORT captured = CaptureStackBackTrace(
        static_cast<DWORD>(skipFrames), static_cast<DWORD>(maxFrames),
        frames.data(), nullptr);

    for (USHORT i = 0; i < captured; ++i) {
        void *addr = frames[i];
        HMODULE hMod = nullptr;
        char frameLine[256];
        if (GetModuleHandleExA(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCSTR>(addr), &hMod) && hMod) {
            char modName[MAX_PATH];
            if (GetModuleFileNameA(hMod, modName, sizeof(modName))) {
                const char *base = std::strrchr(modName, '\\');
                base = base ? base + 1 : modName;
                uintptr_t rva = reinterpret_cast<uintptr_t>(addr) -
                                reinterpret_cast<uintptr_t>(hMod);
                std::snprintf(frameLine, sizeof(frameLine), "#%u %s+0x%08llx",
                              static_cast<unsigned>(i), base,
                              static_cast<unsigned long long>(rva));
            } else {
                std::snprintf(frameLine, sizeof(frameLine), "#%u %p",
                              static_cast<unsigned>(i), addr);
            }
        } else {
            std::snprintf(frameLine, sizeof(frameLine), "#%u %p",
                          static_cast<unsigned>(i), addr);
        }
        if (i > 0) {
            oss << "\n";
        }
        oss << frameLine;
    }
#else
    std::vector<void *> frames(maxFrames + skipFrames);
    int captured = backtrace(frames.data(), maxFrames + skipFrames);
    if (captured > skipFrames) {
        char **symbols = backtrace_symbols(frames.data(), captured);
        if (symbols) {
            int outIdx = 0;
            for (int i = skipFrames; i < captured; ++i) {
                if (outIdx > 0) {
                    oss << "\n";
                }
                oss << "#" << outIdx << " " << symbols[i];
                ++outIdx;
            }
            free(symbols);
        }
    }
#endif

    return oss.str();
}

void OctiCrashReporter::pruneDirectory(const std::string &dirPath, std::size_t maxFiles)
{
    try {
        if (!std::filesystem::exists(dirPath)) {
            return;
        }
        std::vector<std::filesystem::directory_entry> entries;
        for (const auto &entry : std::filesystem::directory_iterator(dirPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                entries.push_back(entry);
            }
        }
        if (entries.size() <= maxFiles) {
            return;
        }

        std::sort(entries.begin(), entries.end(),
                  [](const std::filesystem::directory_entry &a,
                     const std::filesystem::directory_entry &b) {
                      return a.last_write_time() < b.last_write_time();
                  });

        std::size_t toDelete = entries.size() - maxFiles;
        for (std::size_t i = 0; i < toDelete; ++i) {
            std::error_code ec;
            std::filesystem::remove(entries[i].path(), ec);
        }
    } catch (...) {
    }
}

std::string OctiCrashReporter::recordHandledException(
    const std::string &threadName,
    const std::string &exceptionType,
    const std::string &exceptionMessage,
    const std::string &contextJson,
    const std::string &installId)
{
    std::string dir = crashesDir();
    std::int64_t epoch = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::string hexSuffix = generateRandomHex(8);
    std::string filePath = dir + "/crash_" + std::to_string(epoch) + "_" +
                           hexSuffix + ".json";

    std::string stackTrace = captureStackTrace(32, 1);

    Json root;
    root["crash_id"] = "crash_" + std::to_string(epoch) + "_" + hexSuffix;
    root["created_at"] = epoch;
    root["platform"] = currentPlatform();
    root["app_version"] = OctiCore::versionString();
    root["os_version"] = currentOsVersion();
    root["exception_type"] = exceptionType;
    root["exception_message"] = exceptionMessage;
    root["thread_name"] = threadName;
    root["stack_trace"] = stackTrace;

    Json ctx = Json::parse(contextJson, nullptr, false);
    if (!ctx.is_discarded() && ctx.is_object()) {
        root["context"] = ctx;
    } else {
        root["context"] = Json::parse(getGameContextJson(), nullptr, false);
    }
    root["install_id"] = installId;

    try {
        std::ofstream ofs(filePath, std::ios::out | std::ios::trunc);
        if (ofs.is_open()) {
            ofs << root.dump(2);
            ofs.close();
        }
        pruneDirectory(dir, 32);
    } catch (...) {
    }

    return filePath;
}

std::string OctiCrashReporter::spoolTrainingTranscript(
    const std::string &gameId,
    const std::string &transcriptJson,
    const std::string &clientId)
{
    std::string dir = pendingUploadsDir();
    std::int64_t epoch = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::string safeId = sanitizeFilenamePart(gameId);
    std::string filePath = dir + "/transcript_" + safeId + "_" +
                           std::to_string(epoch) + ".json";

    Json root;
    root["game_id"] = gameId;
    Json parsed = Json::parse(transcriptJson, nullptr, false);
    if (!parsed.is_discarded() && parsed.is_object()) {
        root["transcript"] = parsed;
    } else {
        root["transcript"] = transcriptJson;
    }
    if (!clientId.empty()) {
        root["client_id"] = clientId;
    }

    try {
        std::ofstream ofs(filePath, std::ios::out | std::ios::trunc);
        if (ofs.is_open()) {
            ofs << root.dump(2);
            ofs.close();
        }
        pruneDirectory(dir, 32);
    } catch (...) {
    }

    return filePath;
}

std::vector<std::string> OctiCrashReporter::listPendingCrashFiles()
{
    std::vector<std::string> files;
    std::string dir = crashesDir();
    try {
        if (std::filesystem::exists(dir)) {
            for (const auto &entry : std::filesystem::directory_iterator(dir)) {
                if (entry.is_regular_file() && entry.path().extension() == ".json") {
                    files.push_back(entry.path().string());
                }
            }
            std::sort(files.begin(), files.end());
        }
    } catch (...) {
    }
    return files;
}

std::vector<std::string> OctiCrashReporter::listPendingTranscriptFiles()
{
    std::vector<std::string> files;
    std::string dir = pendingUploadsDir();
    try {
        if (std::filesystem::exists(dir)) {
            for (const auto &entry : std::filesystem::directory_iterator(dir)) {
                if (entry.is_regular_file() && entry.path().extension() == ".json") {
                    files.push_back(entry.path().string());
                }
            }
            std::sort(files.begin(), files.end());
        }
    } catch (...) {
    }
    return files;
}

int OctiCrashReporter::flushPendingCrashReports(
    OctiNetworkClient &client,
    const std::string &installId)
{
    int flushedCount = 0;
    std::vector<std::string> files = listPendingCrashFiles();

    for (const auto &file : files) {
        try {
            std::error_code ec;
            std::uintmax_t size = std::filesystem::file_size(file, ec);
            if (ec || size == 0 || size > 65536) {
                std::filesystem::remove(file, ec);
                continue;
            }

            std::ifstream ifs(file);
            if (!ifs.is_open()) {
                continue;
            }
            std::string content((std::istreambuf_iterator<char>(ifs)),
                                std::istreambuf_iterator<char>());
            ifs.close();

            Json j = Json::parse(content, nullptr, false);
            if (j.is_discarded() || !j.is_object()) {
                std::filesystem::remove(file, ec);
                continue;
            }

            CrashReportRequest req;
            req.crashId = j.value("crash_id", "");
            req.createdAt = j.value("created_at", static_cast<std::int64_t>(0));
            req.platform = j.value("platform", currentPlatform());
            req.appVersion = j.value("app_version", OctiCore::versionString());
            req.osVersion = j.value("os_version", currentOsVersion());
            req.exceptionType = j.value("exception_type", "");
            req.exceptionMessage = j.value("exception_message", "");
            req.threadName = j.value("thread_name", "");
            req.stackTrace = j.value("stack_trace", "");
            if (j.contains("context")) {
                req.contextJson = j["context"].is_string()
                    ? j["context"].get<std::string>()
                    : j["context"].dump();
            }
            req.installId = j.value("install_id", installId);
            if (req.installId.empty() && !installId.empty()) {
                req.installId = installId;
            }

            CrashReportResponse res = client.uploadCrashReport(req);
            if (res.ok || res.httpStatus == 400 || res.httpStatus == 413) {
                std::filesystem::remove(file, ec);
                ++flushedCount;
            } else if (res.httpStatus == 0 || res.httpStatus >= 500) {
                // Network unreachable or server error, retain for retry
                break;
            }
        } catch (...) {
        }
    }

    return flushedCount;
}

int OctiCrashReporter::flushPendingTranscripts(
    OctiNetworkClient &client,
    const std::string &overrideInstallId)
{
    int flushedCount = 0;
    std::vector<std::string> files = listPendingTranscriptFiles();

    for (const auto &file : files) {
        try {
            std::error_code ec;
            std::uintmax_t size = std::filesystem::file_size(file, ec);
            if (ec || size == 0 || size > 131072) {
                std::filesystem::remove(file, ec);
                continue;
            }

            std::ifstream ifs(file);
            if (!ifs.is_open()) {
                continue;
            }
            std::string content((std::istreambuf_iterator<char>(ifs)),
                                std::istreambuf_iterator<char>());
            ifs.close();

            Json j = Json::parse(content, nullptr, false);
            if (j.is_discarded() || !j.is_object()) {
                std::filesystem::remove(file, ec);
                continue;
            }

            UploadTranscriptRequest req;
            if (j.contains("transcript")) {
                req.transcriptJson = j["transcript"].is_string()
                    ? j["transcript"].get<std::string>()
                    : j["transcript"].dump();
            } else {
                req.transcriptJson = content;
            }

            if (j.contains("client_id") && j["client_id"].is_string()) {
                req.clientId = j["client_id"].get<std::string>();
            } else if (!overrideInstallId.empty()) {
                req.clientId = overrideInstallId;
            }

            auto res = client.uploadTrainingTranscript(req);
            if (res.ok || res.httpStatus == 400) {
                std::filesystem::remove(file, ec);
                ++flushedCount;
            } else if (res.httpStatus == 0 || res.httpStatus >= 500) {
                // Network unreachable or server error, retain for retry
                break;
            }
        } catch (...) {
        }
    }

    return flushedCount;
}

} // namespace desktop
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
