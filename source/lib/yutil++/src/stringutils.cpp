#include <sstream>

#include <yutil++/stringutils.hpp>

namespace yutilpp {

bool StringUtils::toInt(const std::string& s, int& n) {
    std::stringstream ss;
    ss << s;
    ss >> n;

    return !ss.fail();
}

void StringUtils::tokenize(const std::string& s, std::vector<std::string>& tokens, const std::string& delim) {
    std::string::size_type i;
    std::string::size_type j = 0;
    std::string::size_type l = delim.size();

    while ((i = s.find(delim, j)) != std::string::npos) {
        tokens.push_back(s.substr(j, i - j));
        j = i + l;
    }

    if (j < s.size()) {
        tokens.push_back(s.substr(j, s.size() - j));
    }
}

} /* namespace yutilpp */
