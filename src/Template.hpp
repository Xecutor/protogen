#ifndef __PROTOGEN_TEMPLATE_HPP__
#define __PROTOGEN_TEMPLATE_HPP__

#include <map>
#include <vector>
#include <string>
#include <ctype.h>
#include <exception>
#include <utility>
#include <memory>
#include "FileReader.hpp"

namespace protogen {

class CustomErrorException : public std::exception {
public:
    explicit CustomErrorException(std::string errorMsg) : msg(std::move(errorMsg))
    {
    }

    const char* what() const noexcept override
    {
        return msg.c_str();
    }

protected:
    std::string msg;

};

class CaseNotFoundException : public std::exception {
public:
    CaseNotFoundException(const std::string& varName, const std::string& caseValue)
    {
        msg = "Case '" + caseValue + "' not found for variable '" + varName + "'";
    }

    const char* what() const noexcept override
    {
        return msg.c_str();
    }

protected:
    std::string msg;
};

class UnknownCmd : public std::exception {
public:
    UnknownCmd(const std::string& cmd, const std::string& file, int line, int col)
    {
        msg = "Unknown template command '";
        msg += cmd;
        msg += "' at ";
        msg += file;
        char buf[32];
        sprintf(buf, ":%d:%d", line, col);
        msg += buf;
    }

    const char* what() const noexcept override
    {
        return msg.c_str();
    }

protected:
    std::string msg;
};

class BoolExprParsingExpr : public std::exception {
public:
    BoolExprParsingExpr(std::string argMsg, int argCol) : msg(std::move(argMsg)), col(argCol)
    {
    }

    const char* what() const noexcept override
    {
        return msg.c_str();
    }

    std::string msg;
    int col;
};

class TemplateParsingException : public std::exception {
public:
    TemplateParsingException(const std::string& txt, const std::string& file, int line, int col)
    {
        msg = txt;
        msg += " at ";
        msg += file;
        char buf[32];
        sprintf(buf, ":%d:%d", line, col);
        msg += buf;
    }

    const char* what() const noexcept override
    {
        return msg.c_str();
    }

protected:
    std::string msg;
};

class IFileFinder {
public:
    virtual ~IFileFinder() = default;

    virtual std::string findFile(const std::string& fileName) = 0;
};

class Template {
public:

    void assignFileFinder(IFileFinder* argFf)
    {
        ff = argFf;
    }

    void Parse(const std::string& fileName);

    void dump();

    template<class DataSource>
    std::string Generate(DataSource& ds)
    {
        int idx = 0;
        std::string rv;
        std::string::size_type packStart = 0;
        int packCnt = 0;
        try
        {
            for(; ops[idx].op != opEnd;)
            {
                //printf("%d,%d,%d\n",idx,ops[idx].line,ops[idx].col);fflush(stdout);
                switch(ops[idx].op)
                {
                    case opText:
                        rv += ops[idx].value;
                        break;
                    case opVar:
                    {
                        if(ops[idx].varFlag == varFlagNone)
                        {
                            rv += ds.getVar(ops[idx].value);
                        }
                        else if(ops[idx].varFlag == varFlagUcf)
                        {
                            std::string val = ds.getVar(ops[idx].value);
                            if(val.length())
                            {
                                val[0] = toupper(val[0]);
                            }
                            rv += val;
                        }
                        else if(ops[idx].varFlag == varFlagUc)
                        {
                            std::string val = ds.getVar(ops[idx].value);
                            for(std::string::size_type i = 0; i < val.length(); i++)
                            {
                                val[i] = toupper(val[i]);
                            }
                            rv += val;
                        }
                        else if(ops[idx].varFlag == varFlagHex)
                        {
                            int val = atoi(ds.getVar(ops[idx].value).c_str());
                            char buf[32];
                            sprintf(buf, "0x%x", val);
                            rv += buf;
                        }
                        break;
                    }
                    case opSetBool:
                    {
                        ds.setBool(ops[idx].value, ops[idx].boolSetValue);
                        break;
                    }
                    case opSetVar:
                    {
                        std::string& varName = ops[idx].value;
                        OpVector& varOps = ops[idx].varValue;
                        varOps.swap(ops);
                        try
                        {
                            ds.setVar(varName, Generate(ds));
                        }
                        catch(...)
                        {
                            varOps.swap(ops);
                            throw;
                        }
                        varOps.swap(ops);
                        break;
                    }
                    case opLoop:
                    {
                        if(ds.loopNext(ops[idx].value))
                        {
                            break;
                        }
                        idx = ops[idx].jidx;
                        continue;
                    }
                    case opIf:
                    {
                        if(ops[idx].boolValue.eval(ds))
                        {
                            break;
                        }
                        else
                        {
                            idx = ops[idx].jidx;
                            continue;
                        }
                    }
                    case opIfdef:
                    {
                        if(ds.haveVar(ops[idx].value))
                        {
                            break;
                        }
                        idx = ops[idx].jidx;
                        continue;
                    }
                    case opIfndef:
                    {
                        if(!ds.haveVar(ops[idx].value))
                        {
                            break;
                        }
                        idx = ops[idx].jidx;
                        continue;
                    }
                    case opJump:
                        idx = ops[idx].jidx;
                        continue;
                    case opSelect:
                    {
                        SelectMap::iterator it = ops[idx].smap.find(ds.getVar(ops[idx].value));
                        if(it == ops[idx].smap.end())
                        {
                            it = ops[idx].smap.find("");
                            if(it == ops[idx].smap.end())
                            {
                                throw CaseNotFoundException(ops[idx].value, ds.getVar(ops[idx].value));
                            }
                        }
                        idx = it->second;
                        continue;
                    }
                    case opPack:
                        if(packCnt == 0)
                        {
                            packStart = rv.length();
                        }
                        packCnt++;
                        //printf("pack:%d(start=%lu)\n",packCnt,packStart);
                        break;
                    case opPackEnd:
                    {
                        packCnt--;
                        //printf("packend:%d(start=%lu)\n",packCnt,packStart);
                        if(packCnt != 0)
                        {
                            break;
                        }
                        std::string::size_type i = packStart;
                        while(i < rv.length() && isspace(rv[i]))
                        {
                            i++;
                        }
                        rv.erase(packStart, i - packStart);
                        i = rv.length() - 1;
                        while(i > packStart && isspace(rv[i]))
                        {
                            i--;
                        }
                        if(!isspace(rv[i]))
                        {
                            i++;
                        }
                        if(i > packStart)
                        {
                            rv.erase(i);
                        }
                        for(i = packStart; i < rv.length(); i++)
                        {
                            if(rv[i] == 0x0a || rv[i] == 0x0d || rv[i] == 0x09)
                            {
                                rv[i] = ' ';
                            }
                        }
                        i = packStart;
                        while((i = rv.find(' ', i)) != std::string::npos)
                        {
                            while(i < rv.length() - 1 && rv[i + 1] == ' ')
                            {
                                rv.erase(i, 1);
                            }
                            i++;
                        }
                        break;
                    }
                    case opError:
                    {
                        throw CustomErrorException(ops[idx].value);
                    }
                    case opEnd:
                        continue;
                }
                idx++;
            }
        }
        catch(std::exception& e)
        {
            ds.dumpContext();
            std::string msg = "Exception during code generation:'";
            msg += e.what();
            msg += "'";
            throw TemplateParsingException(msg, files[ops[idx].fidx], ops[idx].line, ops[idx].col);
        }
        return rv;
    }

protected:
    enum OpCode {
        opText,
        opVar,
        opLoop,
        opIf,
        opIfdef,
        opIfndef,
        opJump,
        opSelect,
        opPack,
        opPackEnd,
        opSetBool,
        opSetVar,
        opError,
        opEnd
    };
    enum VarFlags {
        varFlagNone,
        varFlagUc,
        varFlagUcf,
        varFlagHex
    };
    typedef std::map<std::string, int> SelectMap;

