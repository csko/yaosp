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
                                                     m_penColor(0, 0, 0), m_needToFlush(false), m_window(window) {
}

GraphicsContext::~GraphicsContext( void ) {
}

const Point& GraphicsContext::getLeftTop( void ) {
    return m_leftTop;
}

bool GraphicsContext::needToFlush( void ) {
    return m_needToFlush;
}

void GraphicsContext::setPenColor( const Color& pen ) {
    RSetPenColor* cmd;

    if ( ( m_penValid ) &&
         ( m_penColor == pen ) ) {
        return;
    }

    m_penColor = pen;
    m_penValid = true;

    cmd = reinterpret_cast<RSetPenColor*>( m_window->getRenderTable()->allocate( sizeof(RSetPenColor) ) );
    cmd->m_header.m_cmd = R_SET_PEN_COLOR;
    cmd->m_penColor = pen;
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

void GraphicsContext::setDrawingMode( DrawingMode mode ) {
    RSetDrawingMode* cmd;

    cmd = reinterpret_cast<RSetDrawingMode*>( m_window->getRenderTable()->allocate( sizeof(RSetDrawingMode) ));
    cmd->m_header.m_cmd = R_SET_DRAWING_MODE;
    cmd->m_drawingMode = mode;
}

void GraphicsContext::translate( const Point& p ) {
    m_translateStack.push( TranslateItem(p) );
    m_leftTop += p;
}

void GraphicsContext::fillRect( const Rect& r ) {
    RFillRect* cmd;
    Rect visibleRect;

    visibleRect = (r + m_leftTop) & m_clipRect;

    if (!visibleRect.isValid()) {
        return;
    }

    cmd = reinterpret_cast<RFillRect*>( m_window->getRenderTable()->allocate( sizeof(RFillRect) ) );
    cmd->m_header.m_cmd = R_FILL_RECT;
    cmd->m_rect = visibleRect;

    m_needToFlush = true;
}

void GraphicsContext::drawRect( const Rect& r ) {
    RDrawRect* cmd;
    Rect visibleRect;

    visibleRect = (r + m_leftTop) & m_clipRect;

    if (!visibleRect.isValid()) {
        return;
    }

    cmd = reinterpret_cast<RDrawRect*>( m_window->getRenderTable()->allocate( sizeof(RDrawRect) ) );
    cmd->m_header.m_cmd = R_DRAW_RECT;
    cmd->m_rect = r;

    m_needToFlush = true;
}

void GraphicsContext::drawBitmap( const Point& p, Bitmap* bitmap ) {
    RDrawBitmap* cmd;
    Rect visibleRect;
    Point bitmapLeftTop = m_leftTop + p;

    visibleRect = (bitmap->bounds() + bitmapLeftTop) & m_clipRect;

    if (!visibleRect.isValid()) {
        return;
    }

    cmd = reinterpret_cast<RDrawBitmap*>( m_window->getRenderTable()->allocate( sizeof(RDrawBitmap) ) );
    cmd->m_header.m_cmd = R_DRAW_BITMAP;
    cmd->m_position = bitmapLeftTop;
    cmd->m_bitmapHandle = bitmap->getHandle();

    m_needToFlush = true;
}

void GraphicsContext::drawText( const Point& p, const std::string& text, int length ) {
    RDrawText* cmd;

    if (length == 0) {
        return;
    } else if (length == -1) {
        length = (int)text.size();
    }

    cmd = reinterpret_cast<RDrawText*>( m_window->getRenderTable()->allocate( sizeof(RDrawText) + length ) );
    cmd->m_header.m_cmd = R_DRAW_TEXT;
    cmd->m_position = p + m_leftTop;
    cmd->m_length = length;
    memcpy(reinterpret_cast<void*>(cmd + 1), text.data(), length);

    m_needToFlush = true;
}

void GraphicsContext::finish( void ) {
    if ( m_needToFlush ) {
        RenderHeader* cmd;

        cmd = reinterpret_cast<RenderHeader*>( m_window->getRenderTable()->allocate( sizeof(RenderHeader) ) );
        cmd->m_cmd = R_DONE;
    }
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

void GraphicsContext::cleanUp( void ) {
    while ( !m_restrictedAreas.empty() ) {
        m_restrictedAreas.pop();
    }

    while ( !m_translateStack.empty() ) {
        m_translateStack.pop();
    }
}

} /* namespace yguipp */
