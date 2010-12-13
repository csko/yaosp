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

#include <yaosp/debug.h>
#include <ygui++/render.hpp>

#include <guiserver/window.hpp>
#include <guiserver/guiserver.hpp>
#include <guiserver/application.hpp>
#include <guiserver/windowmanager.hpp>

yguipp::Point Window::translateToWindow(const yguipp::Point& p) {
    yguipp::Point r = p;

    if ((m_flags & yguipp::WINDOW_NO_BORDER) == 0) {
        r += m_guiServer->getWindowManager()->getDecorator()->leftTop();
    }

    return r;
}

yguipp::Rect Window::translateToWindow(const yguipp::Rect& r) {
    yguipp::Rect r2 = r;

    if ((m_flags & yguipp::WINDOW_NO_BORDER) == 0) {
        r2 += m_guiServer->getWindowManager()->getDecorator()->leftTop();
    }

    return r2;
}

bool Window::handleRender(uint8_t* data, size_t size) {
    cairo_t* cr = m_renderContext.getCairoContext();
    bool renderingDone = false;
    uint8_t* dataEnd = data + size;

    while (data < dataEnd) {
        yguipp::RenderHeader* header = reinterpret_cast<yguipp::RenderHeader*>(data);

        switch (header->m_cmd) {
            case yguipp::R_SET_CLIP_RECT : {
                yguipp::Rect clipRect = translateToWindow(reinterpret_cast<yguipp::RSetClipRect*>(header)->m_clipRect);

                cairo_reset_clip(cr);
                cairo_rectangle(cr, clipRect.m_left, clipRect.m_top, clipRect.width(), clipRect.height());
                cairo_clip(cr);

                data += sizeof(yguipp::RSetClipRect);
                break;
            }

            case yguipp::R_SET_PEN_COLOR : {
                yguipp::Color penColor = reinterpret_cast<yguipp::RSetPenColor*>(header)->m_penColor;
                cairo_set_source_rgb(cr, penColor.m_red / 255.0f, penColor.m_green / 255.0f, penColor.m_blue / 255.0f);
                data += sizeof(yguipp::RSetPenColor);
                break;
            }

            case yguipp::R_SET_FONT :
                cairo_set_scaled_font(cr, m_application->getFont(reinterpret_cast<yguipp::RSetFont*>(header)->m_fontHandle));
                data += sizeof(yguipp::RSetFont);
                break;

            case yguipp::R_SET_LINE_WIDTH :
                cairo_set_line_width(cr, reinterpret_cast<yguipp::RSetLineWidth*>(data)->m_width);
                data += sizeof(yguipp::RSetLineWidth);
                break;

            case yguipp::R_SET_ANTIALIAS :
                switch (reinterpret_cast<yguipp::RSetAntiAlias*>(data)->m_mode) {
                    case yguipp::NONE : cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE); break;
                    case yguipp::GRAY : cairo_set_antialias(cr, CAIRO_ANTIALIAS_GRAY); break;
                    case yguipp::SUBPIXEL : cairo_set_antialias(cr, CAIRO_ANTIALIAS_SUBPIXEL); break;
                }

                data += sizeof(yguipp::RSetAntiAlias);
                break;

            case yguipp::R_MOVE_TO : {
                yguipp::Point p = translateToWindow(reinterpret_cast<yguipp::RMoveTo*>(data)->m_p);
                cairo_move_to(cr, p.m_x, p.m_y);
                data += sizeof(yguipp::RMoveTo);
                break;
            }

            case yguipp::R_LINE_TO : {
                yguipp::Point p = translateToWindow(reinterpret_cast<yguipp::RLineTo*>(data)->m_p);
                cairo_line_to(cr, p.m_x, p.m_y);
                data += sizeof(yguipp::RLineTo);
                break;
            }

            case yguipp::R_RECTANGLE : {
                yguipp::Rect r = translateToWindow(reinterpret_cast<yguipp::RRectangle*>(data)->m_rect);
                cairo_rectangle(cr, r.m_left, r.m_top, r.width(), r.height());
                data += sizeof(yguipp::RRectangle);
                break;
            }

            case yguipp::R_ARC : {
                yguipp::RArc* arc = reinterpret_cast<yguipp::RArc*>(data);
                yguipp::Point center = translateToWindow(arc->m_center);
                cairo_arc(cr, center.m_x, center.m_y, arc->m_radius, arc->m_angle1, arc->m_angle2);
                data += sizeof(yguipp::RArc);
                break;
            }

            case yguipp::R_CLOSE_PATH :
                cairo_close_path(cr);
                data += sizeof(yguipp::RenderHeader);
                break;

            case yguipp::R_STROKE :
                cairo_stroke(cr);
                data += sizeof(yguipp::RenderHeader);
                break;

            case yguipp::R_FILL :
                cairo_fill(cr);
                data += sizeof(yguipp::RenderHeader);
                break;

            case yguipp::R_FILL_PRESERVE :
                cairo_fill_preserve(cr);
                data += sizeof(yguipp::RenderHeader);
                break;

            case yguipp::R_SHOW_TEXT : {
                yguipp::RShowText* cmd = reinterpret_cast<yguipp::RShowText*>(data);
                char* s = reinterpret_cast<char*>(cmd + 1);

                cairo_show_text(cr, s);

                data += sizeof(yguipp::RShowText);
                data += strlen(s) + 1;
                break;
            }

            case yguipp::R_DONE :
                renderingDone = true;

                if (m_visible) {
                    m_guiServer->getWindowManager()->updateWindowRegion(this, m_screenRect);
                }

                data += sizeof(yguipp::RenderHeader);
                break;

            default :
                dbprintf( "Window::handleRender(): unknown command: %d\n", (int)header->m_cmd );
                return false;
        }
    }

    return renderingDone;
}
