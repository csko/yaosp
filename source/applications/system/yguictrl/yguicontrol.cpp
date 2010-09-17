#include <iostream>

#include <ygui++/application.hpp>

#include "yguicontrol.hpp"
#include "screenmode.hpp"

YGuiControl::YGuiControl(void) {
    m_handlers["screenmode"] = new ScreenMode();
}

YGuiControl::~YGuiControl(void) {
    for (std::map<std::string, CommandHandler*>::const_iterator it = m_handlers.begin();
         it != m_handlers.end();
         ++it) {
        delete it->second;
    }
}

int YGuiControl::run(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << argv[0] << " command [ARG] ..." << std::endl;
        return 0;
    }

    yguipp::Application::createInstance("yguictrl");

    std::map<std::string, CommandHandler*>::const_iterator it = m_handlers.find(argv[1]);

    if (it == m_handlers.end()) {
        std::cerr << argv[0] << ": unknown command: " << argv[1] << std::endl;
        return -1;
    }

    return it->second->handleCommand(argc - 2, &argv[2]);
}
