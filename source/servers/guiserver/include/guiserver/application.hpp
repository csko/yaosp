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

#ifndef _APPLICATION_HPP_
#define _APPLICATION_HPP_

#include <map>

#include <ygui++/protocol.hpp>
#include <yutil++/ipclistener.hpp>

class Window;
class GuiServer;
class FontNode;
class Bitmap;

class Application : public yutilpp::IPCListener {
  private:
    Application( GuiServer* guiServer );
    ~Application( void );

    bool init( AppCreate* request );

  public:
    FontNode* getFont(int fontHandle);
    Bitmap* getBitmap(int bitmapHandle);
    inline yutilpp::IPCPort* getClientPort(void) { return m_clientPort; }

    int ipcDataAvailable( uint32_t code, void* data, size_t size );

    static Application* createFrom( GuiServer* guiServer, AppCreate* request );

  private:
    int handleWindowCreate(WinCreate* request);
    int handleWindowDestroy(Window* window);

    int handleFontCreate( FontCreate* request );
    int handleFontStringWidth( FontStringWidth* request );
    int handleBitmapCreate( BitmapCreate* request );
    int handleDesktopGetSize( DesktopGetSize* request );
    int handleScreenModeGetList(ScreenModeGetList* request);
    int handleScreenModeSet(ScreenModeSet* request);

    int getWindowId( void );
    int getFontId( void );
    int getBitmapId( void );

  private:
    yutilpp::IPCPort* m_clientPort;

    typedef std::map<int, Window*> WindowMap;
    typedef WindowMap::iterator WindowMapIter;
    typedef WindowMap::const_iterator WindowMapCIter;
    typedef std::map<int, FontNode*> FontMap;
    typedef FontMap::const_iterator FontMapCIter;
    typedef std::map<int, Bitmap*> BitmapMap;
    typedef BitmapMap::const_iterator BitmapMapCIter;

    int m_nextWinId;
    WindowMap m_windowMap;
    int m_nextFontId;
    FontMap m_fontMap;
    int m_nextBitmapId;
    BitmapMap m_bitmapMap;

    GuiServer* m_guiServer;
}; /* class Application */

#endif /* _APPLICATION_HPP_ */
