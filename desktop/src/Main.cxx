/*
 * Main.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include <iostream>
#include <string>
#include <QApplication>
#include <QTimer>
#include "OctiMainWindow.hxx"
#include "selfupdate/UpdateInstaller.hxx"

int main(int argc, char *argv[])
{
    octi::DesktopCliArgs cliArgs;
    std::string errMsg;
    if (!octi::DesktopCliArgs::parse(
            argc, const_cast<const char * const *>(argv), cliArgs, errMsg)) {
        std::cerr << "Error: " << errMsg << "\n"
                  << octi::DesktopCliArgs::usageBanner(
                         argc > 0 ? argv[0] : "octi");
        return 1;
    }

    if (cliArgs.showVersion) {
        std::cout << octi::DesktopCliArgs::versionBanner();
        return 0;
    }
    if (cliArgs.showHelp) {
        std::cout << octi::DesktopCliArgs::usageBanner(
            argc > 0 ? argv[0] : "octi");
        return 0;
    }

    selfupdate::UpdateInstaller::cleanupStaleBackup();

    QApplication app(argc, argv);
    app.setWindowIcon(octi::createOctiAppIcon());
    octi::OctiSplashDialog splash;
    splash.exec();
    octi::OctiMainWindow window(
        nullptr, cliArgs.configPath, cliArgs.startOffline);
    window.show();
    if (!cliArgs.startOffline) {
        QTimer::singleShot(0, [&window]() {
            window.checkForAppUpdates(false);
        });
    }
    return app.exec();
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
