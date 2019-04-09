#pragma once

#include <utility>
#include <vector>
#include <string>
#include <map>
#include <list>
#include <inttypes.h>
#include <stdlib.h>
#include <set>

#include "Exceptions.hpp"
#include "FileReader.hpp"

namespace protogen {

enum TokenType {
    ttIdent, ttInclude, ttMessage, ttFieldSet, ttEnd, ttVersion,
    ttVersionValue, ttType, ttProperty, ttBool, ttInt, ttString,
    ttIntValue, ttHexValue, ttStringValue, ttDefault, ttTrue, ttFalse, ttEqual, ttColon,
    ttProtocol, ttEnum, ttFileName, ttEoln, ttPackage, ttEof
};

std::string TokenTypeToString(TokenType tt);

enum PropertyType {
    ptNone, ptBool, ptInt, ptString
};


struct PropertyField {
    PropertyType pt;
    std::string name;
    bool boolValue;
    int intValue;
    std::string strValue;
};

typedef std::list<PropertyField> PropertyFieldList;

enum PropertyDefault {
    pdNotDefault,
    pdDefaultForField,
    pdDefaultForMessage,
    pdDefaultForEnum,
    pdDefaultForType
};

struct Property {
    Property() : pt(ptNone), def(pdNotDefault)
    {
    }

    std::string name;
    PropertyType pt;
    PropertyDefault def;

    PropertyFieldList fields;

    void clear()
    {
        name = "";
        pt = ptNone;
        def = pdNotDefault;
        fields.clear();
    }
};

typedef std::list<Property> PropertyList;
typedef std::map<std::string, Property> PropertyMap;

enum FieldKind {
    fkType,
    fkNested,
    fkEnum
};

struct FieldType {
    std::string typeName;
    FieldKind fk;
    PropertyList properties;
};

typedef std::map<std::string, FieldType> FieldTypeMap;


struct Enum {
    std::string name;
    std::string typeName;
    std::string pkg;
    PropertyList properties;
    enum ValueType {
        vtInt, vtString
    };
    ValueType vt;
    struct EnumValue {
        std::string name;
        std::string strVal;
        int intVal;
    };
    std::vector<EnumValue> values;

    void clear()
    {
        values.clear();
    }
};

typedef std::map<std::string, Enum> EnumMap;


struct Field {

    FieldType ft;
    std::string name;
    std::string fsname;
    int tag = -1;
    PropertyList properties;

    void clear()
    {
        name = "";
        tag = -1;
        properties.clear();
    }
};

typedef std::vector<Field> FieldsVector;

struct FieldSet {
    std::string name;
    std::string pkg;
    FieldsVector fields;
    PropertyMap properties;
    bool used = false;

    typedef std::map<std::string, FieldsVector::iterator> FieldsMap;
    FieldsMap fieldsMap;

    void clear()
    {
        name = "";
        fields.clear();
        fieldsMap.clear();
    }

    void finish()
    {
        for(auto it = fields.begin(), end = fields.end(); it != end; ++it)
        {
            fieldsMap[it->name] = it;
        }
    }
};

typedef std::list<FieldSet> FieldSetsList;

struct Message {
    std::string name;
    std::string pkg;
    std::string parent;
    uint16_t majorVersion = 1;
    uint16_t minorVersion = 0;
    FieldsVector fields;
    PropertyList properties;
    int tag = 0;
    bool haveTag = false;

    void clear()
    {
        name = "";
        parent = "";
        haveTag = false;
        fields.clear();
        properties.clear();
    }
};

struct Protocol {
    std::string name;
    std::string pkg;
    struct MessageRecord {
        std::string msgName;
        PropertyList props;
    };
    typedef std::vector<MessageRecord> MessagesVector;
    MessagesVector messages;

    void clear()
    {
        messages.clear();
    }
};


typedef std::map<std::string, Protocol> ProtocolsMap;


class Parser {
public:
    Parser()
    {
        parsePos = tokens.end();
        recursive = 0;
        requireVersion = false;
    }

    void parseFile(const char* fileName);

    const ProtocolsMap& getProtocols() const
    {
        return protocols;
    }

    const EnumMap& getEnums() const
    {
        return enumMap;
    }

    const FieldSetsList& getFieldSets() const
    {
        return fieldsSets;
    }

    const FieldSet& getFieldset(const std::string& name)
    {
        for(auto& fieldsSet : fieldsSets)
        {
            if(fieldsSet.name == name)
            {
                return fieldsSet;
            }
        }
        throw FieldSetNotFoundException(name, "", 0, 0);
    }

    const Enum& getEnum(const std::string& enumName) const
    {
        auto it = enumMap.find(enumName);
        if(it == enumMap.end())
        {
            throw TypeNotFoundException(enumName, "", 0, 0);
        }
        return it->second;
    }

    const Protocol& getProtocol(const std::string& protoName) const
    {
        auto it = protocols.find(protoName);
        if(it == protocols.end())
        {
            throw ProtocolNotFoundException(protoName, "", 0, 0);
        }
        return it->second;
    }

    const Message& getMessage(const std::string& name) const
    {
        auto it = messages.find(name);
        if(it == messages.end())
        {
            throw MessageNotFoundException(name, "", 0, 0);
        }
        return it->second;
    }

    void addSearchPath(const std::string& path)
    {
        searchPath.push_back(path);
    }

    void disableSearchInCurDir()
    {
        searchInCurDir = false;
    }

    void setVersionRequirement(bool value)
    {
        requireVersion = value;
    }

    typedef std::map<std::string, Message> MessageMap;

    const MessageMap& getMessages() const
    {
        return messages;
    }

    const StrVector& getAllFiles()const
    {
        return files;
    }

protected:
    std::set<std::string> fsNames;
    FieldSetsList fieldsSets;
    MessageMap messages;
    ProtocolsMap protocols;
    FieldTypeMap types;
    PropertyMap properties;
    EnumMap enumMap;
    bool requireVersion;

    struct Token {
        Token() : tt(ttEof), file(0), line(0), col(0)
        {
        }

        Token(TokenType argTt, int argFile, int argLine, int argCol, std::string argValue = "") :
                tt(argTt), file(argFile), line(argLine), col(argCol), value(std::move(argValue))
        {
        }

        TokenType tt;
        int file;
        int line;
        int col;
        std::string value;

        int asInt() const
        {
            if(tt == ttIntValue)
            {
                return atoi(value.c_str());
            }
            int rv;
            sscanf(value.c_str(), "0x%x", &rv);
            return rv;
        }

        bool asBool() const
        {
            return tt == ttTrue;
        }
    };

    StrVector files;
    typedef std::list<Token> TokensList;
    TokensList tokens;
    TokensList::iterator parsePos;
    int recursive;

    StrVector searchPath;
    bool searchInCurDir = true;

    void pushToken(const Token& t)
    {
        parsePos = tokens.insert(parsePos, t);
        parsePos++;
    }

    struct TokenTypeList {
        TokenTypeList(TokenType argTt) : tt(argTt), prev(0)
        {
        }

        TokenType tt;
        const TokenTypeList* prev;

        TokenTypeList operator,(TokenType argTt) const
        {
            TokenTypeList node(argTt);
            node.prev = this;
            return node;
        }
    };

    const Token& expect(TokensList::iterator& it, const TokenTypeList& ttl);

    void unexpected(TokensList::iterator it)
    {
        throw ParsingException("Unexpected '" + TokenTypeToString(it->tt) + "' at", files[it->file], it->line, it->col);
    }

    void fillPropertyField(TokensList::iterator& it, Property& p, PropertyField& pf);

};

} // namespace protogen
