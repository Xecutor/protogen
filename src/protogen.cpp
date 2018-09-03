#include <set>
#include <string>
#include <stdio.h>
#include <stdexcept>
#include <algorithm>

#include "Parser.hpp"
#include "Template.hpp"
#include "FileReader.hpp"
#include "DataSource.hpp"

const char* sccs_version = "@(#) protogen 1.7.3 " __DATE__;

static std::string int2str(int val)
{
    char buf[32];
    sprintf(buf, "%d", val);
    return buf;
}

using protogen::StrVector;

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

struct TemplateDataSource : protogen::DataSource {

    void fillPackage(const char* varName, const std::string& pkg)
    {
        if(pkg.empty())
        {
            return;
        }
        setVar(varName, pkg);

        Loop& pkgLoop = createLoop(varName);
        const StrVector& pkgArr = splitString(pkg, ".");
        for(const auto& it : pkgArr)
        {
            pkgLoop.newItem().addVar("package.part", it);
        }
    }

    void initForMessage(protogen::Parser& p, const std::string& messageName)
    {
        using namespace protogen;

        const Message& msg = p.getMessage(messageName);
        setVar("message.name", messageName);
        setVar("message.versionMajor", int2str(msg.majorVersion));
        setVar("message.versionMinor", int2str(msg.minorVersion));
        fillPackage("message.package", msg.pkg);

        setBool("message.havetag", msg.haveTag);
        if(msg.haveTag)
        {
            setVar("message.tag", int2str(msg.tag));
        }

        setBool("message.haveparent", !msg.parent.empty());
        if(!msg.parent.empty())
        {
            setVar("message.parent", msg.parent);
            const Message& pmsg = p.getMessage(msg.parent);
            if(!pmsg.pkg.empty())
            {
                //varMap["message.parentpackage"]=pmsg.pkg;
                fillPackage("message.parentpackage", pmsg.pkg);
            }
        }


        Loop& msgFields = createLoop("field");
        fillFields(p, msgFields, msg.fields);
        {
            const Message* pmsg = msg.parent.empty() ? nullptr : &p.getMessage(msg.parent);
            Loop& msgParentFields = createLoop("parent.field");
            msgParentFields.name = "field";
            while(pmsg)
            {
                fillFields(p, msgParentFields, pmsg->fields);
                pmsg = pmsg->parent.empty() ? nullptr : &p.getMessage(pmsg->parent);
            }
        }

        for(const auto& prop : msg.properties)
        {
            for(const auto& field : prop.fields)
            {
                std::string n = "message.";
                n += field.name;
                if(field.pt == ptString)
                {
                    setVar(n, field.strValue);
                }
                else if(field.pt == ptBool)
                {
                    setBool(n, field.boolValue);
                }
                else
                {
                    setVar(n, int2str(field.intValue));
                }
            }
        }

    }

    void initForFieldSet(protogen::Parser& p, const protogen::FieldSet& fs)
    {
        setVar("fieldset.name", fs.name);
        fillPackage("fieldset.package", fs.pkg);
        fillFields(p, createLoop("field"), fs.fields);
    }


    void initForProtocol(const protogen::Parser& p, const std::string& protoName)
    {
        using namespace protogen;
        const Protocol& proto = p.getProtocol(protoName);
        setVar("protocol.name", protoName);
        fillPackage("protocol.package", proto.pkg);
        typedef protogen::Protocol::MessagesVector MsgVector;
        Loop& ld = createLoop("message");
        for(const auto& message : proto.messages)
        {
            const Message& msg = p.getMessage(message.msgName);
            Namespace& mn = ld.newItem();
            mn.addVar("message.name", msg.name);
            mn.addBool("message.havetag", msg.haveTag);
            mn.addBool("message.haveparent", !msg.parent.empty());
            if(!msg.parent.empty())
            {
                mn.addVar("message.parent", msg.parent);
            }

            if(msg.haveTag)
            {
                mn.addVar("message.tag", int2str(msg.tag));
            }
            for(const auto& prop : msg.properties)
            {
                for(const auto& field : prop.fields)
                {
                    if(field.pt == ptBool)
                    {
                        mn.addBool("message." + field.name, field.boolValue);
                    }
                    else if(field.pt == ptString)
                    {
                        mn.addVar("message." + field.name, field.strValue);
                    }
                    else if(field.pt == ptInt)
                    {
                        mn.addVar("message." + field.name, int2str(field.intValue));
                    }
                }
            }
            for(const auto& prop : message.props)
            {
                for(const auto& field : prop.fields)
                {
                    if(field.pt == ptBool)
                    {
                        mn.addBool("message." + field.name, field.boolValue);
                    }
                    else if(field.pt == ptString)
                    {
                        mn.addVar("message." + field.name, field.strValue);
                    }
                    else if(field.pt == ptInt)
                    {
                        mn.addVar("message." + field.name, int2str(field.intValue));
                    }
                }
            }
        }
    }

