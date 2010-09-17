#include <iostream>

#include <ygui++/application.hpp>

#include "screenmode.hpp"

int ScreenMode::handleCommand(int argc, char** argv) {
    if (argc < 1) {
        return -1;
    }

    std::string cmd = argv[0];

    if (cmd == "list") {
        return list();
    }

    return 0;
}

int ScreenMode::list(void) {
    std::vector<yguipp::ScreenModeInfo> modeList;
    yguipp::Application::getInstance()->getScreenModeList(modeList);

    if (modeList.empty()) {
        std::cerr << "There are no available screen modes." << std::endl;
    } else {
        std::cout << "Available screen modes:" << std::endl;

        for (std::vector<yguipp::ScreenModeInfo>::const_iterator it = modeList.begin();
             it != modeList.end();
             ++it) {
            yguipp::ScreenModeInfo info = *it;

            /* List only CS_RGB32 screen modes for now as guiserver does not support others. */
            if (info.m_colorSpace != yguipp::CS_RGB32) {
                continue;
            }

            std::cout << "    " << info.m_width << "x" << info.m_height << " @ " << colorspace_to_bpp(info.m_colorSpace) * 8 << "bpp" << std::endl;
        }
    }

    return 0;
}
