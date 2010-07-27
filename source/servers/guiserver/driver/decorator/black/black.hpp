#ifndef _DECORATOR_BLACK_HPP_
#define _DECORATOR_BLACK_HPP_

#include <ygui++/color.hpp>

#include <guiserver/window.hpp>
#include <guiserver/decorator.hpp>

class Black : public Decorator {
  public:
    yguipp::Point leftTop(void);
    yguipp::Point getSize(void);

    DecoratorData* createWindowData(void);

    int update(GraphicsDriver* driver, Window* window);

  private:
    enum {
        SIZE_HEADER = 23
    };

    static yguipp::Color m_headerColors[SIZE_HEADER];
}; /* class Black */

#endif /* _DECORATOR_BLACK_HPP_ */