    void initForEnum(protogen::Parser& p, const std::string& enumName)
    {
        using namespace protogen;
        const Enum& e = p.getEnum(enumName);
        setVar("enum.name", e.name);
        setVar("enum.type", e.typeName);
        Loop& ld = createLoop("item");
        for(auto& val : e.values)
        {
            Namespace& en = ld.newItem();
            en.addVar("item.name", val.name);
            if(e.vt == Enum::vtString)
            {
                en.addVar("item.value", val.strVal);
            }
            else
            {
                en.addVar("item.value", int2str(val.intVal));
            }
        }

        for(const auto& prop : e.properties)
        {
            for(const auto& field : prop.fields)
            {
                std::string n = "enum." + field.name;
                if(field.pt == ptBool)
                {
                    setBool(n, field.boolValue);
                }
                else if(field.pt == ptString)
                {
                    setVar(n, field.strValue);
                }
                else if(field.pt == ptInt)
                {
                    setVar(n, int2str(field.intValue));
                }
            }
        }
    }

    void fillFields(protogen::Parser& p, Loop& ld, const protogen::FieldsVector& fields)
    {
        using namespace protogen;
        FieldsVector::const_iterator it, end;
        //beg = it = fields.begin();
        end = fields.end();
        for(; it != end; it++)
        {
            Namespace& fn = ld.newItem();
            fn.addVar("name", it->name);
            fn.addVar("tag", int2str(it->tag));
            if(it->ft.fk == fkNested)
            {
                fn.addVar("type", "nested");
                fn.addVar("typename", it->ft.typeName);
                const Message& fmsg = p.getMessage(it->ft.typeName);
                if(!fmsg.pkg.empty())
                {
                    fn.addVar("typepackage", fmsg.pkg);
                }
            }
            else if(it->ft.fk == fkEnum)
            {
                fn.addVar("type", "enum");
                fn.addVar("typename", it->ft.typeName);
                const Enum& fenum = p.getEnum(it->ft.typeName);
                fn.addVar("valuetype", fenum.typeName);
                if(!fenum.pkg.empty())
                {
                    fn.addVar("typepackage", fenum.pkg);
                }
            }
            else
            {
                const FieldType& ft = it->ft;
                fn.addVar("type", ft.typeName);
                for(const auto& prop : ft.properties)
                {
                    for(const auto& field : prop.fields)
                    {
                        if(field.pt == ptBool)
                        {
                            fn.addBool(field.name, field.boolValue);
                        }
                        else if(field.pt == ptInt)
                        {
                            fn.addVar(field.name, int2str(field.intValue));
                        }
                        else
                        {
                            fn.addVar(field.name, field.strValue);
                        }
                    }
                }
            }
            if(!it->fsname.empty())
            {
                fn.addVar("fieldset", it->fsname);
                const FieldSet& fs = p.getFieldset(it->fsname);
                if(!fs.pkg.empty())
                {
                    fn.addVar("package", fs.pkg);
                }
            }
            for(const auto& prop2 : it->properties)
            {
                for(const auto& field : prop2.fields)
                {
                    if(field.pt == ptString)
                    {
                        fn.addVar(field.name, field.strValue);
                    }
                    else if(field.pt == ptBool)
                    {
                        fn.addBool(field.name, field.boolValue);
                    }
                    else
                    {
                        fn.addVar(field.name, int2str(field.intValue));
                    }
                }
            }
        }
    }

/*
  void setLoopVar(const std::string& loopName,size_t idx,const std::string& varName,const std::string& varValue)
  {
    LoopMap::iterator it=loopMap.find(loopName);
    if(it==loopMap.end())
    {
      it=loopMap.insert(LoopMap::value_type(loopName,LoopData())).first;
    }

    LoopData& ld=it->second;

    if(idx>ld.loopVector.size())
    {
      throw std::runtime_error("Unexpected loop index for data "+loopName);
    }
    if(idx==ld.loopVector.size())
    {
      ld.loopVector.push_back(new LoopData::LoopVarRow);
    }
    ld.loopVector[idx]->push_back(LoopData::LoopVar(varName,varValue));
  }

  void setVar(const std::string& varName,const std::string& varValue)
  {
    varMap[varName]=varValue;
  }

  std::string getVar(const std::string& varName)
  {
    VarMap::const_iterator it=varMap.find(varName);
    if(it==varMap.end())
    {
      throw std::runtime_error("var not found:"+varName);
    }
    return it->second.value;
  }

  bool haveVar(const std::string& varName)
  {
    return varMap.find(varName)!=varMap.end() || boolMap.find(varName)!=boolMap.end();
  }

  void setBool(const std::string& boolName,bool boolValue)
  {
    boolMap[boolName]=boolValue;
  }

  bool getBool(const std::string& boolName)
  {
    BoolMap::const_iterator it=boolMap.find(boolName);
    if(it==boolMap.end())
    {
      throw std::runtime_error("bool not found:"+boolName);
    }
    return it->second.value;
  }

  bool loopNext(const std::string& loopName)
  {
    LoopMap::iterator it=loopMap.find(loopName);
    if(it==loopMap.end())
    {
      throw std::runtime_error("loop not found:"+loopName);
    }
    LoopData& ld=it->second;
    return ld.next(varMap,boolMap);
  }

  struct VarData{
    VarData(const std::string& argValue=""):value(argValue)
    {
    }
    std::string value;
    //bool loopVar;
  };

  struct BoolData{
    BoolData(bool argValue=false):value(argValue)
    {
    }
    bool value;
    //bool loopVar;
  };

  typedef std::map<std::string,VarData> VarMap;
  VarMap varMap;
  typedef std::map<std::string,BoolData> BoolMap;
  BoolMap boolMap;


  struct LoopData{
    size_t idx;

    LoopData():idx(0)
    {
    }

    void clear()
    {
      for(LoopVector::iterator it=loopVector.begin();it!=loopVector.end();it++)
      {
        delete *it;
      }
      loopVector.clear();
    }

    struct LoopVar
    {
      LoopVar():bValue(false),boolVar(false)
      {
      }
      LoopVar(const std::string& argName,const std::string& argValue):name(argName),value(argValue),bValue(false),boolVar(false)
      {
      }
      LoopVar(const std::string& argName,const char* argValue):name(argName),value(argValue),bValue(false),boolVar(false)
      {
      }
      LoopVar(const std::string& argName,bool argValue):name(argName),bValue(argValue),boolVar(true)
      {
      }
      std::string name;
      std::string value;
      bool bValue;
      bool boolVar;
    };
    typedef std::vector<LoopVar> LoopVarRow;
    typedef std::vector<LoopVarRow*> LoopVector;
    LoopVector loopVector;

    bool next(VarMap& vm,BoolMap& bm)
    {
      if(idx>=loopVector.size())
      {
        if(!loopVector.empty())
        {
          LoopVarRow::iterator end=loopVector[idx-1]->end();
          for(LoopVarRow::iterator it=loopVector[idx-1]->begin();it!=end;it++)
          {
            if(it->boolVar)
            {
              bm.erase(it->name);
            }else
            {
              vm.erase(it->name);
            }
          }
        }
        idx=0;
        return false;
      }
      if(idx>0)
      {
        LoopVarRow::iterator end=loopVector[idx-1]->end();
        for(LoopVarRow::iterator it=loopVector[idx-1]->begin();it!=end;it++)
        {
          if(it->boolVar)
          {
            bm.erase(it->name);
          }else
          {
            vm.erase(it->name);
          }
        }
      }
      LoopVarRow::iterator end=loopVector[idx]->end();
      for(LoopVarRow::iterator it=loopVector[idx]->begin();it!=end;it++)
      {
        if(it->boolVar)
        {
          bm[it->name]=it->bValue;
        }else
        {
          vm[it->name]=it->value;
        }
      }
      idx++;
      return true;
    }
  };



  typedef std::map<std::string,LoopData> LoopMap;
  LoopMap loopMap;

  typedef std::map<std::string,int> MsgProps;
  MsgProps msgProps;
  MsgProps enumProps;*/

