#ifndef _YGUICTRL_HPP_
#define _YGUICTRL_HPP_

#include <map>
#include <string>

class CommandHandler {
  public:
    virtual int handleCommand(int argc, char** argv) = 0;
}; /* class CommandHandler */

class YGuiControl {
  public:
    YGuiControl(void);
    ~YGuiControl(void);

    int run(int argc, char** argv);

  private:
    std::map<std::string, CommandHandler*> m_handlers;
}; /* class YGuiControl */

#endif /* _YGUICTRL_HPP_ */
