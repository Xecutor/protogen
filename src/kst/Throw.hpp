#ifndef __KST_THROW_HPP__
#define __KST_THROW_HPP__

#include <exception>
#include <kst/Format.hpp>

namespace kst {

class Exception : public std::exception {
public:
    explicit Exception(const char* argMsg) : msg(argMsg)
    {
    }

    ~Exception() override = default;

    const char* what() const noexcept override
    {
        return msg.c_str();
    }

protected:
    std::string msg;
};

}

#define KSTHROW(...) throw kst::Exception(FORMAT(__VA_ARGS__))


#endif