    void dumpContext()
    {
        printf("Current context vars dump:\n");
        for(auto& var : top->vars)
        {
            printf("%s='%s'\n", var.first.c_str(), var.second.c_str());
        }
    }
};

static bool split(std::string& source, char delim, std::string& tail)
{
    std::string::size_type pos = source.find(delim);
    if(pos == std::string::npos)
    {
        return false;
    }
    tail = source.substr(pos + 1);
    source.erase(pos);
    return true;
}


void setOptValue(StrVector& sv, const std::string& opt, const std::string& ext, const std::string& val)
{
    if(ext.length() == 0)
    {
        sv.push_back(val);
        return;
    }
    //printf("ext=%s\n",ext.c_str());
    if(ext == ":")
    {
        sv.clear();
        sv.push_back(val);
        return;
    }
    size_t idx = atoi(ext.c_str()) - 1;
    if(idx > sv.size())
    {
        throw std::runtime_error("Invalid index '" + ext + "' for option '" + opt + "'");
    }
    sv.resize(idx + 1);
    sv[idx] = val;
}

class FileFinder : public protogen::IFileFinder {
public:
    explicit FileFinder(StrVector& argSearchPath) : searchPath(argSearchPath)
    {

    }

    std::string findFile(const std::string& fileName) override
    {
        return protogen::findFile(searchPath, fileName);
    }

protected:
    StrVector& searchPath;
};

