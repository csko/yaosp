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

#ifndef _IMAGE_HPP_
#define _IMAGE_HPP_

#include <ygui++/bitmap.hpp>
#include <ygui++/widget.hpp>

namespace yguipp {

class Image : public Widget {
  public:
    Image( Bitmap* bitmap = NULL );
    virtual ~Image( void );

    Point getPreferredSize( void );

    int paint( GraphicsContext* g );

  private:
    Bitmap* m_bitmap;
}; /* class Image */

} /* namespace yguipp */

#endif /* _IMAGE_HPP_ */
