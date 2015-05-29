#ifndef __PROTOGEN_PARSER_HPP__
#define __PROTOGEN_PARSER_HPP__

#include <vector>
#include <string>
#include <map>
#include <list>
#include <inttypes.h>
#include <stdlib.h>
#include <set>

#include "Exceptions.hpp"
#include "FileReader.hpp"

namespace protogen{

enum TokenType{
  ttIdent, ttInclude, ttMessage, ttFieldSet, ttEnd,ttVersion,
  ttVersionValue,ttType,ttProperty,ttBool,ttInt,ttString,
  ttIntValue,ttHexValue,ttStringValue,ttDefault,ttTrue,ttFalse, ttEqual,ttColon,
  ttProtocol,ttEnum,ttFileName,ttEoln,ttPackage,ttEof
};

inline std::string TokenTypeToString(TokenType tt)
{
  switch(tt)
  {
    case ttIdent:return "identifier";
    case ttInclude:return "include";
    case ttMessage:return "message";
    case ttFieldSet:return "fieldset";
    case ttEnd:return "end";
    case ttVersion:return "version";
    case ttVersionValue:return "version value";
    case ttType:return "type";
    case ttProperty:return "property";
    case ttBool:return "bool";
    case ttInt:return "int";
    case ttString:return "string";
    case ttIntValue:return "int value";
    case ttHexValue:return "hex value";
    case ttStringValue:return "string value";
    case ttDefault:return "default";
    case ttTrue:return "true";
    case ttFalse:return "false";
    case ttEqual:return "equal";
    case ttColon:return "colon";
    case ttProtocol:return "protocol";
    case ttEnum:return "enum";
    case ttEoln:return "end of line";
    case ttEof:return "end of file";
    case ttPackage:return "package";
    case ttFileName:return "file name";
  }
  return "unknown";
}

enum FieldKind{
  fkType,
  fkNested,
  fkEnum
};

struct FieldType{
  std::string typeName;
  FieldKind fk;
};

typedef std::map<std::string,FieldType> FieldTypeMap;



enum PropertyType{
  ptNone,ptBool,ptInt,ptString
};



struct PropertyField{
  PropertyType pt;
  std::string name;
  bool boolValue;
  int intValue;
  std::string strValue;
};

typedef std::list<PropertyField> PropertyFieldList;

struct Property{
  Property():pt(ptNone),isDefault(false),isMessage(false),isEnum(false)
  {
  }
  std::string name;
  PropertyType pt;
  bool isDefault;
  bool isMessage;
  bool isEnum;
  PropertyFieldList fields;
  void clear()
  {
    name="";
    pt=ptNone;
    isDefault=false;
    isMessage=false;
    isEnum=false;
    fields.clear();
  }
};

typedef std::list<Property> PropertyList;
typedef std::map<std::string,Property> PropertyMap;

struct Enum{
  std::string name;
  std::string typeName;
  std::string pkg;
  PropertyList properties;
  enum ValueType{
    vtInt,vtString
  };
  ValueType vt;
  struct EnumValue{
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

typedef std::map<std::string,Enum> EnumMap;



struct Field{
  Field():tag(-1)
  {

  }
  FieldType ft;
  std::string name;
  std::string fsname;
  int tag;
  PropertyList properties;
  void clear()
  {
    name="";
    tag=0;
    properties.clear();
  }
};

typedef std::vector<Field> FieldsVector;

struct FieldSet{
  std::string name;
  std::string pkg;
  FieldsVector fields;
  PropertyMap properties;
  bool used;

  FieldSet():used(false){}

  typedef std::map<std::string,FieldsVector::iterator> FieldsMap;
  FieldsMap fieldsMap;
  void clear()
  {
    name="";
    fields.clear();
    fieldsMap.clear();
  }
  void finish()
  {
    for(FieldsVector::iterator it=fields.begin(),end=fields.end();it!=end;++it)
    {
      fieldsMap[it->name]=it;
    }
  }
};

typedef std::list<FieldSet> FieldSetsList;

struct Message{
  std::string name;
  std::string pkg;
  std::string parent;
  uint8_t majorVersion;
  uint8_t minorVersion;
  FieldsVector fields;
  PropertyList properties;
  int tag;
  bool haveTag;

  Message():majorVersion(1),minorVersion(0),tag(0),haveTag(false)
  {

  }

