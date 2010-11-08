/* yaosp GUI library
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

#ifndef _YGUIPP_APPLICATION_HPP_
#define _YGUIPP_APPLICATION_HPP_

#include <map>
#include <string>
#include <vector>

#include <ygui++/imageloader.hpp>
#include <ygui++/point.hpp>
#include <ygui++/yconstants.hpp>
#include <yutil++/ipcport.hpp>
#include <yutil++/thread/thread.hpp>
#include <yutil++/thread/mutex.hpp>

namespace yguipp {

class Window;

class ApplicationListener {
  public:
    virtual ~ApplicationListener(void) {}

    virtual void onScreenModeChanged(const ScreenModeInfo& modeInfo) = 0;
}; /* class ApplicationListener */

class Application {
  private:
    friend class Window;

    Application( const std::string& name );
    ~Application( void );

    bool init(void);

  public:
    void addListener(ApplicationListener* listener);

    void lock(void );
    void unLock(void);

    ScreenModeInfo getCurrentScreenMode(int desktopIndex = 0);
    int getScreenModeList(std::vector<ScreenModeInfo>& modeList);
    bool setScreenMode(const ScreenModeInfo& modeInfo);

    yutilpp::IPCPort* getGuiServerPort( void );
    yutilpp::IPCPort* getClientPort( void );
    yutilpp::IPCPort* getReplyPort( void );
    inline yutilpp::IPCPort* getServerPort( void ) { return m_serverPort; }

    int mainLoop(void);

    bool isEventDispatchThread(void);

    static bool createInstance( const std::string& name );
    static inline Application* getInstance(void) { return m_instance; }

  public:
    struct Message {
        static const size_t BUFFER_SIZE = 512;

        uint32_t m_code;
        int m_size;
        uint8_t m_buffer[BUFFER_SIZE];
    }; /* struct Message */

    bool receiveMessage(Message& msg);
    bool handleMessage(const Message& msg);

  private:
    bool registerApplication( void );
    int registerWindow(int id, Window* window);
    int unregisterWindow(int id);

    void handleScreenModeChanged(const void* buffer);

  private:
    yutilpp::thread::Mutex* m_lock;

    yutilpp::IPCPort* m_guiServerPort;
    yutilpp::IPCPort* m_clientPort;
    yutilpp::IPCPort* m_serverPort;
    yutilpp::IPCPort* m_replyPort;

    typedef std::map<int, Window*> WindowMap;
    typedef WindowMap::iterator WindowMapIter;
    typedef WindowMap::const_iterator WindowMapCIter;

    WindowMap m_windowMap;
    std::vector<ApplicationListener*> m_listeners;

    yutilpp::thread::Thread::Id m_mainLoopThread;

    static Application* m_instance;
}; /* class Application */

} /* namespace yguipp */

#endif /* _YGUIPP_APPLICATION_HPP_ */
