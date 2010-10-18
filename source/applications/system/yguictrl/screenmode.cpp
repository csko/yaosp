#include <iostream>

#include <ygui++/application.hpp>
#include <yutil++/stringutils.hpp>

#include "screenmode.hpp"

int ScreenMode::handleCommand(int argc, char** argv) {
    std::string cmd;

    if (argc > 0) {
        cmd = argv[0];
    }

    if ((cmd.empty()) ||
        (cmd == "help")) {
        return help();
    } else if (cmd == "list") {
        return list();
    } else if (cmd == "set") {
        return set(argc, argv);
    }

    return 0;
}

int ScreenMode::list(void) {
    yguipp::ScreenModeInfo currentMode;
    std::vector<yguipp::ScreenModeInfo> modeList;

    yguipp::Application::getInstance()->getScreenModeList(modeList);
    currentMode = yguipp::Application::getInstance()->getCurrentScreenMode();

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

            std::cout << "    " << info.m_width << "x" << info.m_height << " @ " << colorspace_to_bpp(info.m_colorSpace) * 8 << "bpp";

            if (info == currentMode) {
                std::cout << " (active)";
            }

            std::cout << std::endl;
        }
    }

    return 0;
}

int ScreenMode::help(void) {
    std::cerr << "Available commands related to screenmode:" << std::endl;
    std::cerr << "    set  - sets the active screen mode" << std::endl;
    std::cerr << "    list - lists available screen modes" << std::endl;

    return 0;
}

int ScreenMode::set(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Not enough parameters at screenmode set." << std::endl;
        std::cerr << "Proper command syntax:" << std::endl;
        std::cerr << "    yguictrl screenmode set newmode" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Screen mode format:" << std::endl;
        std::cerr << "    [width]x[height]@[depth]" << std::endl;
        return -1;
    }

    int depth;
    std::string mode = argv[1];
    yguipp::ScreenModeInfo modeInfo;

    std::vector<std::string> tokens;
    yutilpp::StringUtils::tokenize(mode, tokens, "x");

    if (tokens.size() != 2) {
        // invalid screen mode
        return -1;
    }

    std::string tmp = tokens[1];
    yutilpp::StringUtils::tokenize(tmp, tokens, "@");

    if (tokens.size() != 4) {
        // invalid screen mode
        return -1;
    }

    if ((!yutilpp::StringUtils::toInt(tokens[0], modeInfo.m_width)) ||
        (!yutilpp::StringUtils::toInt(tokens[2], modeInfo.m_height)) ||
        (!yutilpp::StringUtils::toInt(tokens[3], depth))) {
        // invalid screen mode
        return -1;
    }

    modeInfo.m_colorSpace = yguipp::bpp_to_colorspace(depth);

    yguipp::Application::getInstance()->setScreenMode(modeInfo);

    return 0;
}
