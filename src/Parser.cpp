#include "Parser.hpp"
#include <stdio.h>
#include <stdexcept>
#include <ctype.h>
#include <stdlib.h>

namespace protogen {

static bool fileOrIdentChar(char c)
{
    return isalnum(c) || c == '_' || c == '.' || c == '/';
}

std::string TokenTypeToString(TokenType tt)
{
    switch(tt)
    {
        case ttIdent:
            return "identifier";
        case ttInclude:
            return "include";
        case ttMessage:
            return "message";
        case ttFieldSet:
            return "fieldset";
        case ttEnd:
            return "end";
        case ttVersion:
            return "version";
        case ttVersionValue:
            return "version value";
        case ttType:
            return "type";
        case ttProperty:
            return "property";
        case ttBool:
            return "bool";
        case ttInt:
            return "int";
        case ttString:
            return "string";
        case ttIntValue:
            return "int value";
        case ttHexValue:
            return "hex value";
        case ttStringValue:
            return "string value";
        case ttDefault:
            return "default";
        case ttTrue:
            return "true";
        case ttFalse:
            return "false";
        case ttEqual:
            return "equal";
        case ttColon:
            return "colon";
        case ttProtocol:
            return "protocol";
        case ttEnum:
            return "enum";
        case ttEoln:
            return "end of line";
        case ttEof:
            return "end of file";
        case ttPackage:
            return "package";
        case ttFileName:
            return "file name";
    }
    return "unknown";
}

static std::map<std::string, TokenType> kwMap = {
        {"message",  ttMessage},
        {"fieldset", ttFieldSet},
        {"end",      ttEnd},
        {"include",  ttInclude},
        {"protocol", ttProtocol},
        {"version",  ttVersion},
        {"type",     ttType},
        {"property", ttProperty},
        {"_bool",    ttBool},
        {"_int",     ttInt},
        {"_string",  ttString},
        {"default",  ttDefault},
        {"true",     ttTrue},
        {"false",    ttFalse},
        {"enum",     ttEnum},
        {"package",  ttPackage},
};;


TokenType findKeyword(const std::string& str)
{
    std::map<std::string, TokenType>::const_iterator it = kwMap.find(str);
    if(it == kwMap.end())
    {
        return ttIdent;
    }
    return it->second;
}

const Parser::Token& Parser::expect(TokensList::iterator& it, const TokenTypeList& ttl)
{
    it++;
    const TokenTypeList* node = &ttl;
    while(node && it != tokens.end())
    {
        if(it->tt == node->tt)
        {
            return *it;
        }
        node = node->prev;
    }
    node = &ttl;
    std::string msg;
    while(node)
    {
        msg += TokenTypeToString(node->tt);
        node = node->prev;
        if(node)
        {
            msg += ',';
        }
    }

    throw ParsingException("Expected '" + msg + "' found '" + TokenTypeToString(it->tt) + "' at", files[it->file],
            it->line, it->col);
}


void Parser::parseFile(const char* fileName)
{
    std::string foundFile = FileReader::findFile(searchPath, fileName, searchInCurDir);
    for(auto& file : files)
    {
        if(foundFile == file)
        {
            return;
        }
    }
    //printf("parsing:%s\n",foundFile.c_str());

    FileReader fr;
    fr.Open(foundFile);
    fr.file = files.size();
    files.push_back(foundFile);
    char c;
    bool lineComment = false;
    bool blockComment = false;
    int bcline = 0, bccol = 0;
    int line = 0, col = 0;
    while(!fr.eof())
    {
        line = fr.line;
        col = fr.col;

        if(lineComment)
        {
            if(fr.getChar() != 0x0a)
            {
                continue;
            }
            pushToken(Token(ttEoln, fr.file, line, col));
            lineComment = false;
            continue;
        }
        if(blockComment)
        {
            if(fr.getChar() != '*')
            {
                continue;
            }
            fr.putChar();
        }

        switch(c = fr.getChar())
        {
            case '/':
            {
                if(fr.eof())
                {
                    throw std::runtime_error("Unexpected symbol '/' at the end of file " + foundFile);
                }
                int col2 = fr.col;
                c = fr.getChar();
                if(c == '/')
                {
                    lineComment = true;
                    break;
                }
                if(c == '*')
                {
                    blockComment = true;
                    bcline = line;
                    bccol = col;
                    break;
                }
                throw ParsingException("Unexpected symbol at ", foundFile, line, col2);
            }
            case '*':
            {
                if(!blockComment)
                {
                    throw ParsingException("Unexpected symbol '*' at ", foundFile, line, col);
                }
                if(fr.eof())
                {
                    throw ParsingException("Unexpected end of file ", foundFile, line, col);
                }
                c = fr.getChar();
                if(c != '/')
                {
                    throw ParsingException("Unexpected symbol at ", foundFile, line, col);
                }
                blockComment = false;
            }
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            {
                TokenType tt = ttIntValue;
                std::string val;
                val = c;
                while(!fr.eof() && isdigit(c = fr.getChar()))
                {
                    val += c;
                }
                if(c == '.')
                {
                    tt = ttVersionValue;
                    val += c;
                    bool ok = false;
                    while(!fr.eof() && isdigit(c = fr.getChar()))
                    {
                        val += c;
                        ok = true;
                    }
                    if(!ok)
                    {
                        throw ParsingException("Invalid version field at ", foundFile, line, col);
                    }
                }
                else if(c == 'x' || c == 'X')
                {
                    tt = ttHexValue;
                    val += c;
                    bool ok = false;
                    while(!fr.eof() && (isdigit(c = fr.getChar()) || strchr("abcdefABCDEF", c)))
                    {
                        val += c;
                        ok = true;
                    }
                    if(!ok)
                    {
                        throw ParsingException("Broken hex value at ", foundFile, line, col);
                    }
                }
                pushToken(Token(tt, fr.file, line, col, val));

                if(c == 0x0a)
                {
                    pushToken(Token(ttEoln, fr.file, fr.line, fr.col));
                }
                else
                {
                    fr.putChar();
                }
            }
                break;
            case '`':
            {
                std::string val;
                while(!fr.eof() && (c = fr.getChar()) != '`')
                {
                    val += c;
                }
                if(c != '`')
                {
                    throw ParsingException("Raw identifier wasn't closed at ", foundFile, line, col);
                }
                pushToken(Token(ttIdent, fr.file, line, col, val));
            }
                break;
            case '"':
            {
                std::string val;
                bool escape = false;
                while(!fr.eof() && ((c = fr.getChar()) != '"' || escape))
                {
                    if(c != '\\' || escape)
                    {
                        val += c;
                        escape = false;
                    }
                    else
                    {
                        escape = true;
                    }
                }
                pushToken(Token(ttStringValue, fr.file, line, col, val));
            }
                break;
            case 0x0a:
            {
                pushToken(Token(ttEoln, fr.file, line, col));
            }
                break;
            case '=':
            {
                pushToken(Token(ttEqual, fr.file, line, col));
            }
                break;
            case ':':
            {
                pushToken(Token(ttColon, fr.file, line, col));
            }
                break;
            default:
            {
                if(isspace(c))
                {
                    continue;
                }
                if(!fileOrIdentChar(c))
                {
                    throw ParsingException("Unexpected symbol at ", foundFile, line, col);
                }
                std::string val;
                val = c;
                TokenType tt = ttIdent;
                while((!fr.eof()) && fileOrIdentChar(c = fr.getChar()))
                {
                    val += c;
                    if(c == '.' || c == '/')
                    {
                        tt = ttFileName;
                    }
                }
                if(!fr.eof() && !isspace(c) && c != '=' && c != ':')
                {
                    throw ParsingException("Unexpected symbol at ", foundFile, line, fr.col - 1);
                }
                if(tt == ttIdent)
                {
                    tt = findKeyword(val);
                }

                pushToken(Token(tt, fr.file, line, col, val));
                if(c == 0x0a)
                {
                    pushToken(Token(ttEoln, fr.file, fr.line, fr.col));
                }
                if(c == '=')
                {
                    pushToken(Token(ttEqual, fr.file, line, col));
                }
                if(c == ':')
                {
                    pushToken(Token(ttColon, fr.file, line, col));
                }
            }
                break;
        }
    }
    pushToken(Token(ttEof, fr.file, line, col));
    if(blockComment)
    {
        throw ParsingException("Block comment wasn't closed at ", foundFile, bcline, bccol);
    }
    if(recursive)
    {
        return;
    }
    recursive++;
    enum CurrentContext {
        ccGlobal,
        ccMessage,
        ccFieldSet,
        ccProtocol,
        ccProperty,
        ccEnum
    };
    CurrentContext cc = ccGlobal;
    //int ccLine, ccCol;
    bool fsProp = false;
    bool msgHaveVersion = false;
    Message curMessage;
    std::set<std::string> curMsgFields;
    std::set<int> curMsgTags, msgTags;
    FieldSet curFieldSet;
    Protocol curProto;
    Property curProperty;
    Enum curEnum;
    bool enumHaveType = false;
    int tag = 0;
    bool haveTag = false;

    StrVector pkgStack;

    std::string pkg;

    for(auto it = tokens.begin(); it != tokens.end(); it++)
    {
        //printf(">%s\n",TokenTypeToString(it->tt).c_str());
        switch(it->tt)
        {
            case ttPackage:
            {
                if(cc != ccGlobal)
                {
                    unexpected(it);
                }
                pkg = expect(it, (TokenTypeList(ttIdent), ttFileName)).value;
            }
                break;
            case ttInclude:
            {
                if(cc != ccGlobal)
                {
                    unexpected(it);
                }
                pkgStack.push_back(pkg);
                pkg = "";
                std::string file = expect(it, (TokenTypeList(ttIdent), ttFileName)).value;
                parsePos = it;
                parsePos++;
                parseFile(file.c_str());
            }
                break;
            case ttType:
            {
                if(cc != ccGlobal)
                {
                    unexpected(it);
                }
                FieldType ft;
                Token t = expect(it, ttIdent);
                ft.typeName = t.value;
                ft.fk = fkType;
                while(expect(it, (TokenTypeList(ttIdent), ttEoln)).tt != ttEoln)
                {
                    Property p;
                    p.name = it->value;
                    auto pit = properties.find(p.name);

                    if(pit == properties.end())
                    {
                        throw PropertyNotFoundException(it->value, files[it->file], it->line, it->col);
                    }

                    p = pit->second;
                    expect(it, (TokenTypeList(ttIdent), ttEqual, ttEoln));
                    if(it->tt == ttEqual)
                    {
                        PropertyField pf;
                        fillPropertyField(it, p, pf);
                        p.fields.push_back(pf);
                    }
                    else
                    {
                        --it;
                    }
                    ft.properties.push_back(p);
                };
                types.insert(FieldTypeMap::value_type(ft.typeName, ft));
            }
                break;
            case ttProperty:
            {
                if(cc != ccGlobal && cc != ccMessage && cc != ccFieldSet && cc != ccEnum)
                {
                    unexpected(it);
                }
                if(cc == ccGlobal || cc == ccFieldSet)
                {
                    fsProp = cc == ccFieldSet;
                    curProperty.clear();
                    cc = ccProperty;
                    curProperty.name = expect(it, ttIdent).value;
                    expect(it, (TokenTypeList(ttBool), ttInt, ttString));
                    if(it->tt == ttBool)
                    {
                        curProperty.pt = ptBool;
                    }
                    else if(it->tt == ttInt)
                    {
                        curProperty.pt = ptInt;
                    }
                    else
                    {
                        curProperty.pt = ptString;
                    }
                }
                else //if(cc==ccMessage || cc==ccEnum)
                {
                    Property p;
                    p.name = expect(it, ttIdent).value;
                    auto pit = properties.find(p.name);

                    if(pit == properties.end())
                    {
                        throw PropertyNotFoundException(it->value, files[it->file], it->line, it->col);
                    }

                    p = pit->second;

                    expect(it, (TokenTypeList(ttEqual), ttEoln));
                    if(it->tt == ttEqual)
                    {
                        PropertyField pf;
                        //expect(it,(TokenTypeList(ttIntValue),ttStringValue,ttTrue,ttFalse,ttHexValue));
                        fillPropertyField(it, p, pf);
                        p.fields.push_back(pf);
                    }
                    if(cc == ccMessage)
                    {
                        curMessage.properties.push_back(p);
                    }
                    else
                    {
                        curEnum.properties.push_back(p);
                    }
                }
            }
                break;
            case ttMessage:
            {
                if(cc != ccGlobal)
                {
                    unexpected(it);
                }
                //ccLine = it->line;
                //ccCol = it->col;
                curMessage.clear();
                curMsgFields.clear();
                curMsgTags.clear();
                curMessage.name = expect(it, (ttIdent)).value;
                curMessage.pkg = pkg;
                if(messages.find(curMessage.name) != messages.end())
                {
                    throw DuplicateItemException("message", curMessage.name, files[it->file], it->line, it->col);
                }
                expect(it, (TokenTypeList(ttIntValue), ttHexValue, ttColon, ttEoln));
                if(it->tt == ttColon)
                {
                    expect(it, ttIdent);
                    getMessage(it->value);
                    curMessage.parent = it->value;
                    expect(it, (TokenTypeList(ttIntValue), ttHexValue, ttEoln));
                }

                if(it->tt == ttIntValue || it->tt == ttHexValue)
                {
                    curMessage.haveTag = true;
                    curMessage.tag = it->asInt();
                    if(msgTags.find(curMessage.tag) != msgTags.end())
                    {
                        throw DuplicateItemException("message tag", curMessage.name, files[it->file], it->line,
                                it->col);
                    }
                }
                cc = ccMessage;
                msgHaveVersion = false;
            }
                break;
            case ttFieldSet:
            {
                if(cc != ccGlobal)
                {
                    unexpected(it);
                }
                curFieldSet.clear();
                curMsgFields.clear();
                curMsgTags.clear();
                curFieldSet.name = expect(it, ttIdent).value;
                curFieldSet.pkg = pkg;
                if(fsNames.find(curFieldSet.name) != fsNames.end())
                {
                    throw DuplicateItemException("fieldset", curFieldSet.name, files[it->file], it->line, it->col);
                }
                cc = ccFieldSet;
            }
                break;
            case ttEnum:
            {
                if(cc != ccGlobal)
                {
                    unexpected(it);
                }
                curEnum.clear();
                curEnum.typeName = expect(it, (ttIdent)).value;
                if(types.find(curEnum.typeName) == types.end())
                {
                    throw MessageOrTypeNotFoundException(it->value, files[it->file], it->line, it->col);
                }
                curEnum.name = expect(it, (ttIdent)).value;
                curEnum.pkg = pkg;
                if(enumMap.find(curEnum.name) != enumMap.end())
                {
                    throw DuplicateItemException("enum", curEnum.name, files[it->file], it->line, it->col);
                }
                cc = ccEnum;
                enumHaveType = false;
            }
                break;
            case ttEnd:
            {
                if(cc == ccMessage)
                {
                    //printf("finished parsing message:%s\n",curMessage.name.c_str());
                    if(requireVersion && !msgHaveVersion)
                    {
                        throw BaseException("Message " + curMessage.name + " do not have version defined");
                    }
                    messages.insert(MessageMap::value_type(curMessage.name, curMessage));
                }
                else if(cc == ccProtocol)
                {
                    protocols.insert(ProtocolsMap::value_type(curProto.name, curProto));
                }
                else if(cc == ccProperty)
                {
                    if(fsProp)
                    {
                        curFieldSet.properties.insert(PropertyMap::value_type(curProperty.name, curProperty));
                    }
                    else
                    {
                        properties.insert(PropertyMap::value_type(curProperty.name, curProperty));
                    }
                }
                else if(cc == ccEnum)
                {
                    enumMap.insert(EnumMap::value_type(curEnum.name, curEnum));
                }
                else if(cc == ccFieldSet)
                {
                    fieldsSets.push_back(curFieldSet);
                    fieldsSets.back().finish();
                }
                else
                {
                    unexpected(it);
                }
                if(fsProp)
                {
                    cc = ccFieldSet;
                    fsProp = false;
                }
                else
                {
                    cc = ccGlobal;
                }
            }
                break;
            case ttVersion:
            {
                if(cc != ccMessage && cc != ccProtocol)
                {
                    unexpected(it);
                }
                auto t = expect(it, (ttVersionValue));
                const char* ptr = t.value.c_str();
                char* end = nullptr;
                int vmajor = strtol(ptr, &end, 10);
                if(!end || ptr == end || *end == '\0')
                {
                    throw ParsingException("Failed to parse version", files[t.file], t.line, t.col);
                }
                int vminor = strtol(end + 1, &end, 10);
                if(!end || ptr == end || *end == '\0')
                {
                    throw ParsingException("Failed to parse version", files[t.file], t.line, t.col);
                }
                if(vmajor<0 || vmajor>255 || vminor<0 || vminor>255)
                {
                    throw ParsingException("Version value is out of range", files[t.file], t.line, t.col);
                }
                curMessage.majorVersion = static_cast<uint16_t>(vmajor);
                curMessage.minorVersion = static_cast<uint16_t>(vminor);
                msgHaveVersion = true;
            }
                break;
            case ttFileName:
            case ttIdent:
            {
                if(cc == ccGlobal)
                {
                    unexpected(it);
                }
                if(cc == ccMessage || cc == ccFieldSet)
                {
                    Field f;
                    auto ftIt = types.find(it->value);
                    bool fsField = false;
                    if(ftIt != types.end())
                    {
                        f.ft = ftIt->second;
                        f.ft.fk = fkType;
                    }
                    else if(messages.find(it->value) != messages.end())
                    {
                        f.ft.fk = fkNested;
                        f.ft.typeName = it->value;
                    }
                    else if(enumMap.find(it->value) != enumMap.end())
                    {
                        f.ft.fk = fkEnum;
                        f.ft.typeName = it->value;
                    }
                    else
                    {
                        if(it->tt == ttFileName)
                        {
                            std::string::size_type pos = it->value.find('.');
                            if(pos == std::string::npos)
                            {
                                unexpected(it);
                            }
                            std::string fsname = it->value.substr(0, pos);
                            std::string fldname = it->value.substr(pos + 1);
                            if(fldname.find('.') != std::string::npos)
                            {
                                unexpected(it);
                            }
                            bool found = false;
                            for(auto& fieldsSet : fieldsSets)
                            {
                                if(fieldsSet.name == fsname)
                                {
                                    auto fld = fieldsSet.fieldsMap.find(fldname);
                                    if(fld != fieldsSet.fieldsMap.end())
                                    {
                                        fieldsSet.used = true;
                                        f = *fld->second;
                                        found = true;
                                        break;
                                    }
                                    else
                                    {
                                        throw NotFoundException("field", it->value, files[it->file], it->line, it->col);
                                    }
                                }
                            }
                            if(!found)
                            {
                                throw NotFoundException("fieldset", it->value, files[it->file], it->line, it->col);
                            }
                        }
                        else
                        {
                            bool found = false;
                            for(auto& fieldsSet : fieldsSets)
                            {
                                auto fld = fieldsSet.fieldsMap.find(it->value);
                                if(fld != fieldsSet.fieldsMap.end())
                                {
                                    fieldsSet.used = true;
                                    f = *fld->second;
                                    found = true;
                                    break;
                                }
                            }
                            if(!found)
                            {
                                throw MessageOrTypeNotFoundException(it->value, files[it->file], it->line, it->col);
                            }
                        }
                        if(haveTag || (cc == ccFieldSet && it->tt != ttFileName))
                        {
                            unexpected(it);
                        }
                        fsField = true;
                    }
                    if(!fsField)
                    {
                        f.name = expect(it, ttIdent).value;
                    }
                    if(cc == ccFieldSet)
                    {
                        if(curFieldSet.fieldsMap.find(f.name) != curFieldSet.fieldsMap.end())
                        {
                            throw DuplicateItemException("field", f.name, files[it->file], it->line, it->col);
                        }
                    }
                    else
                    {
                        if(curMsgFields.find(f.name) != curMsgFields.end())
                        {
                            throw DuplicateItemException("field", f.name, files[it->file], it->line, it->col);
                        }
                    }
                    if(!fsField)
                    {
                        if(haveTag)
                        {
                            f.tag = tag;
                            haveTag = false;
                        }
                        else
                        {
                            f.tag = expect(it, (TokenTypeList(ttIntValue), ttHexValue)).asInt();
                        }
                    }
                    if(curMsgTags.find(f.tag) != curMsgTags.end())
                    {
                        throw DuplicateItemException("field tag", f.name, files[it->file], it->line, it->col);
                    }
                    curMsgFields.insert(f.name);
                    curMsgTags.insert(f.tag);
                    do
                    {
                        expect(it, (TokenTypeList(ttIdent), ttEoln));
                        while(it->tt == ttIdent)
                        {
                            auto pit = properties.find(it->value);
                            if(pit == properties.end())
                            {
                                if(cc == ccFieldSet)
                                {
                                    pit = curFieldSet.properties.find(it->value);
                                }
                                if(pit == properties.end())
                                {
                                    throw PropertyNotFoundException(it->value, files[it->file], it->line, it->col);
                                }
                            }
                            Property p = pit->second;
                            expect(it, (TokenTypeList(ttIdent), ttEqual, ttEoln));
                            if(it->tt == ttEqual)
                            {
                                PropertyField pf;
                                pf.name = p.name;
                                pf.pt = p.pt;
                                if(p.pt == ptBool)
                                {
                                    pf.boolValue = expect(it, (TokenTypeList(ttTrue), ttFalse)).asBool();
                                }
                                else if(p.pt == ptInt)
                                {
                                    pf.intValue = expect(it, (TokenTypeList(ttIntValue), ttHexValue)).asInt();
                                }
                                else//ptString
                                {
                                    expect(it, ttStringValue);
                                    pf.strValue = it->value;
                                }
                                p.fields.push_back(pf);
                            }
                            f.properties.push_back(p);
                        }
                    } while(it->tt != ttEoln);
                    if(cc == ccMessage)
                    {
                        curMessage.fields.push_back(f);
                    }
                    else//ccFieldSet
                    {
                        f.fsname = curFieldSet.name;
                        curFieldSet.fields.push_back(f);
                    }
                }
                else if(cc == ccProtocol)
                {
                    Protocol::MessageRecord mr;
                    mr.msgName = it->value;
                    getMessage(mr.msgName);
                    //mr.tag=expect(it,(TokenTypeList(ttIntValue),ttHexValue)).asInt();
                    do
                    {
                        expect(it, (TokenTypeList(ttEoln), ttIdent));
                        while(it->tt == ttIdent)
                        {
                            Property p;
                            p.name = it->value;
                            auto pit = properties.find(p.name);

                            if(pit == properties.end())
                            {
                                throw PropertyNotFoundException(it->value, files[it->file], it->line, it->col);
                            }

                            p = pit->second;
                            expect(it, (TokenTypeList(ttEqual), ttIdent, ttEoln));
                            if(it->tt == ttEqual)
                            {
                                PropertyField pf;
                                pf.name = p.name;
                                pf.pt = p.pt;
                                if(p.pt == ptBool)
                                {
                                    pf.boolValue = expect(it, (TokenTypeList(ttTrue), ttFalse)).asBool();
                                }
                                else if(p.pt == ptInt)
                                {
                                    pf.intValue = expect(it, (TokenTypeList(ttIntValue), ttHexValue)).asInt();
                                }
                                else//ptString
                                {
                                    expect(it, ttStringValue);
                                    pf.strValue = it->value;
                                }
                                p.fields.push_back(pf);
                            }
                            mr.props.push_back(p);
                        }
                    } while(it->tt != ttEoln);

                    curProto.messages.push_back(mr);
                }
                else if(cc == ccProperty)
                {
                    PropertyField pf;
                    pf.name = it->value;
                    expect(it, (TokenTypeList(ttTrue), ttFalse, ttStringValue, ttIntValue, ttHexValue));
                    if(it->tt == ttTrue || it->tt == ttFalse)
                    {
                        pf.boolValue = it->asBool();
                        pf.pt = ptBool;
                    }
                    else if(it->tt == ttInt || it->tt == ttHexValue)
                    {
                        pf.intValue = it->asInt();
                        pf.pt = ptInt;
                    }
                    else//ptString
                    {
                        pf.strValue = it->value;
                        pf.pt = ptString;
                    }
                    curProperty.fields.push_back(pf);
                }
                else if(cc == ccEnum)
                {
                    Enum::EnumValue ev;
                    ev.name = it->value;
                    expect(it, (TokenTypeList(ttStringValue), ttIntValue, ttHexValue));
                    if(it->tt == ttStringValue)
                    {
                        ev.strVal = it->value;
                        if(enumHaveType)
                        {
                            if(curEnum.vt != Enum::vtString)
                            {
                                unexpected(it);
                            }
                        }
                        else
                        {
                            curEnum.vt = Enum::vtString;
                        }
                    }
                    else
                    {
                        ev.intVal = it->asInt();
                        if(enumHaveType)
                        {
                            if(curEnum.vt != Enum::vtInt)
                            {
                                unexpected(it);
                            }
                        }
                        else
                        {
                            curEnum.vt = Enum::vtInt;
                        }
                    }
                    curEnum.values.push_back(ev);
                }
            }
                break;
            case ttProtocol:
            {
                if(cc != ccGlobal)
                {
                    unexpected(it);
                }
                curProto.clear();
                curProto.name = expect(it, ttIdent).value;
                curProto.pkg = pkg;
                if(protocols.find(curProto.name) != protocols.end())
                {
                    throw DuplicateItemException("protocol", curProto.name, files[it->file], it->line, it->col);
                }
                cc = ccProtocol;
            }
                break;
            case ttDefault:
            {
                if(cc != ccProperty)
                {
                    unexpected(it);
                }
                curProperty.def = pdDefaultForField;
                expect(it, (TokenTypeList(ttMessage), ttEnum, ttType, ttEoln));
                if(it->tt != ttEoln && fsProp)
                {
                    throw ParsingException("Unexpected message/enum property in fieldset " + curFieldSet.name,
                            files[it->file], it->line, it->col);
                }
                if(it->tt == ttMessage)
                {
                    curProperty.def = pdDefaultForMessage;
                }
                else if(it->tt == ttEnum)
                {
                    curProperty.def = pdDefaultForEnum;
                }
                else if(it->tt == ttType)
                {
                    curProperty.def = pdDefaultForType;
                }
            }
                break;
            case ttEof:
            {
                if(cc != ccGlobal)
                {
                    if(cc == ccProtocol)
                    {
                        throw BaseException(
                                "Protocol " + curProto.name + " wasn't ended at end of file " + files[it->file]);
                    }
                    if(cc == ccMessage)
                    {
                        throw BaseException(
                                "Message " + curMessage.name + " wasn't ended at end of file" + files[it->file]);
                    }
                    if(cc == ccProperty)
                    {
                        throw BaseException(
                                "Property " + curMessage.name + " wasn't ended at end of file" + files[it->file]);
                    }
                }
                if(!pkgStack.empty())
                {
                    pkg = pkgStack.back();
                    pkgStack.pop_back();
                }
            }
                break;
            case ttEoln:
                break;
            case ttIntValue:
            case ttHexValue:
            {
                if(cc != ccMessage && cc != ccFieldSet)
                {
                    unexpected(it);
                }
                tag = it->asInt();
                haveTag = true;
            }
                break;
            default:
                unexpected(it);
                break;
        }
    }
    for(auto& prop : properties)
    {
        if(prop.second.def == pdNotDefault)
        {
            continue;
        }
        for(auto& message : messages)
        {
            if(prop.second.def == pdDefaultForMessage)
            {
                message.second.properties.push_front(prop.second);
            }
            else if(prop.second.def == pdDefaultForField)
            {
                for(auto& field : message.second.fields)
                {
                    field.properties.push_front(prop.second);
                }
            }
            else if(prop.second.def == pdDefaultForType)
            {
                for(auto& field : message.second.fields)
                {
                    if(field.ft.fk == fkType)
                    {
                        field.ft.properties.push_front(prop.second);
                    }
                }
            }
        }
        if(prop.second.def == pdDefaultForEnum)
        {
            for(auto& eit : enumMap)
            {
                eit.second.properties.push_front(prop.second);
            }
        }
        if(prop.second.def == pdDefaultForField || prop.second.def == pdDefaultForType)
        {
            for(auto& fieldsSet : fieldsSets)
            {
                for(auto& field : fieldsSet.fields)
                {
                    if(prop.second.def == pdDefaultForField)
                    {
                        field.properties.push_front(prop.second);
                    }
                    else
                    {
                        if(field.ft.fk == fkType)
                        {
                            field.ft.properties.push_front(prop.second);
                        }
                    }
                }
            }
        }
    }
    for(auto& fieldsSet : fieldsSets)
    {
        for(auto pit = fieldsSet.properties.begin(); pit != fieldsSet.properties.end(); ++pit)
        {
            if(pit->second.def != pdDefaultForField)
            {
                continue;
            }
            for(auto& field : fieldsSet.fields)
            {
                bool found = false;
                for(auto& prop : field.properties)
                {
                    if(prop.name == pit->first)
                    {
                        prop = pit->second;
                        found = true;
                        break;
                    }
                }
                if(!found)
                {
                    field.properties.push_front(pit->second);
                }
            }
            for(auto& message : messages)
            {
                for(auto& field : message.second.fields)
                {
                    if(field.fsname == fieldsSet.name)
                    {
                        field.properties.push_back(pit->second);
                    }
                }
            }
        }
    }
    recursive--;
}

void Parser::fillPropertyField(TokensList::iterator& it, Property& p, PropertyField& pf)
{
    pf.name = p.name;
    pf.pt = p.pt;
    if(p.pt == ptBool)
    {
        pf.boolValue = expect(it, (TokenTypeList(ttTrue), ttFalse)).asBool();
    }
    else if(p.pt == ptInt)
    {
        pf.intValue = expect(it, (TokenTypeList(ttIntValue), ttHexValue)).asInt();
    }
    else//ptString
    {
        expect(it, ttStringValue);
        pf.strValue = it->value;
    }
}

}
