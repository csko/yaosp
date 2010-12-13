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

#include <guiserver/rendercontext.hpp>

RenderContext::RenderContext(void) : m_cairo(NULL) {
}

RenderContext::~RenderContext(void) {
}

bool RenderContext::init(Bitmap* bitmap) {
    if (m_cairo != NULL) {
        return false;
    }

    m_cairo = cairo_create(bitmap->getSurface());
    cairo_set_antialias(m_cairo, CAIRO_ANTIALIAS_NONE);

    return true;
}

void RenderContext::finish(void) {
    if (m_cairo == NULL) {
        return;
    }

    cairo_destroy(m_cairo);
    m_cairo = NULL;
}

cairo_t* RenderContext::getCairoContext(void) {
    return m_cairo;
}
