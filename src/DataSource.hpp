#ifndef __PROTOGEN_DATASOURCE_HPP__
#define __PROTOGEN_DATASOURCE_HPP__

#include <map>
#include <list>
#include <string>
#include "kst/Throw.hpp"

namespace protogen{

struct DataSource{
  DataSource()
  {
    top=&global;
  }
  typedef std::map<std::string,std::string> VarMap;
  typedef std::map<std::string,bool> BoolMap;
  struct Loop;
  typedef std::map<std::string,Loop> LoopMap;

  struct Namespace{
    Namespace():parent(0){}
    Namespace(const std::string& argParentName):parentName(argParentName),parent(0){}
    VarMap vars;
    BoolMap bools;
    LoopMap loops;
    std::string parentName;
    Namespace* parent;
    const std::string& getVar(const std::string& name)const
    {
      VarMap::const_iterator it;
      if(!name.empty() && name[0]=='.')
      {
        //printFmt("getVar %s\n",parentName+name);
        it=vars.find(parentName+name);
      }else
      {
        it=vars.find(name);
      }
      if(it==vars.end())
      {
        if(parent)
        {
          return parent->getVar(name);
        }else
        {
          KSTHROW("Template var not found:%s",name);
        }
      }
      return it->second;
    }
    bool getBool(const std::string& name)const
    {
      BoolMap::const_iterator it;
      if(!name.empty() && name[0]=='.')
      {
        it=bools.find(parentName+name);
      }else
      {
        it=bools.find(name);
      }
      if(it==bools.end())
      {
        if(parent)
        {
          return parent->getBool(name);
        }else
        {
          KSTHROW("Template bool not found:%s",name);
        }
      }
      return it->second;
    }
    bool haveVar(const std::string& name)const
    {
      bool rv=vars.find(name)!=vars.end() || bools.find(name)!=bools.end();
      if(!rv && parent)
      {
        rv=parent->haveVar(name);
      }
      return rv;
    }
    void setBool(const std::string& name,bool value)
    {
      bools[name]=value;
    }
    bool loopNext(const std::string& name)
    {
      LoopMap::iterator it=loops.find(name);
      if(it==loops.end())
      {
        KSTHROW("Template loop not found:%s",name);
      }
      return it->second.next();
    }
    Loop& getLoop(const std::string& name)
    {
      LoopMap::iterator it;
      if(!name.empty() && name[0]=='.')
      {
        it=loops.find(parentName+name);
      }else
      {
        it=loops.find(name);
      }
      if(it==loops.end())
      {
        if(parent)
        {
          return parent->getLoop(name);
        }else
        {
          KSTHROW("Template loop not found:%s",name);
        }
      }
      return it->second;
    }
    Loop& createLoop(const std::string& name,bool doClear=true)
    {
      if(parentName.empty())
      {
        Loop& rv=loops[name];
        if(doClear)rv.clear();
        rv.name=name;
        return rv;
      }else
      {
        Loop& rv=loops[parentName+"."+name];
        if(doClear)rv.clear();
        rv.name=parentName+"."+name;
        return rv;
      }
    }


    void addVar(const std::string& name,const std::string& val)
    {
      if(parentName.empty() || name.find('.')!=std::string::npos)
      {
        vars[name]=val;
      }else
      {
        vars[parentName+"."+name]=val;
      }
    }

    void addVar(const std::string& val)
    {
      vars[parentName]=val;
    }

    void addBool(const std::string& name,bool val)
    {
      if(parentName.empty() || name.find('.')!=std::string::npos)
      {
        bools[name]=val;
      }else
      {
        bools[parentName+"."+name]=val;
      }
    }

  };

  struct Loop{
    Loop():started(false),first(false)
    {
    }
    std::string name;
    std::list<Namespace> items;
    std::list<Namespace>::iterator current;
    bool started;
    bool first;
    void init()
    {
      current=items.begin();
      started=true;
      first=true;
    }
    void clear()
    {
      items.clear();
      started=false;
      first=false;
    }
    Namespace* get()
    {
      return &*current;
    }
    bool next()
    {
      if(!first)
      {
        current++;
      }else
      {
        first=false;
      }
      bool rv=current!=items.end();
      if(!rv)
      {
        started=false;
      }
      return rv;
    }
    Namespace& getItem(size_t idx)
    {
      if(idx>items.size())
      {
        KSTHROW("Unexpected loop index %{} for loop %{}",idx,name);
      }

      if(idx==items.size())
      {
        items.push_back(Namespace());
        return items.back();
      }else
      {
        std::list<Namespace>::iterator it=items.begin();
        std::advance(it,idx);
        return *it;
      }
    }
    Namespace& newItem()
    {
      bool first=items.empty();
      if(!first)
      {
        items.back().addBool("last",false);
      }
      items.push_back(Namespace(name));
      items.back().addBool("first",first);
      items.back().addBool("last",true);
      return items.back();
    }
  };
  Namespace global;
  Namespace* top;
  const std::string& getVar(const std::string& name)
  {
    return top->getVar(name);
  }
  bool getBool(const std::string& name)
  {
    return top->getBool(name);
  }
  bool haveVar(const std::string& name)
  {
    return top->haveVar(name);
  }
  void setBool(const std::string& name,bool value)
  {
    global.setBool(name,value);
  }
  bool loopNext(const std::string& name)
  {
    Loop& l=top->getLoop(name);
    if(!l.started)
    {
      l.init();
      if(l.next())
      {
        Namespace* nx=l.get();
        nx->parent=top;
        top=nx;
        return true;
      }else
      {
        return false;
      }
    }
    bool rv=l.next();
    if(rv)
    {
      top=top->parent;
      Namespace* nx=l.get();
      nx->parent=top;
      top=nx;
    }else
    {
      top=top->parent;
    }
    return rv;
  }

  void setVar(const std::string& name,const std::string& val)
  {
    global.vars[name]=val;
  }

  Loop& createLoop(const std::string& name,bool doClear=true)
  {
    return global.createLoop(name,doClear);
  }

};

}

#endif
