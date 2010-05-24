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

#include <guiserver/region.hpp>

Region::Region( void ) : m_clipRects(NULL) {
}

Region::~Region( void ) {
    clear();
}

int Region::clear( void ) {
    while ( m_clipRects != NULL ) {
        ClipRect* clipRect = m_clipRects;
        m_clipRects = clipRect->m_next;
        delete clipRect;
    }

    return 0;
}

int Region::add( const yguipp::Rect& rect ) {
    ClipRect* clipRect = new ClipRect(rect);
    clipRect->m_next = m_clipRects;
    m_clipRects = clipRect;

    return 0;
}

int Region::exclude( const yguipp::Rect& rect ) {
    ClipRect* newList = NULL;

    while ( m_clipRects != NULL ) {
        ClipRect* current = m_clipRects;
        m_clipRects = current->m_next;

        yguipp::Rect hidden = current->m_rect & rect;

        if ( !hidden.isValid() ) {
            current->m_next = newList;
            newList = current;

            continue;
        }

        yguipp::Rect newRects[4] = {
            yguipp::Rect( current->m_rect.m_left, current->m_rect.m_top,
                          current->m_rect.m_right, hidden.m_top - 1 ),
            yguipp::Rect( current->m_rect.m_left, hidden.m_bottom + 1,
                          current->m_rect.m_right, current->m_rect.m_bottom ),
            yguipp::Rect( current->m_rect.m_left, current->m_rect.m_top,
                          hidden.m_left - 1, hidden.m_bottom ),
            yguipp::Rect( hidden.m_right + 1, hidden.m_top,
                          current->m_rect.m_right, current->m_rect.m_bottom )
        };

        delete current;

        for ( int i = 0; i < 4; i++ ) {
            if ( !newRects[i].isValid() ) {
                continue;
            }

            ClipRect* clipRect = new ClipRect(newRects[i]);
            clipRect->m_next = newList;
            newList = clipRect;
        }
    }

    m_clipRects = newList;

    return 0;
}
