#ifndef _RENDER_HPP_
#define _RENDER_HPP_

#include <ygui++/color.hpp>
#include <ygui++/rect.hpp>
#include <ygui++/yconstants.hpp>

namespace yguipp {

enum {
    R_SET_PEN_COLOR = 1,
    R_SET_FONT,
    R_SET_CLIP_RECT,
    R_SET_DRAWING_MODE,
    R_DRAW_RECT,
    R_FILL_RECT,
    R_DRAW_TEXT,
    R_DRAW_BITMAP,
    R_DONE
};

struct RenderHeader {
    uint8_t m_cmd;
} __attribute__((packed));

struct RSetPenColor {
    RenderHeader m_header;
    Color m_penColor;
} __attribute__((packed));

struct RSetFont {
    RenderHeader m_header;
    int m_fontHandle;
} __attribute__((packed));

struct RSetClipRect {
    RenderHeader m_header;
    Rect m_clipRect;
} __attribute__((packed));

struct RSetDrawingMode {
    RenderHeader m_header;
    DrawingMode m_drawingMode;
} __attribute__((packed));

struct RDrawRect {
    RenderHeader m_header;
    Rect m_rect;
} __attribute__((packed));

struct RFillRect {
    RenderHeader m_header;
    Rect m_rect;
} __attribute__((packed));

struct RDrawText {
    RenderHeader m_header;
    Point m_position;
    int m_length;
} __attribute__((packed));

} /* namespace yguipp */

#endif /* _RENDER_HPP_ */
