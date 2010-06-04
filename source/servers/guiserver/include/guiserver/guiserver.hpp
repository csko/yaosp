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

#ifndef _GUISERVER_HPP_
#define _GUISERVER_HPP_

#include <yutil++/ipcport.hpp>

#include <guiserver/graphicsdriver.hpp>
#include <guiserver/windowmanager.hpp>
#include <guiserver/input.hpp>
#include <guiserver/font.hpp>

class GuiServer {
  public:
    GuiServer( void );

    int run( void );

    inline GraphicsDriver* getGraphicsDriver(void) { return m_graphicsDriver; }
    inline Bitmap* getScreenBitmap(void) { return m_screenBitmap; }
    inline WindowManager* getWindowManager(void) { return m_windowManager; }
    inline FontStorage* getFontStorage(void) { return m_fontStorage; }

  private:
    GraphicsDriver* m_graphicsDriver;
    Bitmap* m_screenBitmap;
    WindowManager* m_windowManager;
    InputThread* m_inputThread;
    FontStorage* m_fontStorage;
    yutilpp::IPCPort* m_serverPort;

    static GuiServer* instance;
}; /* class GuiServer */

#endif /* _GUISERVER_HPP_ */
