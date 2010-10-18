#ifndef _YGUICTRL_SCREENMODE_HPP_
#define _YGUICTRL_SCREENMODE_HPP_

#include "yguicontrol.hpp"

class ScreenMode : public CommandHandler {
  public:
    int handleCommand(int argc, char** argv);

  private:
    int list(void);
    int help(void);
    int set(int argc, char** argv);
}; /* class ScreenMode */

#endif /* _YGUICTRL_SCREENMODE_HPP_ */
