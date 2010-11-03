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

#ifndef _YGUIPP_EVENT_KEYLISTENER_HPP_
#define _YGUIPP_EVENT_KEYLISTENER_HPP_

#include <vector>

namespace yguipp {

class Widget;

namespace event {

class KeyListener {
  public:
    virtual ~KeyListener(void) {}

    virtual int onKeyPressed(Widget* widget, int key) = 0;
    virtual int onKeyReleased(Widget* widget, int key) = 0;
}; /* class KeyListener */

class KeySpeaker {
  public:
    virtual ~KeySpeaker(void) {}

    void addKeyListener(KeyListener* listener);
    void removeKeyListener(KeyListener* listener);

  protected:
    void fireKeyPressedListeners(Widget* widget, int key);
    void fireKeyReleasedListeners(Widget* widget, int key);

  private:
    std::vector<KeyListener*> m_listeners;
}; /* class KeySpeaker */

} /* namespace event */
} /* namespace yguipp */

#endif /* _YGUIPP_EVENT_KEYLISTENER_HPP_ */
