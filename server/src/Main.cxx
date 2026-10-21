/*
 * Main.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "ServerApp.hxx"
#include <iostream>
#include <string>

int main(int argc, char *argv[])
{
    octi::ServerApp app;
    std::string errMsg;

    if (!app.parseArgs(argc, argv, errMsg)) {
        std::cerr << "Error: " << errMsg << "\n"
                  << octi::ServerApp::usageBanner(argc > 0 ? argv[0] : "octi_server");
        return 1;
    }

    return app.run(std::cout, std::cerr);
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
