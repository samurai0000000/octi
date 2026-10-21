/*
 * Main.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "OctiCore.hxx"
#include <cstring>
#include <iostream>
#include <QApplication>
#include <QWidget>

int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        if (argv[i] != nullptr &&
            (std::strcmp(argv[i], "--version") == 0 ||
             std::strcmp(argv[i], "-v") == 0)) {
            std::cout << octi::OctiCore::buildBanner("octi");
            return 0;
        }
    }

    QApplication app(argc, argv);
    QWidget window;
    window.setWindowTitle(
        QString("OCTI v%1").arg(octi::OctiCore::versionString()));
    window.show();
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