  void clear()
  {
    name="";
    parent="";
    haveTag=false;
    fields.clear();
    properties.clear();
  }
};

struct Protocol{
  std::string name;
  std::string pkg;
  struct MessageRecord{
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


typedef std::map<std::string,Protocol> ProtocolsMap;


class Parser{
public:
  Parser()
  {
    parsePos=tokens.end();
    recursive=0;
    requireVersion=false;
  }
  void parseFile(const char* fileName);

  const ProtocolsMap& getProtocols()const
  {
    return protocols;
  }
  const EnumMap& getEnums()const
  {
    return enumMap;
  }

  const FieldSetsList& getFieldSets()const
  {
    return fieldsSets;
  }

  const FieldSet& getFieldset(const std::string& name)
  {
    for(protogen::FieldSetsList::iterator fit=fieldsSets.begin(),fend=fieldsSets.end();fit!=fend;++fit)
    {
      if(fit->name==name)
      {
        return *fit;
      }
    }
    throw FieldSetNotFoundException(name,"",0,0);
  }

  const Enum& getEnum(const std::string& enumName)const
  {
    EnumMap::const_iterator it=enumMap.find(enumName);
    if(it==enumMap.end())
    {
      throw TypeNotFoundException(enumName,"",0,0);
    }
    return it->second;
  }

  const Protocol& getProtocol(const std::string& protoName)const
  {
    ProtocolsMap::const_iterator it=protocols.find(protoName);
    if(it==protocols.end())
    {
      throw ProtocolNotFoundException(protoName,"",0,0);
    }
    return it->second;
  }
  const Message& getMessage(const std::string& name)const
  {
    MessageMap::const_iterator it=messages.find(name);
    if(it==messages.end())
    {
      throw MessageNotFoundException(name,"",0,0);
    }
    return it->second;
  }
  void addSearchPath(const std::string& path)
  {
    searchPath.push_back(path);
  }
  void setVersionRequirement(bool value)
  {
    requireVersion=value;
  }
  typedef std::map<std::string,Message> MessageMap;
  const MessageMap& getMessages()const
  {
    return messages;
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
  struct Token{
    Token():tt(ttEof),file(0),line(0),col(0)
    {
    }
    Token(TokenType argTt,int argFile,int argLine,int argCol,const std::string& argValue=""):
      tt(argTt),file(argFile),line(argLine),col(argCol),value(argValue)
    {
    }
    TokenType tt;
    int file;
    int line;
    int col;
    std::string value;
    int asInt()const
    {
      if(tt==ttIntValue)
      {
        return atoi(value.c_str());
      }
      int rv;
      sscanf(value.c_str(),"0x%x",&rv);
      return rv;
    }
    bool asBool()const
    {
      return tt==ttTrue?true:false;
    }
  };

  StrVector files;
  typedef std::list<Token> TokensList;
  TokensList tokens;
  TokensList::iterator parsePos;
  int recursive;

  StrVector searchPath;

  void pushToken(const Token& t)
  {
    parsePos=tokens.insert(parsePos,t);
    parsePos++;
  }

  struct TokenTypeList{
    TokenTypeList(TokenType argTt):tt(argTt),prev(0)
    {
    }
    TokenType tt;
    const TokenTypeList* prev;

    TokenTypeList operator,(TokenType argTt)const
    {
      TokenTypeList node(argTt);
      node.prev=this;
      return node;
    }
  };
  const Token& expect(TokensList::iterator& it,const TokenTypeList& ttl)
  {
    it++;
    const TokenTypeList* node=&ttl;
    while(node && it!=tokens.end())
    {
      if(it->tt==node->tt)
      {
        return *it;
      }
      node=node->prev;
    }
    node=&ttl;
    std::string msg;
    while(node)
    {
      msg+=TokenTypeToString(node->tt);
      node=node->prev;
      if(node)
      {
        msg+=',';
      }
    }

    throw ParsingException("Expected '"+msg+"' found '"+TokenTypeToString(it->tt)+"' at",files[it->file],it->line,it->col);
  }

  void unexpected(TokensList::iterator it)
  {
    throw ParsingException("Unexpected '"+TokenTypeToString(it->tt)+"' at",files[it->file],it->line,it->col);
  }

};

}

#endif

