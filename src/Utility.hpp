#pragma once

#include <string>
#include <vector>

namespace protogen {

using StrVector = std::vector<std::string>;

StrVector splitString(const std::string& str, const std::string& div);

}