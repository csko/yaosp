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

#ifndef _REGION_HPP_
#define _REGION_HPP_

#include <ygui++/rect.hpp>

class ClipRect {
  public:
    ClipRect( const yguipp::Rect& rect ) : m_rect(rect), m_next(NULL) {}
    ClipRect( ClipRect* rect ) : m_rect(rect->m_rect), m_next(NULL) {}

    yguipp::Rect m_rect;
    ClipRect* m_next;
}; /* class ClipRect */

class Region {
  public:
    Region( void );
    Region( const Region& r );
    ~Region( void );

    int clear( void );
    int add( const yguipp::Rect& rect );
    int exclude( const yguipp::Rect& rect );

    inline ClipRect* getClipRects( void ) { return m_clipRects; }

  private:
    Region& operator=( const Region& r );

  private:
    ClipRect* m_clipRects;
}; /* class Region */

#endif /* _REGION_HPP_ */
