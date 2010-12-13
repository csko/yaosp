#ifndef _RENDER_HPP_
#define _RENDER_HPP_

#include <ygui++/color.hpp>
#include <ygui++/rect.hpp>
#include <ygui++/yconstants.hpp>

#define _PACKED __attribute__((packed))

namespace yguipp {

enum {
    R_SET_PEN_COLOR = 1,
    R_SET_LINE_WIDTH,
    R_SET_FONT,
    R_SET_CLIP_RECT,
    R_SET_ANTIALIAS,

    R_MOVE_TO,
    R_LINE_TO,
    R_RECTANGLE,
    R_ARC,

    R_CLOSE_PATH,

    R_STROKE,
    R_FILL,
    R_FILL_PRESERVE,
    R_SHOW_TEXT,
    R_SHOW_BITMAP,
    R_DONE
};

struct RenderHeader {
    uint8_t m_cmd;
} _PACKED;

struct RSetFont {
    RenderHeader m_header;
    int m_fontHandle;
} _PACKED;

struct RSetClipRect {
    RenderHeader m_header;
    Rect m_clipRect;
} _PACKED;

struct RSetAntiAlias {
    RenderHeader m_header;
    AntiAliasMode m_mode;
} _PACKED;

struct RShowText {
    RenderHeader m_header;
} _PACKED;

struct RShowBitmap {
    RenderHeader m_header;
    Point m_position;
    int m_handle;
} _PACKED;

struct RSetPenColor {
    RenderHeader m_header;
    Color m_penColor;
} _PACKED;

struct RSetLineWidth {
    RenderHeader m_header;
    double m_width;
} _PACKED;

struct RMoveTo {
    RenderHeader m_header;
    Point m_p;
} _PACKED;

struct RLineTo {
    RenderHeader m_header;
    Point m_p;
} _PACKED;

struct RRectangle {
    RenderHeader m_header;
    Rect m_rect;
} _PACKED;

struct RArc {
    RenderHeader m_header;
    Point m_center;
    double m_radius;
    double m_angle1;
    double m_angle2;
} _PACKED;

} /* namespace yguipp */

#endif /* _RENDER_HPP_ */
