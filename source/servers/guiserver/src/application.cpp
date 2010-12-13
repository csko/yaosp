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

#include <assert.h>
#include <yaosp/debug.h>
#include <ygui++/ipcrendertable.hpp>

#include <guiserver/application.hpp>
#include <guiserver/window.hpp>
#include <guiserver/guiserver.hpp>
#include <guiserver/bitmap.hpp>
#include <guiserver/windowmanager.hpp>

#include <math.h>

Application::Application( GuiServer* guiServer ) : IPCListener("app", MAX_PACKET_SIZE), m_clientPort(NULL),
                                                   m_nextWinId(0), m_nextFontId(0), m_nextBitmapId(0),
                                                   m_guiServer(guiServer) {
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

cairo_scaled_font_t* Application::getFont(int fontHandle) {
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

void Application::screenModeChanged(const yguipp::ScreenModeInfo& modeInfo) {
    m_clientPort->send(Y_SCREEN_MODE_CHANGED, &modeInfo, sizeof(modeInfo));
}

int Application::ipcDataAvailable(uint32_t code, void* data, size_t size) {
    switch (code) {
        case Y_APPLICATION_DESTROY :
            m_guiServer->removeApplication(this);
            stopListener();
            break;

        case Y_WINDOW_CREATE :
            handleWindowCreate(reinterpret_cast<WinCreate*>(data));
            break;

        case Y_WINDOW_DESTROY : {
            WinHeader* header = reinterpret_cast<WinHeader*>(data);
            WindowMapCIter it = m_windowMap.find(header->m_windowId);

            if (it != m_windowMap.end()) {
                Window* window = it->second;
                handleWindowDestroy(window);
            }

            break;
        }

        case Y_WINDOW_SHOW :
        case Y_WINDOW_HIDE :
        case Y_WINDOW_DO_RESIZE :
        case Y_WINDOW_DO_MOVETO :
        case Y_WINDOW_RENDER : {
            WinHeader* header = reinterpret_cast<WinHeader*>(data);
            WindowMapCIter it = m_windowMap.find(header->m_windowId);

            if (it != m_windowMap.end()) {
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

        case Y_SCREEN_MODE_GET_LIST :
            handleScreenModeGetList(reinterpret_cast<ScreenModeGetList*>(data));
            break;

        case Y_SCREEN_MODE_SET :
            handleScreenModeSet(reinterpret_cast<ScreenModeSet*>(data));
            break;
    }

    return 0;
}

Application* Application::createFrom( GuiServer* guiServer, AppCreate* request ) {
    Application* app = new Application(guiServer);
    app->init(request);
    return app;
}

int Application::handleWindowCreate(WinCreate* request) {
    WinCreateReply reply;
    Window* window = Window::createFrom(m_guiServer, this, request);

    reply.m_windowId = getWindowId();
    m_windowMap[reply.m_windowId] = window;
    window->setId(reply.m_windowId);

    yutilpp::IPCPort::sendTo(request->m_replyPort, 0, reinterpret_cast<void*>(&reply), sizeof(reply));

    return 0;
}

int Application::handleWindowDestroy(Window* window) {
    window->close();

    WindowMapIter it = m_windowMap.find(window->getId());
    assert(it != m_windowMap.end());
    m_windowMap.erase(it);

    delete window;

    return 0;
}

int Application::handleFontCreate( FontCreate* request ) {
    char* family;
    char* style;
    FontCreateReply reply;

    family = reinterpret_cast<char*>(request + 1);
    style = family + strlen(family) + 1;

    int fontHandle = getFontId();
    cairo_font_face_t* fontFace = m_guiServer->getFontStorage()->getCairoFontFace(family, style);

    if (fontFace == NULL) {
        reply.m_fontHandle = -1;
    } else {
        cairo_matrix_t fm;
        cairo_matrix_t ctm;
        cairo_matrix_init_scale(&fm, request->m_fontInfo.m_pointSize, request->m_fontInfo.m_pointSize);
        cairo_matrix_init_identity(&ctm);

        cairo_font_options_t* options = cairo_font_options_create();
        cairo_font_options_set_antialias(options, CAIRO_ANTIALIAS_SUBPIXEL);
        cairo_font_options_set_hint_style(options, CAIRO_HINT_STYLE_FULL);

        cairo_scaled_font_t* scaledFont = cairo_scaled_font_create(fontFace, &fm, &ctm, options);
        cairo_font_options_destroy(options);

        cairo_font_extents_t extents;
        cairo_scaled_font_extents(scaledFont, &extents);

        reply.m_fontHandle = fontHandle;
        reply.m_ascender = extents.ascent;
        reply.m_descender = -extents.descent;
        reply.m_height = extents.height;

        m_fontMap[reply.m_fontHandle] = scaledFont;
    }

    yutilpp::IPCPort::sendTo(request->m_replyPort, 0, reinterpret_cast<void*>(&reply), sizeof(reply));

    return 0;
}

int Application::handleFontStringWidth( FontStringWidth* request ) {
    FontStringWidthReply reply;
    cairo_scaled_font_t* scaledFont = getFont(request->m_fontHandle);

    if (scaledFont == NULL) {
        reply.m_width = 0;
    } else {
        cairo_text_extents_t extents;
        cairo_scaled_font_text_extents(scaledFont, reinterpret_cast<char*>(request + 1), &extents);
        reply.m_width = extents.x_advance;
    }

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
    Bitmap* screen;
    yguipp::ScreenModeInfo modeInfo;

    screen = m_guiServer->getScreenBitmap();
    modeInfo.m_width = screen->width();
    modeInfo.m_height = screen->height();
    modeInfo.m_colorSpace = screen->getColorSpace();

    yutilpp::IPCPort::sendTo(request->m_replyPort, 0, reinterpret_cast<void*>(&modeInfo), sizeof(modeInfo));

    return 0;
}

int Application::handleScreenModeGetList(ScreenModeGetList* request) {
    GraphicsDriver* driver = m_guiServer->getGraphicsDriver();
    size_t modeCount = driver->getModeCount();

    if (modeCount == 0) {
        yutilpp::IPCPort::sendTo(request->m_replyPort, 0, NULL, 0);
        return 0;
    }

    yguipp::ScreenModeInfo* infoTable = new yguipp::ScreenModeInfo[modeCount];

    for (size_t i = 0; i < modeCount; i++) {
        ScreenMode* mode = driver->getModeInfo(i);

        infoTable[i].m_width = mode->m_width;
        infoTable[i].m_height = mode->m_height;
        infoTable[i].m_colorSpace = mode->m_colorSpace;
    }

    yutilpp::IPCPort::sendTo(request->m_replyPort, 0, reinterpret_cast<void*>(infoTable), sizeof(yguipp::ScreenModeInfo) * modeCount);
    delete[] infoTable;

    return 0;
}

int Application::handleScreenModeSet(ScreenModeSet* request) {
    int result;

    result = m_guiServer->changeScreenMode(request->m_modeInfo);
    yutilpp::IPCPort::sendTo(request->m_replyPort, 0, &result, sizeof(result));

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
