/*
 * OctiCrashReporter.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef OCTI_DESKTOP_OCTI_CRASH_REPORTER_HXX
#define OCTI_DESKTOP_OCTI_CRASH_REPORTER_HXX

#include <cstdint>
#include <string>
#include <vector>
#include "OctiNetworkClient.hxx"

namespace octi {
namespace desktop {

class OctiCrashReporter {
public:
    static void setStorageRootDir(const std::string &dir);
    static std::string storageRootDir();
    static std::string crashesDir();
    static std::string pendingUploadsDir();

    static void installGlobalHandlers(const std::string &storageRootDir = "");
    static void uninstallGlobalHandlers();

    static void updateGameContext(const std::string &matchMode,
                                  int plyCount,
                                  const std::string &fen);
    static std::string getGameContextJson();

    static std::string recordHandledException(
        const std::string &threadName,
        const std::string &exceptionType,
        const std::string &exceptionMessage,
        const std::string &contextJson = "{}",
        const std::string &installId = "");

    static std::string spoolTrainingTranscript(
        const std::string &gameId,
        const std::string &transcriptJson,
        const std::string &clientId = "");

    static std::vector<std::string> listPendingCrashFiles();
    static std::vector<std::string> listPendingTranscriptFiles();

    static int flushPendingCrashReports(
        OctiNetworkClient &client,
        const std::string &installId = "");

    static int flushPendingTranscripts(
        OctiNetworkClient &client,
        const std::string &overrideInstallId = "");

    static std::string captureStackTrace(int maxFrames = 32, int skipFrames = 1);
    static void pruneDirectory(const std::string &dirPath, std::size_t maxFiles = 32);

    static std::string currentPlatform();
    static std::string currentOsVersion();
};

} // namespace desktop
} // namespace octi

#endif // OCTI_DESKTOP_OCTI_CRASH_REPORTER_HXX

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