bool pkgMatch(const std::string& pkgName, const std::string& pkgMask)
{
    if(*pkgMask.rbegin() == '*')
    {
        return pkgName.substr(0, pkgMask.length() - 1) == pkgMask.substr(0, pkgMask.length() - 1);
    }
    else
    {
        return pkgName == pkgMask;
    }
}

int main(int argc, char* argv[])
{
    protogen::Parser p;
    TemplateDataSource ds;
    try
    {
        if(argc == 1)
        {
            printf("Usage: protogen [--options] projectfile [--options]\n");
            return 0;
        }
        std::string projectFileName;
        StrVector protoOutDir;
        StrVector msgOutDir;
        StrVector enumOutDir;
        StrVector fsOutDir;
        StrVector optionsOverride;
        for(int i = 1; i < argc; i++)
        {
            std::string option = argv[i];
            if(option.substr(0, 2) == "--")
            {
                optionsOverride.push_back(option.substr(2));
                continue;
            }
            if(projectFileName.length())
            {
                printf("Only one project file expected in command line\n");
                return 1;
            }
            projectFileName = argv[i];
        }
        if(!projectFileName.length())
        {
            printf("Expected project file name in command line\n");
            return 1;
        }

        typedef std::vector<bool> BoolVector;
        typedef std::map<std::string, BoolVector> IdxOptions;
        typedef std::map<std::string, StrVector> IdxData;
        IdxOptions idxOptions;
        IdxData idxData;

        StrVector sources;

        StrVector protoToGen;
        StrVector msgToGen;
        StrVector enumToGen;
        StrVector fsToGen;
        StrVector pkgToGen;

        StrVector msgTemplates;
        StrVector protoTemplates;
        StrVector enumTemplates;
        StrVector fsTemplates;
        StrVector protoExtensions;
        StrVector msgExtensions;
        StrVector enumExtensions;
        StrVector fsExtensions;
        StrVector protoPrefix;
        StrVector protoSuffix;
        StrVector msgPrefix;
        StrVector msgSuffix;
        StrVector enumPrefix;
        StrVector enumSuffix;
        StrVector fsPrefix;
        StrVector fsSuffix;

        StrVector searchPaths;

        {
            const char* envsp = getenv("PROTOGEN_SEARCH_PATH");
            if(envsp)
            {
                std::string sp = envsp;
                if(*sp.rbegin() != '/')
                {
                    sp += '/';
                }
                searchPaths.push_back(sp);
            }
        }

        FileFinder ff(searchPaths);

        bool reqMsgVersion = false;
        bool debugMode = false;
        bool verbose = false;
        bool genFieldsets = false;

        FILE* f = fopen(projectFileName.c_str(), "rt");
        if(!f)
        {
            printf("Failed to open %s for reading\n", projectFileName.c_str());
            return 1;
        }
        char buf[1024];
        StrVector projectSrc;
        while(fgets(buf, sizeof(buf), f))
        {
            projectSrc.push_back(buf);
        }

        projectSrc.insert(projectSrc.end(), optionsOverride.begin(), optionsOverride.end());

        for(auto line : projectSrc)
        {
            while(line.length() && isspace(*line.rbegin()))
            {
                line.erase(line.rbegin().base() - 1);
            }
            while(line.length() && isspace(line[0]))
            {
                line.erase(0, 1);
            }
            if(line.length() == 0)
            {
                continue;
            }
            if(line[0] == '#')
            {
                continue;
            }
            std::string::size_type pos = line.find('=');
            if(pos == std::string::npos)
            {
                printf("Unrecognized line:'%s'\n", line.c_str());
                return 1;
            }
            std::string name;
            std::string value;
            std::string ext;
            name = line.substr(0, pos);
            value = line.substr(pos + 1);
            split(name, ':', ext);
            if(name == "message.template")
            {
                setOptValue(msgTemplates, name, ext, value);
            }
            else if(name == "protocol.template")
            {
                setOptValue(protoTemplates, name, ext, value);
            }
            else if(name == "enum.template")
            {
                setOptValue(enumTemplates, name, ext, value);
            }
            else if(name == "fieldset.template")
            {
                setOptValue(fsTemplates, name, ext, value);
            }
            else if(name == "out.extension")
            {
                setOptValue(protoExtensions, name, ext, value);
                setOptValue(msgExtensions, name, ext, value);
                setOptValue(enumExtensions, name, ext, value);
                setOptValue(fsExtensions, name, ext, value);
            }
            else if(name == "out.protocol.extension")
            {
                setOptValue(protoExtensions, name, ext, value);
            }
            else if(name == "out.message.extension")
            {
                setOptValue(msgExtensions, name, ext, value);
            }
            else if(name == "out.enum.extension")
            {
                setOptValue(enumExtensions, name, ext, value);
            }
            else if(name == "out.fieldset.extension")
            {
                setOptValue(fsExtensions, name, ext, value);
            }
            else if(name == "out.protocol.prefix")
            {
                setOptValue(protoPrefix, name, ext, value);
            }
            else if(name == "out.protocol.suffix")
            {
                setOptValue(protoSuffix, name, ext, value);
            }
            else if(name == "out.message.prefix")
            {
                setOptValue(msgPrefix, name, ext, value);
            }
            else if(name == "out.message.suffix")
            {
                setOptValue(msgSuffix, name, ext, value);
            }
            else if(name == "out.enum.prefix")
            {
                setOptValue(enumPrefix, name, ext, value);
            }
            else if(name == "out.enum.suffix")
            {
                setOptValue(enumSuffix, name, ext, value);
            }
            else if(name == "out.fieldset.prefix")
            {
                setOptValue(fsPrefix, name, ext, value);
            }
            else if(name == "out.fieldset.suffix")
            {
                setOptValue(fsSuffix, name, ext, value);
            }
            else if(name == "option")
            {
                if(value != "true" && value != "false")
                {
                    printf("option value can be 'true' or 'false'\n");
                    return 1;
                }
                ds.setBool(ext, value == "true");
            }
            else if(name == "idxoption")
            {
                if(value != "true" && value != "false")
                {
                    printf("option value can be 'true' or 'false'\n");
                    return 1;
                }
                std::string var = ext;
                if(split(var, ':', ext))
                {
                    int idx = atoi(ext.c_str()) - 1;
                    idxOptions[var].resize(idx + 1);
                    idxOptions[var][idx] = value == "true";
                }
                else
                {
                    idxOptions[var].push_back(value == "true");
                }
            }
            else if(name == "data")
            {
                std::string var = ext;
                if(split(var, ':', ext))
                {
                    std::string loopVar;
                    if(split(ext, ':', loopVar))
                    {
                        protogen::DataSource::Loop& lp = ds.createLoop(var, false);
                        lp.getItem(atoi(ext.c_str()) - 1).addVar(loopVar, value);
                        //ds.setLoopVar(var,atoi(ext.c_str())-1,loopVar,value);
                    }
                    else
                    {
                        protogen::DataSource::Loop& lp = ds.createLoop(var, false);
                        lp.getItem(atoi(ext.c_str()) - 1).addVar(var, value);
                        //ds.setLoopVar(var,atoi(ext.c_str())-1,var,value);
                    }
                }
                else
                {
                    ds.setVar(var, value);
                }
            }
            else if(name == "idxdata")
            {
                std::string var = ext;
                if(split(var, ':', ext))
                {
                    int idx = atoi(ext.c_str()) - 1;
                    idxData[var].resize(idx + 1);
                    idxData[var][idx] = value;
                }
                else
                {
                    idxData[ext].push_back(value);
                }
            }
            else if(name == "source")
            {
                setOptValue(sources, name, ext, value);
            }
            else if(name == "generate.protocol")
            {
                setOptValue(protoToGen, name, ext, value);
            }
            else if(name == "generate.message")
            {
                setOptValue(msgToGen, name, ext, value);
            }
            else if(name == "generate.enum")
            {
                setOptValue(enumToGen, name, ext, value);
            }
            else if(name == "generate.package")
            {
                setOptValue(pkgToGen, name, ext, value);
            }
            else if(name == "generate.fieldset")
            {
                setOptValue(fsToGen, name, ext, value);
            }
            else if(name == "search.path")
            {
                setOptValue(searchPaths, name, ext, value);
            }
            else if(name == "out.dir")
            {
                std::string dir = value;
                if(dir.length() && *dir.rbegin() != '/')
                {
                    dir += '/';
                }
                setOptValue(protoOutDir, name, ext, dir);
                setOptValue(msgOutDir, name, ext, dir);
                setOptValue(enumOutDir, name, ext, dir);
                setOptValue(fsOutDir, name, ext, dir);
            }
            else if(name == "out.protocol.dir")
            {
                std::string dir = value;
                if(dir.length() && *dir.rbegin() != '/')
                {
                    dir += '/';
                }
                setOptValue(protoOutDir, name, ext, dir);
            }
            else if(name == "out.message.dir")
            {
                std::string dir = value;
                if(dir.length() && *dir.rbegin() != '/')
                {
                    dir += '/';
                }
                setOptValue(msgOutDir, name, ext, dir);
            }
            else if(name == "out.enum.dir")
            {
                std::string dir = value;
                if(dir.length() && *dir.rbegin() != '/')
                {
                    dir += '/';
                }
                setOptValue(enumOutDir, name, ext, dir);
            }
            else if(name == "out.fieldset.dir")
            {
                std::string dir = value;
                if(dir.length() && *dir.rbegin() != '/')
                {
                    dir += '/';
                }
                setOptValue(fsOutDir, name, ext, dir);
            }
            else if(name == "requireMessageVersion")
            {
                reqMsgVersion = value == "true";
            }
            else if(name == "debugMode")
            {
                debugMode = value == "true";
            }
            else if(name == "verbose")
            {
                verbose = value == "true";
            }
            else if(name == "genFieldsets")
            {
                genFieldsets = value == "true";
            }
            else
            {
                printf("Unrecognized line:'%s'\n", line.c_str());
            }
        }

        if(sources.empty())
        {
            printf("No sources to load\n");
            return 1;
        }

        {
            StrVector* outDir[4] = {&protoOutDir, &msgOutDir, &enumOutDir, &fsOutDir};
            StrVector* ext[4] = {&protoExtensions, &msgExtensions, &enumExtensions, &fsExtensions};
            for(int j = 0; j < 4; j++)
            {
                if(outDir[j]->empty())
                {
                    for(size_t i = 0; i < ext[j]->size(); i++)
                    {
                        outDir[j]->push_back("./");
                    }
                }
                if(outDir[j]->size() == 1 && ext[j]->size() > 1)
                {
                    for(size_t i = 1; i < ext[j]->size(); i++)
                    {
                        outDir[j]->push_back((*outDir[j])[0]);
                    }
                }
            }
        }

        p.setVersionRequirement(reqMsgVersion);

        for(auto& searchPath : searchPaths)
        {
            if(!searchPath.empty() && *searchPath.rbegin() != '/')
            {
                searchPath += '/';
            }
            p.addSearchPath(searchPath);
        }

#define VPRINTF(...) if(verbose)printf(__VA_ARGS__)

        for(auto it = sources.begin(); it != sources.end(); it++)
        {
            VPRINTF("Parsing %s\n", it->c_str());
            p.parseFile(it->c_str());
        }

        for(auto& it : pkgToGen)
        {
            const auto& proto = p.getProtocols();
            for(auto pit = proto.begin(); pit != proto.end(); pit++)
            {
                if(pkgMatch(pit->second.pkg, it))
                {
                    VPRINTF("Add protocol %s to generation list via package\n", pit->first.c_str());
                    protoToGen.push_back(pit->first);
                }
            }
            const protogen::Parser::MessageMap& msgs = p.getMessages();
            for(auto mit = msgs.begin(), mend = msgs.end(); mit != mend; ++mit)
            {
                if(pkgMatch(mit->second.pkg, it))
                {
                    VPRINTF("Add message %s to generation list via package\n", mit->first.c_str());
                    msgToGen.push_back(mit->first);
                }
            }
            const protogen::EnumMap& enums = p.getEnums();
            for(auto eit = enums.begin(), eend = enums.end(); eit != eend; ++eit)
            {
                if(pkgMatch(eit->second.pkg, it))
                {
                    VPRINTF("Add enum %s to generation list via package\n", eit->first.c_str());
                    enumToGen.push_back(eit->first);
                }
            }
            const protogen::FieldSetsList& fs = p.getFieldSets();
            for(auto fit = fs.begin(), fend = fs.end(); fit != fend; ++fit)
            {
                if(pkgMatch(fit->pkg, it))
                {
                    VPRINTF("Add fieldset %s to generation list via package\n", fit->name.c_str());
                    fsToGen.push_back(fit->name);
                }
            }

        }

        if(protoToGen.empty() && pkgToGen.empty())
        {
            const protogen::ProtocolsMap& proto = p.getProtocols();
            for(auto it = proto.begin(); it != proto.end(); it++)
            {
                VPRINTF("Add protocol %s to generation list by default\n", it->first.c_str());
                protoToGen.push_back(it->first);
            }
        }

        {
            std::sort(protoToGen.begin(), protoToGen.end());
            auto end = std::unique(protoToGen.begin(), protoToGen.end());
            protoToGen.erase(end, protoToGen.end());
        }

        for(auto it = protoToGen.begin(); it != protoToGen.end(); it++)
        {
            ds.initForProtocol(p, *it);
            for(size_t idx = 0; idx < protoTemplates.size(); idx++)
            {
                VPRINTF("Parsing template %s for protocol %s\n", protoTemplates[idx].c_str(), it->c_str());
                for(auto& idxOption : idxOptions)
                {
                    if(idxOption.second.size() > idx)
                    {
                        ds.setBool(idxOption.first, idxOption.second[idx]);
                    }
                }
                for(auto& dit : idxData)
                {
                    if(dit.second.size() > idx)
                    {
                        ds.setVar(dit.first, dit.second[idx]);
                    }
                }

                protogen::Template t;
                t.assignFileFinder(&ff);
                t.Parse(protoTemplates[idx]);
                if(debugMode)
                {
                    printf("Dump(%s):\n", protoTemplates[idx].c_str());
                    t.dump();
                }
                std::string result = t.Generate(ds);
                if(idx >= protoExtensions.size())
                {
                    printf("Extension for index %d not found\n", (int) idx);
                    return 1;
                }
                std::string fileName = *it;
                if(protoPrefix.size() > idx)
                {
                    fileName.insert(0, protoPrefix[idx]);
                }
                if(protoSuffix.size() > idx)
                {
                    fileName += protoSuffix[idx];
                }
                fileName += "." + protoExtensions[idx];
                std::string fullPath = protoOutDir[idx] + fileName;
                VPRINTF("Generating %s\n", fullPath.c_str());
                FILE* outProto = fopen(fullPath.c_str(), "wb");
                if(!outProto)
                {
                    printf("Failed to open file '%s' for writing\n", fullPath.c_str());
                    return 1;
                }
                fwrite(result.c_str(), result.length(), 1, outProto);
                fclose(outProto);
            }
            typedef protogen::Protocol::MessagesVector MsgVector;
            const protogen::Protocol& proto = p.getProtocol(*it);
            for(auto mit = proto.messages.begin(); mit != proto.messages.end(); mit++)
            {
                const protogen::Message& msg = p.getMessage(mit->msgName);
                if(msg.pkg == proto.pkg)
                {
                    VPRINTF("Add message %s to generation list via protocol %s\n", msg.name.c_str(),
                            proto.name.c_str());
                    msgToGen.push_back(mit->msgName);
                }
            }
        }

        {
            std::set<std::string> processed;
            for(size_t idx = 0; idx < msgToGen.size(); idx++)
            {
                const protogen::Message& msg = p.getMessage(msgToGen[idx]);
                if(!msg.parent.empty())
                {
                    const protogen::Message& parentMsg = p.getMessage(msg.parent);
                    if(parentMsg.pkg == msg.pkg)
                    {
                        VPRINTF("Add message %s to generation list as parent of %s\n", msg.parent.c_str(),
                                msg.name.c_str());
                        msgToGen.push_back(msg.parent);
                    }
                }
                if(processed.find(msg.name) != processed.end())
                {
                    continue;
                }
                processed.insert(msg.name);
                for(auto fit = msg.fields.begin(); fit != msg.fields.end(); fit++)
                {
                    if(fit->ft.fk == protogen::fkNested)
                    {
                        const protogen::Message& nestedMsg = p.getMessage(fit->ft.typeName);
                        if(nestedMsg.pkg == msg.pkg)
                        {
                            VPRINTF("Add message %s to generation list as field of %s\n", fit->ft.typeName.c_str(),
                                    msg.name.c_str());
                            msgToGen.push_back(fit->ft.typeName);
                        }
                    }
                    else if(fit->ft.fk == protogen::fkEnum)
                    {
                        const protogen::Enum& en = p.getEnum(fit->ft.typeName);
                        if(en.pkg == msg.pkg)
                        {
                            VPRINTF("Add enum %s to generation list as field of %s\n", fit->ft.typeName.c_str(),
                                    msg.name.c_str());
                            enumToGen.push_back(fit->ft.typeName);
                        }
                    }
                }
            }
            if(genFieldsets)
            {
                const auto& lst = p.getFieldSets();
                for(auto it = lst.begin(), end = lst.end(); it != end; ++it)
                {
                    if(it->used)
                    {
                        VPRINTF("Add fieldset %s to generation list as being used\n", it->name.c_str());
                        fsToGen.push_back(it->name);
                    }
                }
            }
            auto fit = std::find(enumToGen.begin(), enumToGen.end(), "*");
            if(fit != enumToGen.end())
            {
                enumToGen.clear();
                const protogen::EnumMap& e = p.getEnums();
                for(auto it = e.begin(), end = e.end(); it != end; ++it)
                {
                    VPRINTF("Add enum %s to generation list from explicit wildcard\n", it->first.c_str());
                    enumToGen.push_back(it->first);
                }
            }
            std::sort(msgToGen.begin(), msgToGen.end());
            auto end = std::unique(msgToGen.begin(), msgToGen.end());
            msgToGen.erase(end, msgToGen.end());
            std::sort(enumToGen.begin(), enumToGen.end());
            end = std::unique(enumToGen.begin(), enumToGen.end());
            enumToGen.erase(end, enumToGen.end());
            std::sort(fsToGen.begin(), fsToGen.end());
            end = std::unique(fsToGen.begin(), fsToGen.end());
            fsToGen.erase(end, fsToGen.end());
        }

        for(auto it = msgToGen.begin(); it != msgToGen.end(); it++)
        {
            ds.initForMessage(p, *it);
            for(size_t idx = 0; idx < msgTemplates.size(); idx++)
            {
                VPRINTF("Parsing template %s for message %s\n", msgTemplates[idx].c_str(), it->c_str());
                protogen::Template t;
                t.assignFileFinder(&ff);
                t.Parse(msgTemplates[idx]);
                if(debugMode)
                {
                    printf("Dump(%s):\n", msgTemplates[idx].c_str());
                    t.dump();
                }
                std::string result = t.Generate(ds);
                if(idx >= msgExtensions.size())
                {
                    printf("Extension for index %d not found\n", (int) idx);
                    return 1;
                }
                std::string fileName = *it;
                if(msgPrefix.size() > idx)
                {
                    fileName.insert(0, msgPrefix[idx]);
                }
                if(msgSuffix.size() > idx)
                {
                    fileName += msgSuffix[idx];
                }
                fileName += "." + msgExtensions[idx];
                std::string fullPath = msgOutDir[idx] + fileName;
                VPRINTF("Generating %s\n", fullPath.c_str());
                FILE* outMsg = fopen(fullPath.c_str(), "wb");
                if(!outMsg)
                {
                    printf("Failed to open file '%s' for writing\n", fullPath.c_str());
                    return 1;
                }
                fwrite(result.c_str(), result.length(), 1, outMsg);
                fclose(outMsg);
            }
        }

        for(auto it = enumToGen.begin(); it != enumToGen.end(); it++)
        {
            ds.initForEnum(p, *it);
            for(size_t idx = 0; idx < enumTemplates.size(); idx++)
            {
                VPRINTF("Parsing template %s for enum %s\n", enumTemplates[idx].c_str(), it->c_str());
                protogen::Template t;
                t.assignFileFinder(&ff);
                t.Parse(enumTemplates[idx]);
                if(debugMode)
                {
                    printf("Dump(%s):\n", fsTemplates[idx].c_str());
                    t.dump();
                }
                std::string result = t.Generate(ds);
                if(idx >= enumExtensions.size())
                {
                    printf("Enum extension for index %d not found\n", (int) idx);
                    return 1;
                }
                std::string fileName = *it;
                if(enumPrefix.size() > idx)
                {
                    fileName.insert(0, enumPrefix[idx]);
                }
                if(enumSuffix.size() > idx)
                {
                    fileName += enumSuffix[idx];
                }
                fileName += "." + enumExtensions[idx];
                std::string fullPath = enumOutDir[idx] + fileName;
                VPRINTF("Generating %s\n", fullPath.c_str());
                FILE* outEnum = fopen(fullPath.c_str(), "wb");
                if(!outEnum)
                {
                    printf("Failed to open file '%s' for writing\n", fullPath.c_str());
                    return 1;
                }
                fwrite(result.c_str(), result.length(), 1, outEnum);
                fclose(outEnum);
            }
        }

        for(auto it = fsToGen.begin(); it != fsToGen.end(); it++)
        {
            ds.initForFieldSet(p, p.getFieldset(*it));
            for(size_t idx = 0; idx < fsTemplates.size(); idx++)
            {
                VPRINTF("Parsing template %s for fieldset %s\n", fsTemplates[idx].c_str(), it->c_str());
                protogen::Template t;
                t.assignFileFinder(&ff);
                t.Parse(fsTemplates[idx]);
                if(debugMode)
                {
                    printf("Dump(%s):\n", fsTemplates[idx].c_str());
                    t.dump();
                }
                std::string result = t.Generate(ds);
                if(idx >= fsExtensions.size())
                {
                    printf("FieldSet extension for index %d not found\n", (int) idx);
                    return 1;
                }
                std::string fileName = *it;
                if(fsPrefix.size() > idx)
                {
                    fileName.insert(0, fsPrefix[idx]);
                }
                if(fsSuffix.size() > idx)
                {
                    fileName += fsSuffix[idx];
                }
                fileName += "." + fsExtensions[idx];
                std::string fullPath = fsOutDir[idx] + fileName;
                VPRINTF("Generating %s\n", fullPath.c_str());
                FILE* outFs = fopen(fullPath.c_str(), "wb");
                if(!outFs)
                {
                    printf("Failed to open file '%s' for writing\n", fullPath.c_str());
                    return 1;
                }
                fwrite(result.c_str(), result.length(), 1, outFs);
                fclose(outFs);
            }
        }

/*
    p.parseFile("MyProtocol.src");
    t.Parse("message.tmpl");
    TemplateDataSource ds;
    ds.initForMessage(p,"TestMessage");
    std::vector<std::string> ns;
    ns.push_back("smsc");
    ns.push_back("protocol");
    ds.initNamespaces(ns);
    ds.setBool("options.throwOnUnsetGet",false);

    std::string result=t.Generate(ds);
    FILE* f=fopen("result.hpp","wb+");
    fwrite(result.c_str(),result.length(),1,f);
    fclose(f);
    */
    }
    catch(std::exception& e)
    {
        printf("Exception:\"%s\"\n", e.what());
        return 1;
    }
    return 0;
}