    enum BoolOp {
        bopNone,
        bopVar,
        bopNotVar,
        bopNot,
        bopEqVal,
        bopNeqVal,
        bopEqVar,
        bopNeqVar,
        bopAnd,
        bopOr
    };

    struct BoolTree {
        BoolOp bop;
        std::string varName;
        std::string value;
        std::unique_ptr<BoolTree> left, right;

        BoolTree() : bop(bopNone)
        {
        }

        BoolTree(const BoolTree& other) : bop(other.bop), varName(other.varName), value(other.value)
        {
            if(other.left)
            {
                left = std::make_unique<BoolTree>(*other.left);
            }
            if(other.right)
            {
                right = std::make_unique<BoolTree>(*other.right);
            }
        }

        BoolTree(BoolTree&&) = default;

        template<class DataSource>
        bool eval(DataSource& ds)
        {
            switch(bop)
            {
                case bopAnd:
                    return left->eval(ds) && right->eval(ds);
                case bopOr:
                    return left->eval(ds) || right->eval(ds);
                case bopVar:
                    return ds.getBool(varName);
                case bopNotVar:
                    return !ds.getBool(varName);
                case bopEqVal:
                    //fprintf(stderr,"eqval:%s==%s\n",varName.c_str(),value.c_str());
                    return ds.getVar(varName) == value;
                case bopNeqVal:
                    return ds.getVar(varName) != value;
                case bopEqVar:
                    //fprintf(stderr,"eqvar:%s==%s\n",varName.c_str(),value.c_str());
                    return ds.getVar(varName) == ds.getVar(value);
                case bopNeqVar:
                    return ds.getVar(varName) != ds.getVar(value);
                case bopNot:
                    return !left->eval(ds);
                default:
                    throw std::runtime_error("invalid bool op!");
            }
        }
    };

    struct Op;
    typedef std::vector<Op> OpVector;

    struct Op {

        OpCode op = opEnd;
        std::string value;
        OpVector varValue;
        BoolTree boolValue;
        int jidx = -1;
        int line = 0;
        int col = 0;
        int fidx = 0;
        VarFlags varFlag = varFlagNone;
        bool boolSetValue = false;
        SelectMap smap;
    };

    OpVector ops;
    IFileFinder* ff = nullptr;
    StrVector files;
    struct MacroInfo {
        std::string macroName;
        std::string macroText;
    };
    typedef std::map<std::string, MacroInfo> MacroMap;
    MacroMap macroMap;

    void Parse(FileReader& fr);

    std::string
    expandMacro(const MacroInfo& mi, const std::vector<std::string>& args, const std::string& fileName, int line,
                int col);

    std::string::size_type parseBool(const std::string& expr, BoolTree& bt, std::string::size_type pos = 0, int prio = 0);

};
}

#endif

