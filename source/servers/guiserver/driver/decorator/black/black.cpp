#include <yaosp/debug.h>
#include <guiserver/graphicsdriver.hpp>

#include "black.hpp"

yguipp::Color Black::m_headerColors[SIZE_HEADER] = {
    yguipp::Color(46, 46, 46),
    yguipp::Color(69, 69, 69),
    yguipp::Color(96, 96, 96),
    yguipp::Color(100, 100, 100),
    yguipp::Color(104, 104, 104),
    yguipp::Color(108, 108, 108),
    yguipp::Color(111, 111, 111),
    yguipp::Color(111, 111, 111),
    yguipp::Color(111, 111, 111),
    yguipp::Color(111, 111, 111),
    yguipp::Color(111, 111, 111),
    yguipp::Color(90, 90, 90),
    yguipp::Color(34, 34, 34),
    yguipp::Color(0, 0, 0),
    yguipp::Color(0, 0, 0),
    yguipp::Color(0, 0, 0),
    yguipp::Color(0, 0, 0),
    yguipp::Color(0, 0, 0),
    yguipp::Color(0, 0, 0),
    yguipp::Color(0, 0, 0),
    yguipp::Color(0, 0, 0),
    yguipp::Color(46, 46, 46),
    yguipp::Color(46, 46, 46)
};

Black::Black(GuiServer* guiServer) : Decorator(guiServer) {
    m_minimizeImage = new Bitmap(23, 23, yguipp::CS_RGB32, m_minimizeButton);
    m_maximizeImage = new Bitmap(23, 23, yguipp::CS_RGB32, m_maximizeButton);
    m_closeImage = new Bitmap(23, 23, yguipp::CS_RGB32, m_closeButton);
    m_titleFont = guiServer->getFontStorage()->getScaledFont("DejaVu Sans", "Bold", 11);
}

Black::~Black(void) {
    delete m_minimizeImage;
    delete m_maximizeImage;
    delete m_closeImage;
    delete m_titleFont;
}

yguipp::Point Black::leftTop(void) {
    return yguipp::Point(0,SIZE_HEADER);
}

yguipp::Point Black::getSize(void) {
    return yguipp::Point(0,SIZE_HEADER);
}

bool Black::calculateItemPositions(Window* window) {
    DecoratorData* data = window->getDecoratorData();
    const yguipp::Rect& screenRect = window->getScreenRect();

    yguipp::Rect r = m_closeImage->bounds();
    r += screenRect.leftTop() + yguipp::Point(window->getBitmap()->width() - m_closeImage->width(), 0);

    data->m_closeRect = r;
    r -= yguipp::Point(m_closeImage->width(), 0);
    data->m_maximizeRect = r;
    r -= yguipp::Point(m_maximizeImage->width(), 0);
    data->m_minimizeRect = r;
    data->m_dragRect = yguipp::Rect(screenRect.m_left, screenRect.m_top, data->m_closeRect.m_left - 1, screenRect.m_top + SIZE_HEADER - 1);

    return true;
}

bool Black::update(GraphicsDriver* driver, Window* window) {
    Bitmap* bitmap = window->getBitmap();
    RenderContext renderContext;
    renderContext.init(bitmap);
    cairo_t* cr = renderContext.getCairoContext();

    /* Window header background */
    for (int i = 0; i < SIZE_HEADER; i++) {
        const yguipp::Color& c = m_headerColors[i];
        cairo_rectangle(cr, 0, i, bitmap->width(), 1);
        cairo_set_source_rgb(cr, c.m_red / 255.0f, c.m_green / 255.0f, c.m_blue / 255.0f);
        cairo_fill(cr);
    }

    /* Window header buttons */
/*
    yguipp::Point buttonPos(bitmap->width(), 0);

    buttonPos.m_x -= m_closeImage->width();
    driver->blitBitmap(bitmap, buttonPos, m_closeImage, m_closeImage->bounds(), yguipp::DM_COPY);
    buttonPos.m_x -= m_maximizeImage->width();
    driver->blitBitmap(bitmap, buttonPos, m_maximizeImage, m_maximizeImage->bounds(), yguipp::DM_COPY);
    buttonPos.m_x -= m_minimizeImage->width();
    driver->blitBitmap(bitmap, buttonPos, m_minimizeImage, m_minimizeImage->bounds(), yguipp::DM_COPY);
*/

    /* Window title */
    const std::string& title = window->getTitle();
    yguipp::Point textPos(
        (bitmap->width() - m_titleFont->getWidth(title)) / 2,
        (SIZE_HEADER - (m_titleFont->getAscent() + m_titleFont->getDescent())) / 2 + m_titleFont->getAscent()
    );

    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_set_scaled_font(cr, m_titleFont->getCairoFont());
    cairo_move_to(cr, textPos.m_x, textPos.m_y);
    cairo_show_text(cr, title.c_str());

    renderContext.finish();

    return true;
}

extern "C" Decorator* createDecorator(GuiServer* guiServer) {
    return new Black(guiServer);
}
