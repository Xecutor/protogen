#include "Utility.hpp"

namespace protogen {

StrVector splitString(const std::string& str, const std::string& div)
{
    StrVector rv;
    std::string::size_type pos, lastPos = 0;
    while((pos = str.find(div, lastPos)) != std::string::npos)
    {
        rv.push_back(str.substr(lastPos, pos - lastPos));
        lastPos = pos + div.length();
    }
    rv.push_back(str.substr(lastPos));
    return rv;
}

}