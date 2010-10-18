#ifndef _YUTILPP_STRINGUTILS_HPP_
#define _YUTILPP_STRINGUTILS_HPP_

#include <string>
#include <vector>

namespace yutilpp {

class StringUtils {
  public:
    static bool toInt(const std::string& s, int& n);

    static void tokenize(const std::string& s, std::vector<std::string>& tokens, const std::string& delim);
}; /* class StringUtils */

} /* namespace yutilpp */

#endif /* _YUTILPP_STRINGUTILS_HPP_ */
