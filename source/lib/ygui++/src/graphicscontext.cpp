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

#include <ygui/render/render.h>

#include <ygui++/graphicscontext.hpp>
#include <ygui++/window.hpp>

namespace yguipp {

GraphicsContext::GraphicsContext( Window* window ) : m_leftTop(0,0), m_clipRect(0, 0, 0, 0), m_penValid(false),
                                                     m_penColor(0, 0, 0), m_needToFlush(false), m_window(window) {
}

GraphicsContext::~GraphicsContext( void ) {
}

void GraphicsContext::setPenColor( const Color& pen ) {
    r_set_pen_color_t* cmd;

    if ( ( m_penValid ) &&
         ( m_penColor == pen ) ) {
        return;
    }

    m_penColor = pen;
    m_penValid = true;

    cmd = reinterpret_cast<r_set_pen_color_t*>( m_window->getRenderTable()->allocate( sizeof(r_set_pen_color_t) ) );
    cmd->header.command = R_SET_PEN_COLOR;
    m_penColor.toColorT( &cmd->color );
}

void GraphicsContext::setClipRect( const Rect& rect ) {
    r_set_clip_rect_t* cmd;

    cmd = reinterpret_cast<r_set_clip_rect_t*>( m_window->getRenderTable()->allocate( sizeof(r_set_clip_rect_t) ) );
    cmd->header.command = R_SET_CLIP_RECT;
    rect.toRectT( &cmd->clip_rect );
}

void GraphicsContext::fillRect( const Rect& r ) {
    r_fill_rect_t* cmd;

    cmd = reinterpret_cast<r_fill_rect_t*>( m_window->getRenderTable()->allocate( sizeof(r_fill_rect_t) ) );
    cmd->header.command = R_FILL_RECT;
    r.toRectT( &cmd->rect );

    m_needToFlush = true;
}

void GraphicsContext::flush( void ) {

    if ( m_needToFlush ) {
        render_header_t* cmd;

        cmd = reinterpret_cast<render_header_t*>( m_window->getRenderTable()->allocate( sizeof(render_header_t) ) );
        cmd->command = R_DONE;

        m_window->getRenderTable()->flush();
    } else {
        m_window->getRenderTable()->reset();
    }
}

} /* namespace yguipp */
