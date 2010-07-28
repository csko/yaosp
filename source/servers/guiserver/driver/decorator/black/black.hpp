#ifndef _DECORATOR_BLACK_HPP_
#define _DECORATOR_BLACK_HPP_

#include <ygui++/color.hpp>

#include <guiserver/window.hpp>
#include <guiserver/decorator.hpp>
#include <guiserver/guiserver.hpp>

class Black : public Decorator {
  public:
    Black(GuiServer* guiServer);
    virtual ~Black(void);

    yguipp::Point leftTop(void);
    yguipp::Point getSize(void);

    bool calculateItemPositions(Window* window);

    bool update(GraphicsDriver* driver, Window* window);

  private:
    enum {
        SIZE_HEADER = 23
    };

    Bitmap* m_minimizeImage;
    Bitmap* m_maximizeImage;
    Bitmap* m_closeImage;

    FontNode* m_titleFont;

    static yguipp::Color m_headerColors[SIZE_HEADER];

    static uint8_t m_minimizeButton[23 * 23 * 4];
    static uint8_t m_maximizeButton[23 * 23 * 4];
    static uint8_t m_closeButton[23 * 23 * 4];
}; /* class Black */

#endif /* _DECORATOR_BLACK_HPP_ */
