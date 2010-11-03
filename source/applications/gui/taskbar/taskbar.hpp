/* Taskbar application
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

#ifndef _TASKBAR_HPP_
#define _TASKBAR_HPP_

#include <ygui++/widget.hpp>
#include <ygui++/bitmap.hpp>
#include <ygui++/menu.hpp>
#include <ygui++/application.hpp>
#include <ygui++/event/actionlistener.hpp>

class MenuButton : public yguipp::Widget, public yguipp::event::ActionSpeaker {
  public:
    MenuButton(void);

    yguipp::Point getPreferredSize(void);
    int paint(yguipp::GraphicsContext* g);
    int mousePressed(const yguipp::Point& p, int button);

  private:
    yguipp::Bitmap* m_image;
}; /* class MenuButton */

class TaskBar : public yguipp::event::ActionListener,
                public yguipp::ApplicationListener {
  public:
    int run(void);

    int onActionPerformed(yguipp::Widget* widget);

    void onScreenModeChanged(const yguipp::ScreenModeInfo& modeInfo);

  private:
    void createMenu(void);

  private:
    yguipp::Window* m_window;
    MenuButton* m_menuButton;
    yguipp::Menu* m_menu;
}; /* class TaskBar */

#endif /* _TASKBAR_HPP_ */
