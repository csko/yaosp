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

#ifndef _RENDERCONTEXT_HPP_
#define _RENDERCONTEXT_HPP_

#include <ygui++/color.hpp>

#include <guiserver/bitmap.hpp>

class RenderContext {
  public:
    RenderContext(void);
    ~RenderContext(void);

    bool init(Bitmap* bitmap);
    void finish(void);

    cairo_t* getCairoContext(void);

  private:
    cairo_t* m_cairo;
}; /* class RenderContext */

#endif /* _RENDERCONTEXT_HPP_ */
