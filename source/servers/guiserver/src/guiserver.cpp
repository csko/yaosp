/* GUI server
 *
 * Copyright (c) 2010 Zoltan Kovacs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <yaosp/debug.h>

#include <guiserver/guiserver.hpp>
#include <guiserver/application.hpp>
#include <guiserver/decoratorloader.hpp>
#include <guiserver/graphicsdriverloader.hpp>
#include <guiserver/windowmanager.hpp>

GuiServer::GuiServer(void) : m_graphicsDriver(NULL), m_screenBitmap(NULL), m_windowManager(NULL),
                             m_inputThread(NULL), m_fontStorage(NULL), m_serverPort(NULL),
                             m_applicationListLock("app_list_lock") {
}

void GuiServer::addListener(GuiServerListener* listener) {
    for (std::vector<GuiServerListener*>::const_iterator it = m_listeners.begin();
         it != m_listeners.end();
         ++it) {
        if (*it == listener) {
            return;
        }
    }

    m_listeners.push_back(listener);
}

int GuiServer::changeScreenMode(const yguipp::ScreenModeInfo& modeInfo) {
    ScreenMode* mode = NULL;

    for (size_t i = 0; i < m_graphicsDriver->getModeCount(); i++) {
        ScreenMode* tmpMode = m_graphicsDriver->getModeInfo(i);

        if (((int)tmpMode->m_width == modeInfo.m_width) &&
            ((int)tmpMode->m_height == modeInfo.m_height) &&
            (tmpMode->m_colorSpace == modeInfo.m_colorSpace)) {
            mode = tmpMode;
            break;
        }
    }

    if (mode == NULL) {
        return -1;
    }

    m_windowManager->lock();

    delete m_screenBitmap;
    m_graphicsDriver->setMode(mode);
    m_screenBitmap = new Bitmap(
        mode->m_width, mode->m_height, mode->m_colorSpace,
        reinterpret_cast<uint8_t*>(m_graphicsDriver->getFrameBuffer())
    );
    m_screenBitmap->addFlag(Bitmap::SCREEN | Bitmap::VIDEO_MEMORY);

    for (std::vector<GuiServerListener*>::const_iterator it = m_listeners.begin();
         it != m_listeners.end();
         ++it) {
        (*it)->onScreenModeChanged(this, modeInfo);
    }

    m_windowManager->unLock();

    {
        yutilpp::thread::ScopedMutex lock(&m_applicationListLock);

        for (std::vector<Application*>::const_iterator it = m_applicationList.begin();
             it != m_applicationList.end();
             ++it) {
            Application* application = *it;
            application->screenModeChanged(modeInfo);
        }
    }

    return 0;
}

void GuiServer::removeApplication(Application* application) {
    yutilpp::thread::ScopedMutex lock(&m_applicationListLock);

    for (std::vector<Application*>::iterator it = m_applicationList.begin();
         it != m_applicationList.end();
         ++it) {
        if (*it == application) {
            m_applicationList.erase(it);
            break;
        }
    }
}

int GuiServer::run(void) {
    /* Setup screen mode. */

    m_graphicsDriver = GraphicsDriverLoader::detectDriver();

    if (m_graphicsDriver == NULL) {
        dbprintf("Failed to detect graphics driver.\n");
        return 0;
    }

    m_graphicsDriver->initialize();

    dbprintf("Using %s graphics driver.\n", m_graphicsDriver->getName().c_str());

    ScreenMode mode(640, 480, yguipp::CS_RGB32);
    m_graphicsDriver->setMode(&mode);

    m_screenBitmap = new Bitmap(
        640, 480, yguipp::CS_RGB32,
        reinterpret_cast<uint8_t*>(m_graphicsDriver->getFrameBuffer())
    );
    m_screenBitmap->addFlag(Bitmap::SCREEN | Bitmap::VIDEO_MEMORY);

    /* Initialize other stuffs. */

    m_fontStorage = new FontStorage();
    m_fontStorage->init();
    m_fontStorage->loadFonts();
    m_windowManager = new WindowManager(this, DecoratorLoader::loadDecorator(this, "black"));
    m_inputThread = new InputThread(m_windowManager);
    m_inputThread->init();

    m_graphicsDriver->fillRect(
        m_screenBitmap, m_screenBitmap->bounds(), yguipp::Rect(0, 0, 639, 479),
        yguipp::Color(75, 100, 125), yguipp::DM_COPY
    );

    m_windowManager->enable();
    m_inputThread->start();

    m_serverPort = new yutilpp::IPCPort();
    m_serverPort->createNew();
    m_serverPort->registerAsNamed("guiserver");

    /* Mainloop. */

    while (1) {
        int ret;
        uint32_t code;
        char buffer[512];

        ret = m_serverPort->receive(code, buffer, sizeof(buffer));

        if (ret < 0) {
            dbprintf( "GuiServer::run(): failed to get message.\n" );
            break;
        }

        switch (code) {
            case Y_APPLICATION_CREATE : {
                Application* application;

                application = Application::createFrom( this, reinterpret_cast<AppCreate*>(buffer) );
                application->start();

                {
                    yutilpp::thread::ScopedMutex lock(&m_applicationListLock);
                    m_applicationList.push_back(application);
                }

                break;
            }
        }
    }

    return 0;
}
