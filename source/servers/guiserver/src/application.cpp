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

#include <guiserver/application.hpp>
#include <guiserver/window.hpp>
#include <guiserver/guiserver.hpp>
#include <guiserver/bitmap.hpp>

Application::Application( GuiServer* guiServer ) : IPCListener("app"), m_clientPort(NULL), m_nextWinId(0),
                                                   m_nextFontId(0), m_nextBitmapId(0), m_guiServer(guiServer) {
}

Application::~Application( void ) {
}

bool Application::init( AppCreate* request ) {
    AppCreateReply reply;

    IPCListener::init();

    m_clientPort = new yutilpp::IPCPort();
    m_clientPort->createFromExisting(request->m_clientPort);

    reply.m_serverPort = getPort()->getId();
    yutilpp::IPCPort::sendTo(request->m_replyPort, 0, reinterpret_cast<void*>(&reply), sizeof(reply));

    return true;
}

FontNode* Application::getFont(int fontHandle) {
    FontMapCIter it = m_fontMap.find(fontHandle);

    if (it == m_fontMap.end()) {
        return NULL;
    }

    return it->second;
}

Bitmap* Application::getBitmap(int bitmapHandle) {
    BitmapMapCIter it = m_bitmapMap.find(bitmapHandle);

    if (it == m_bitmapMap.end()) {
        return NULL;
    }

    return it->second;
}

int Application::ipcDataAvailable( uint32_t code, void* data, size_t size ) {
    switch (code) {
        case Y_WINDOW_CREATE :
            handleWindowCreate(reinterpret_cast<WinCreate*>(data));
            break;

        case Y_WINDOW_SHOW :
        case Y_WINDOW_HIDE :
        case Y_WINDOW_DO_RESIZE :
        case Y_WINDOW_DO_MOVETO :
        case Y_WINDOW_RENDER : {
            WinHeader* header = reinterpret_cast<WinHeader*>(data);
            WindowMapCIter it = m_windowMap.find(header->m_windowId);

            if ( it != m_windowMap.end() ) {
                it->second->handleMessage(code, data, size);
            }

            break;
        }

        case Y_FONT_CREATE :
            handleFontCreate(reinterpret_cast<FontCreate*>(data));
            break;

        case Y_FONT_STRING_WIDTH :
            handleFontStringWidth(reinterpret_cast<FontStringWidth*>(data));
            break;

        case Y_BITMAP_CREATE :
            handleBitmapCreate(reinterpret_cast<BitmapCreate*>(data));
            break;

        case Y_DESKTOP_GET_SIZE :
            handleDesktopGetSize(reinterpret_cast<DesktopGetSize*>(data));
            break;
    }

    return 0;
}

Application* Application::createFrom( GuiServer* guiServer, AppCreate* request ) {
    Application* app = new Application(guiServer);
    app->init(request);
    return app;
}

int Application::handleWindowCreate( WinCreate* request ) {
    WinCreateReply reply;
    Window* window = Window::createFrom(m_guiServer, this, request);

    reply.m_windowId = getWindowId();
    m_windowMap[reply.m_windowId] = window;
    window->setId(reply.m_windowId);

    yutilpp::IPCPort::sendTo(request->m_replyPort, 0, reinterpret_cast<void*>(&reply), sizeof(reply));

    return 0;
}

int Application::handleFontCreate( FontCreate* request ) {
    char* family;
    char* style;
    FontNode* fontNode;
    FontCreateReply reply;

    family = reinterpret_cast<char*>(request + 1);
    style = family + strlen(family) + 1;
    fontNode = m_guiServer->getFontStorage()->getFontNode(family, style, request->m_fontInfo);

    if (fontNode == NULL) {
        reply.m_fontHandle = -1;
        yutilpp::IPCPort::sendTo(request->m_replyPort, 0, reinterpret_cast<void*>(&reply), sizeof(reply));
        return 0;
    }

    reply.m_fontHandle = getFontId();
    reply.m_ascender = fontNode->getAscender();
    reply.m_descender = fontNode->getDescender();
    reply.m_lineGap = fontNode->getLineGap();
    m_fontMap[reply.m_fontHandle] = fontNode;

    yutilpp::IPCPort::sendTo(request->m_replyPort, 0, reinterpret_cast<void*>(&reply), sizeof(reply));

    return 0;
}

int Application::handleFontStringWidth( FontStringWidth* request ) {
    FontStringWidthReply reply;

    FontMapCIter it = m_fontMap.find(request->m_fontHandle);
    if (it == m_fontMap.end()) {
        reply.m_width = 0;
        yutilpp::IPCPort::sendTo(request->m_replyPort, 0, reinterpret_cast<void*>(&reply), sizeof(reply));
        return 0;
    }

    reply.m_width = it->second->getWidth(reinterpret_cast<char*>(request + 1), request->m_length);
    yutilpp::IPCPort::sendTo(request->m_replyPort, 0, reinterpret_cast<void*>(&reply), sizeof(reply));

    return 0;
}

int Application::handleBitmapCreate( BitmapCreate* request ) {
    size_t size;
    void* buffer;
    Bitmap* bitmap;
    region_id region;
    BitmapCreateReply reply;

    size = request->m_size.m_x * request->m_size.m_y * colorspace_to_bpp(request->m_colorSpace);
    region = memory_region_create("bitmap", PAGE_ALIGN(size), REGION_READ | REGION_WRITE, &buffer);

    if (region < 0) {
        reply.m_bitmapHandle = -1;
        yutilpp::IPCPort::sendTo(request->m_replyPort, 0, reinterpret_cast<void*>(&reply), sizeof(reply));
        return 0;
    }

    if (memory_region_alloc_pages(region) != 0) {
        reply.m_bitmapHandle = -1;
        yutilpp::IPCPort::sendTo(request->m_replyPort, 0, reinterpret_cast<void*>(&reply), sizeof(reply));
        return 0;
    }

    bitmap = new Bitmap(request->m_size.m_x, request->m_size.m_y, request->m_colorSpace,
                        reinterpret_cast<uint8_t*>(buffer), region);
    reply.m_bitmapHandle = getBitmapId();
    reply.m_bitmapRegion = region;
    m_bitmapMap[reply.m_bitmapHandle] = bitmap;

    yutilpp::IPCPort::sendTo(request->m_replyPort, 0, reinterpret_cast<void*>(&reply), sizeof(reply));

    return 0;
}

int Application::handleDesktopGetSize( DesktopGetSize* request ) {
    yguipp::Point size = m_guiServer->getScreenBitmap()->size();

    yutilpp::IPCPort::sendTo(request->m_replyPort, 0, reinterpret_cast<void*>(&size), sizeof(size));

    return 0;
}

int Application::getWindowId( void ) {
    while ( m_windowMap.find(m_nextWinId) != m_windowMap.end() ) {
        m_nextWinId++;

        if (m_nextWinId < 0) {
            m_nextWinId = 0;
        }
    }

    return m_nextWinId;
}

int Application::getFontId( void ) {
    while ( m_fontMap.find(m_nextFontId) != m_fontMap.end() ) {
        m_nextFontId++;

        if (m_nextFontId < 0) {
            m_nextFontId = 0;
        }
    }

    return m_nextFontId;
}

int Application::getBitmapId( void ) {
    while ( m_bitmapMap.find(m_nextBitmapId) != m_bitmapMap.end() ) {
        m_nextBitmapId++;

        if (m_nextBitmapId < 0) {
            m_nextBitmapId = 0;
        }
    }

    return m_nextBitmapId;
}
