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

#include <assert.h>
#include <string.h>

#include <ygui++/graphicscontext.hpp>
#include <ygui++/window.hpp>
#include <ygui++/render.hpp>

namespace yguipp {

GraphicsContext::GraphicsContext( Window* window ) : m_leftTop(0,0), m_clipRect(0, 0, 0, 0), m_penValid(false),
                                                     m_penColor(0, 0, 0), m_window(window) {
    m_renderTable = window->getRenderTable();
}

GraphicsContext::~GraphicsContext( void ) {
}

const Point& GraphicsContext::getLeftTop( void ) {
    return m_leftTop;
}

void GraphicsContext::setClipRect( const Rect& rect ) {
    RSetClipRect* cmd;
    const Rect& resArea = currentRestrictedArea();

    m_clipRect = (rect + m_leftTop) & resArea;

    cmd = reinterpret_cast<RSetClipRect*>( m_window->getRenderTable()->allocate( sizeof(RSetClipRect) ) );
    cmd->m_header.m_cmd = R_SET_CLIP_RECT;
    cmd->m_clipRect = m_clipRect;
}

void GraphicsContext::setFont( Font* font ) {
    RSetFont* cmd;

    cmd = reinterpret_cast<RSetFont*>( m_window->getRenderTable()->allocate( sizeof(RSetFont) ) );
    cmd->m_header.m_cmd = R_SET_FONT;
    cmd->m_fontHandle = font->getHandle();
}

void GraphicsContext::translate( const Point& p ) {
    m_translateStack.push( TranslateItem(p) );
    m_leftTop += p;
}

void GraphicsContext::setPenColor( const Color& pen ) {
    RSetPenColor* cmd;

    if ((m_penValid) &&
         (m_penColor == pen)) {
        return;
    }

    m_penColor = pen;
    m_penValid = true;

    cmd = reinterpret_cast<RSetPenColor*>( m_window->getRenderTable()->allocate( sizeof(RSetPenColor) ) );
    cmd->m_header.m_cmd = R_SET_PEN_COLOR;
    cmd->m_penColor = pen;
}

void GraphicsContext::setLineWidth(double width) {
    RSetLineWidth* cmd = reinterpret_cast<RSetLineWidth*>(m_renderTable->allocate(sizeof(RSetLineWidth)));
    cmd->m_header.m_cmd = R_SET_LINE_WIDTH;
    cmd->m_width = width;
}

void GraphicsContext::setAntiAlias(AntiAliasMode mode) {
    RSetAntiAlias* cmd = reinterpret_cast<RSetAntiAlias*>(m_renderTable->allocate(sizeof(RSetAntiAlias)));
    cmd->m_header.m_cmd = R_SET_ANTIALIAS;
    cmd->m_mode = mode;
}

void GraphicsContext::moveTo(const Point& p) {
    RMoveTo* cmd = reinterpret_cast<RMoveTo*>(m_renderTable->allocate(sizeof(RMoveTo)));
    cmd->m_header.m_cmd = R_MOVE_TO;
    cmd->m_p = p + m_leftTop;
}

void GraphicsContext::lineTo(const Point& p) {
    RLineTo* cmd = reinterpret_cast<RLineTo*>(m_renderTable->allocate(sizeof(RLineTo)));
    cmd->m_header.m_cmd = R_LINE_TO;
    cmd->m_p = p + m_leftTop;
}

void GraphicsContext::rectangle(const Rect& r) {
    RRectangle* cmd = reinterpret_cast<RRectangle*>(m_renderTable->allocate(sizeof(RRectangle)));
    cmd->m_header.m_cmd = R_RECTANGLE;
    cmd->m_rect = r + m_leftTop;
}

void GraphicsContext::arc(const Point& center, double radius, double angle1, double angle2) {
    RArc* cmd = reinterpret_cast<RArc*>(m_renderTable->allocate(sizeof(RArc)));
    cmd->m_header.m_cmd = R_ARC;
    cmd->m_center = center + m_leftTop;
    cmd->m_radius = radius;
    cmd->m_angle1 = angle1;
    cmd->m_angle2 = angle2;
}

void GraphicsContext::closePath(void) {
    RenderHeader* cmd = reinterpret_cast<RenderHeader*>(m_renderTable->allocate(sizeof(RenderHeader)));
    cmd->m_cmd = R_CLOSE_PATH;
}

void GraphicsContext::stroke(void) {
    RenderHeader* cmd = reinterpret_cast<RenderHeader*>(m_renderTable->allocate(sizeof(RenderHeader)));
    cmd->m_cmd = R_STROKE;
}

void GraphicsContext::fill(void) {
    RenderHeader* cmd = reinterpret_cast<RenderHeader*>(m_renderTable->allocate(sizeof(RenderHeader)));
    cmd->m_cmd = R_FILL;
}

void GraphicsContext::fillPreserve(void) {
    RenderHeader* cmd = reinterpret_cast<RenderHeader*>(m_renderTable->allocate(sizeof(RenderHeader)));
    cmd->m_cmd = R_FILL_PRESERVE;
}

void GraphicsContext::showText(const std::string& text) {
    size_t size;
    uint8_t* data;
    RShowText* cmd;
    size_t length = text.length();

    size = sizeof(RShowText) + length + 1;
    data = reinterpret_cast<uint8_t*>(m_window->getRenderTable()->allocate(size));
    cmd = reinterpret_cast<RShowText*>(data);

    cmd->m_header.m_cmd = R_SHOW_TEXT;
    memcpy(reinterpret_cast<void*>(cmd + 1), text.data(), length);
    data[size - 1] = 0;
}

void GraphicsContext::showBitmap(const Point& p, Bitmap* bitmap) {
    RShowBitmap* cmd = reinterpret_cast<RShowBitmap*>(m_renderTable->allocate(sizeof(RShowBitmap)));
    cmd->m_header.m_cmd = R_SHOW_BITMAP;
    cmd->m_position = p + m_leftTop;
    cmd->m_handle = bitmap->getHandle();
}

void GraphicsContext::finish(void) {
    RenderHeader* cmd = reinterpret_cast<RenderHeader*>(m_window->getRenderTable()->allocate(sizeof(RenderHeader)));
    cmd->m_cmd = R_DONE;
}

void GraphicsContext::pushRestrictedArea( const Rect& rect ) {
    RSetClipRect* cmd;

    m_restrictedAreas.push(rect);
    m_clipRect = rect;

    cmd = reinterpret_cast<RSetClipRect*>( m_window->getRenderTable()->allocate( sizeof(RSetClipRect) ) );
    cmd->m_header.m_cmd = R_SET_CLIP_RECT;
    cmd->m_clipRect = rect;
}

void GraphicsContext::popRestrictedArea( void ) {
    m_restrictedAreas.pop();
}

const Rect& GraphicsContext::currentRestrictedArea( void ) {
    assert( !m_restrictedAreas.empty() );
    return m_restrictedAreas.top();
}

void GraphicsContext::translateCheckPoint( void ) {
    m_translateStack.push( TranslateItem() );
}

void GraphicsContext::rollbackTranslate( void ) {
    bool done = false;

    while ( !done ) {
        TranslateItem item = m_translateStack.top();
        m_translateStack.pop();

        switch ( item.m_type ) {
            case TRANSLATE :
                m_leftTop -= item.m_point;
                break;

            case CHECKPOINT :
                done = true;
                break;
        }
    }
}

void GraphicsContext::cleanUp(void) {
    while (!m_restrictedAreas.empty()) {
        m_restrictedAreas.pop();
    }

    while (!m_translateStack.empty()) {
        m_translateStack.pop();
    }
}

} /* namespace yguipp */
