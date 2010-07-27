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

yguipp::Point Black::leftTop(void) {
    return yguipp::Point(0,SIZE_HEADER);
}

yguipp::Point Black::getSize(void) {
    return yguipp::Point(0,SIZE_HEADER);
}

DecoratorData* Black::createWindowData(void) {
    return 0;
}

int Black::update(GraphicsDriver* driver, Window* window) {
    Bitmap* bitmap = window->getBitmap();

    for (int i = 0; i < SIZE_HEADER; i++) {
        driver->fillRect(bitmap, bitmap->bounds(), yguipp::Rect(0, i, bitmap->width() - 1, i), m_headerColors[i], yguipp::DM_COPY);
    }

    return 0;
}

extern "C" Decorator* createDecorator(void) {
    return new Black();
}
