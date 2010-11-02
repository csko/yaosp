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

#ifndef _YGUIPP_EVENT_ACTIONLISTENER_HPP_
#define _YGUIPP_EVENT_ACTIONLISTENER_HPP_

#include <vector>

namespace yguipp {

class Widget;

namespace event {

class ActionListener {
  public:
    virtual ~ActionListener(void) {}

    virtual int actionPerformed(Widget* widget) = 0;
}; /* class ActionListener */

class ActionSpeaker {
  public:
    virtual ~ActionSpeaker(void) {}

    void addActionListener(ActionListener* listener);
    void removeActionListener(ActionListener* listener);

  protected:
    void fireActionListeners(Widget* widget);

  private:
    std::vector<ActionListener*> m_listeners;
}; /* class ActionSpeaker */

} /* namespace event */
} /* namespace yguipp */

#endif /* _YGUIPP_EVENT_ACTIONLISTENER_HPP_ */
