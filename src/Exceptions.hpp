#ifndef __PROTOGEN_EXCEPTIONS_HPP__
#define __PROTOGEN_EXCEPTIONS_HPP__

#include <exception>
#include <string>
#include <stdio.h>
#include <string.h>

class BaseException : public std::exception {
public:
    BaseException() = default;

    explicit BaseException(std::string msg) : txt(std::move(msg))
    {
    }

    const char* what() const noexcept override
    {
        return txt.c_str();
    }

protected:
    std::string txt;
};

class ParsingException : public BaseException {
public:
    ParsingException(std::string msg, const std::string& file, int line, int col)
    {
        char buf[32];
        txt = std::move(msg);
        if(file.length())
        {
            txt += ' ';
            txt += file;
            sprintf(buf, ":%d:%d", line, col);
            txt += buf;
        }
    }
};

class DuplicateItemException : public ParsingException {
public:
    DuplicateItemException(const std::string& what, const std::string& name, const std::string& file, int line, int col)
            :
            ParsingException("Duplicate definition of " + what + " " + name, file, line, col)
    {
    }
};

class NotFoundException : public ParsingException {
public:
    NotFoundException(const std::string& what, const std::string& name, const std::string& file, int line, int col) :
            ParsingException(what + " " + name + " not found", file, line, col)
    {
    }
};

class MessageNotFoundException : public NotFoundException {
public:
    MessageNotFoundException(const std::string& name, const std::string& file, int line, int col) :
            NotFoundException("Message", name, file, line, col)
    {
    }
};

class MessageOrTypeNotFoundException : public NotFoundException {
public:
    MessageOrTypeNotFoundException(const std::string& name, const std::string& file, int line, int col) :
            NotFoundException("Message or type", name, file, line, col)
    {
    }
};


class ProtocolNotFoundException : public NotFoundException {
public:
    ProtocolNotFoundException(const std::string& name, const std::string& file, int line, int col) :
            NotFoundException("Protocol", name, file, line, col)
    {
    }
};

class TypeNotFoundException : public NotFoundException {
public:
    TypeNotFoundException(const std::string& name, const std::string& file, int line, int col) :
            NotFoundException("Type", name, file, line, col)
    {
    }
};

class FieldSetNotFoundException : public NotFoundException {
public:
    FieldSetNotFoundException(const std::string& name, const std::string& file, int line, int col) :
            NotFoundException("FieldSet", name, file, line, col)
    {
    }
};

class PropertyNotFoundException : public NotFoundException {
public:
    PropertyNotFoundException(const std::string& name, const std::string& file, int line, int col) :
            NotFoundException("Property", name, file, line, col)
    {
    }
};

#endif
