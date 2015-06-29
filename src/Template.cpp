#include <stdio.h>
#include <string.h>
#include "Template.hpp"
#include "FileReader.hpp"

namespace protogen{

enum TmplCmd{
  tcMacro,
  tcPack,
  tcIf,
  tcIfdef,
  tcIfndef,
  tcForEach,
  tcSelect,
  tcCase,
  tcDefault,
  tcVar,
  tcSetBool,
  tcSetVar,
  tcExpand,
  tcInclude,
  tcElse,
  tcComment,
  tcError
};

struct CmdMapRec{
  const char* cmdName;
  TmplCmd cmd;
};
static CmdMapRec cmdMapArray[]=
{
  {"macro",tcMacro},
  {"pack",tcPack},
  {"if",tcIf},
  {"ifdef",tcIfdef},
  {"ifndef",tcIfndef},
  {"foreach",tcForEach},
  {"select",tcSelect},
  {"case",tcCase},
  {"default",tcDefault},
  {"var",tcVar},
  {"setbool",tcSetBool},
  {"setvar",tcSetVar},
  {"expand",tcExpand},
  {"include",tcInclude},
  {"else",tcElse},
  {"comment",tcComment},
  {"error",tcError}
};
typedef std::map<std::string,TmplCmd> CmdMap;
static CmdMap cMap;

struct CmdMapIniter{
  CmdMapIniter()
  {
    for(size_t i=0;i<sizeof(cmdMapArray)/sizeof(cmdMapArray[0]);i++)
    {
      //printf("%s,%d\n",cmdMapArray[i].cmdName,cmdMapArray[i].cmd);
      cMap.insert(CmdMap::value_type(cmdMapArray[i].cmdName,cmdMapArray[i].cmd));
    }
  }
}cmdMapIniter;

static void trimLine(std::string& str)
{
  int i=str.length()-1;
  while(i>0 && (str[i]==' ' || str[i]=='\t'))
  {
    i--;
  }
  i++;
  if(i>=0)str.erase(i);
}

bool myisspace(char c)
{
  return c==' ' || c=='\t';
}

static void skipSpaces(FileReader& fr)
{
  char c=0;
  while(!fr.eof() && myisspace(c=fr.getChar()))
  {
  }
  if(!fr.eof() && !myisspace(c))
  {
    fr.putChar();
  }
}

static std::string getContent(FileReader& fr,const char* endChars,char& endChar)
{
  std::string rv;
  char c=0;
  while(!fr.eof() && !strchr(endChars,c=fr.getChar()))
  {
    rv+=c;
  }
  endChar=c;
  return rv;
}

struct CmdStack{
  TmplCmd cmd;
  int idx;
  int idx2;
  std::string value;
  int line,col;
  CmdStack(TmplCmd argCmd,int argIdx,int argLine,int argCol,const std::string& argValue="",int argIdx2=-1):
    cmd(argCmd),idx(argIdx),idx2(argIdx2),value(argValue),line(argLine),col(argCol)
  {
  }
};


/*
static void expect(FileReader& fr,char ex)
{
  int line=fr.line;
  int col=fr.col;
  char c=fr.getChar();
  if(c!=ex)
  {
    std::string msg="Unexpected symbol '";
    msg+=c;
    msg+="' expecting '";
    msg+=ex;
    msg+="'";
    throw TemplateParsingException(msg,fr.fileName,line,col);
  }
}
*/

class InvalidArgumentIndex{
public:
  InvalidArgumentIndex(int argIdx):idx(argIdx)
  {
  }
  int idx;
};

std::string Template::expandMacro(const MacroInfo& mi,const std::vector<std::string>& args,const std::string& fileName,int line,int col)
{
  std::string rv;
  std::string::size_type prevPos=0,pos=0;
  while(pos!=std::string::npos)
  {
    pos=mi.macroText.find('%',pos);
    if(pos!=std::string::npos)
    {
      rv.append(mi.macroText.c_str()+prevPos,pos-prevPos);
      pos++;
      if(pos==mi.macroText.length()-1 || mi.macroText[pos]=='%')
      {
        rv+='%';
        pos++;
      }else
      {
        if(!isdigit(mi.macroText[pos]))
        {
          throw TemplateParsingException("expected argument index",fileName,line,col);
        }
        int idx,end;
        sscanf(mi.macroText.c_str()+pos,"%d%n",&idx,&end);
        if(idx==0)
        {
          throw TemplateParsingException("macro argument index is 1 based",fileName,line,col);
        }
        pos+=end;
        idx--;
        if((size_t)idx>=args.size())
        {
          throw InvalidArgumentIndex(idx);
        }
        rv+=args[idx];
      }
      prevPos=pos;
    }
  }
  rv+=mi.macroText.c_str()+prevPos;
  return rv;
}

void Template::Parse(const std::string& fileName)
{
  macroMap.clear();
  FileReader fr;
  std::string file=fileName;
  if(ff)
  {
    file=ff->findFile(file);
  }
  fr.Open(file);
  fr.file=files.size();
  files.push_back(file);
  Parse(fr);
  Op op;
  op.op=opEnd;
  ops.push_back(op);
}

enum BoolTerm{
  btVar,
  btVal,
  btNot,
  btEq,
  btNeq,
  btAnd,
  btOr,
  btOpen,
  btClose,
  btEnd
};

static int exprPrio[]={
    150,
    150,
    125,
    100,
    100,
    50,
    25,
    0,
    0
};

static BoolTerm getNextTerm(const std::string& expr,std::string::size_type& pos,std::string& val)
{
  val.clear();
  while(pos<expr.length() && isspace(expr[pos]))++pos;
  if(pos==expr.length())return btEnd;
  switch(expr[pos++])
  {
    case '"':
    {
      std::string::size_type spos=pos-1;
      while(expr[pos]!='"')
      {
        if(pos==expr.length())throw BoolExprParsingExpr("Unexpected end of string value",spos);
        if(expr[pos]=='\\')
        {
          ++pos;
          if(pos==expr.length())throw BoolExprParsingExpr("Unexpected end of string value",spos);
          switch(expr[pos])
          {
            case '"':val+='"';break;
            case 't':val+='\t';break;
            case 'n':val+='\n';break;
            case 'r':val+='\r';break;
            case 'x':
            {
              int hex,n;
              if(sscanf(expr.c_str()+pos+1,"%x%n",&hex,&n)!=1)
              {
                throw BoolExprParsingExpr("Invalid hex value",pos);
              }
              pos+=n-1;
            }break;
            default:throw BoolExprParsingExpr("Unexpected escape symbol",pos);
          }
          ++pos;
          continue;
        }
        val+=expr[pos++];
      }
      ++pos;
      return btVal;
    }break;
    case '!':
      if(expr[pos]=='=')
      {
        ++pos;
        return btNeq;
      }
      return btNot;
    case '=':
      if(expr[pos]=='=')++pos;
      return btEq;
    case '&':
      if(expr[pos]=='&')++pos;
      return btAnd;
    case '|':
      if(expr[pos]=='|')++pos;
      return btOr;
    case '(':
      return btOpen;
    case ')':
      return btClose;
    default:
      if(!isalpha(expr[pos-1]))break;
      val+=expr[pos-1];
      while(pos<expr.length() && (isalnum(expr[pos]) || expr[pos]=='_' || expr[pos]=='.'))
      {
        val+=expr[pos];
        ++pos;
      }
      return btVar;
  }
  char buf[64];
  if(expr[pos-1]>=32)
  {
    sprintf(buf,"Unexpected symbol '%c'",expr[pos-1]);
  }else
  {
    sprintf(buf,"Unexpected symbol '%02x'",(int)expr[pos-1]);
  }
  throw BoolExprParsingExpr(buf,pos-1);
}

int Template::parseBool(const std::string& expr,BoolTree& bt,std::string::size_type pos,int prio)
{
  std::string val;
  BoolTerm t;
  std::string::size_type oldPos;
  while((t=getNextTerm(expr,(oldPos=pos,pos),val))!=btEnd)
  {
    if(t==btVar)
    {
      if(bt.bop!=bopNone)
      {
        throw BoolExprParsingExpr("Unexpected var",oldPos);
      }
      bt.bop=bopVar;
      bt.varName=val;
      continue;
    }else if(t==btVal)
    {
      throw BoolExprParsingExpr("Unexpected string",pos-1);
    }else if(t==btOpen)
    {
      if(bt.bop!=bopNone)
      {
        throw BoolExprParsingExpr("Unexpected bracket",oldPos);
      }
      pos=parseBool(expr,bt,pos);
      if(getNextTerm(expr,pos,val)!=btClose)
      {
        throw BoolExprParsingExpr("Unbalanced brackets",oldPos);
      }
      continue;
    }else if(t==btNot)
    {
      if(bt.bop!=bopNone)
      {
        throw BoolExprParsingExpr("Unexpected !",oldPos);
      }
      BoolTree tmp;
      pos=parseBool(expr,tmp,pos,exprPrio[t]);
      if(tmp.bop==bopVar)
      {
        bt.bop=bopNotVar;
        bt.varName=tmp.varName;
      }else
      {
        bt.bop=bopNot;
        bt.left=new BoolTree(tmp);
      }
      continue;
    }
    int newPrio;
    BoolOp bop;
    switch(t)
    {
      case btAnd:bop=bopAnd;break;
      case btOr:bop=bopOr;break;
      case btEq:bop=bopEqVal;break;
      case btNeq:bop=bopNeqVal;break;
      default:return oldPos;
    }
    newPrio=exprPrio[t];
    if(newPrio<prio)
    {
      return oldPos;
    }
    if(bop==bopEqVal || bop==bopNeqVal)
    {
      if(bt.bop!=bopVar)
      {
        if(t==btEq)
          throw BoolExprParsingExpr("Unexpected ==",oldPos);
        if(t==btNeq)
          throw BoolExprParsingExpr("Unexpected ==",oldPos);
      }
      bt.bop=bop;
      oldPos=pos;
      t=getNextTerm(expr,pos,val);
      if(t!=btVar && t!=btVal)
      {
        throw BoolExprParsingExpr("Expected var or value",oldPos);
      }
      bt.value=val;
      if(t==btVar)
      {
        bt.bop=bop==bopEqVal?bopEqVar:bopNeqVar;
      }
    }else
    {
      bt.left=new BoolTree(bt);
      bt.bop=bop;
      bt.right=new BoolTree;
      pos=parseBool(expr,*bt.right,pos,newPrio);
    }
  }
  return pos;
}

void Template::Parse(FileReader& fr)
{
  std::string curText;
  std::string curLine;
  bool lineHasVarCommand=false;
  bool lineHasStaticText=false;
  bool lineHasCommand=false;

  bool macro=false;
  std::string macroName;


  //FileReader savedFr;
  //bool macroExpansion=false;

  std::vector<CmdStack> stack;

  char c;
  int line,col;
  while(!fr.eof())
  {
    line=fr.line;
    col=fr.col;
    c=fr.getChar();
    if(line!=fr.line)
    {
      line++;
      col=1;
    }
    if(c=='$')
    {
      if(fr.eof())
      {
        throw TemplateParsingException("Unexpected $ at the end of input",fr.fileName,line,col);
      }
      c=fr.getChar();
      if(c=='$')
      {
        curLine+=c;
        continue;
      }
      lineHasCommand=true;
      bool cmdEnd=false;
      if(c=='-')
      {
        cmdEnd=true;
      }
      if(!macro)
      {
        if(curText.length() || curLine.length())
        {
          Op op;
          op.op=opText;
          op.value=curText+curLine;
          ops.push_back(op);
          curText="";
          curLine="";
        }
      }
      if(c!='-')
      {
        fr.putChar();
      }
      std::string opname=getContent(fr," $",c);
      //printf("op='%s%s',c=%c,l=%d,col=%d,fr.pos=%d\n",cmdEnd?"-":"",opname.c_str(),c,line,col,fr.pos);
      CmdMap::const_iterator it=cMap.find(opname);
      if(it==cMap.end())
      {
        throw UnknownCmd(opname,fr.fileName,line,col);
      }
      if(it->second==tcVar)
      {
        lineHasVarCommand=true;
      }
      if(macro && it->second!=tcMacro)
      {
        curText+=curLine;
        curLine="";
        curText+='$';
        if(cmdEnd)
        {
          curText+='-';
        }
        curText+=opname;
        if(c==' ')
        {
          curText+=c;
          opname=getContent(fr,"$",c);
          curText+=opname;
          curText+=c;
        }else
        {
          curText+=c;
        }
        continue;
      }
      if(c==' ')
      {
        skipSpaces(fr);
      }
      switch(it->second)
      {
        case tcMacro:
        {
          if(cmdEnd)
          {
            //expect(fr,'$');
            MacroInfo mi;
            mi.macroName=macroName;
            mi.macroText=curText+curLine;
            macroMap.insert(MacroMap::value_type(mi.macroName,mi));
            macro=false;
            curText="";
            curLine="";
          }else
          {
            macroName=getContent(fr,"$",c);
            macro=true;
          }
        }break;
        case tcPack:
        {
          Op op;
          op.op=cmdEnd?opPackEnd:opPack;
          op.line=line;
          op.col=col;
          op.fidx=fr.file;
          ops.push_back(op);
        }break;
        case tcIf:
        {
          if(cmdEnd)
          {
            if(stack.empty() || stack.back().cmd!=tcIf)
            {
              throw TemplateParsingException("Unexpected end of if command",fr.fileName,line,col);
            }
            ops[stack.back().idx].jidx=ops.size();
            stack.pop_back();
          }else
          {
            stack.push_back(CmdStack(tcIf,ops.size(),line,col));
            Op op;
            op.op=opIf;
            line=fr.line;
            col=fr.col;
            op.value=getContent(fr,"$",c);
            op.line=line;
            op.col=col;
            op.fidx=fr.file;
            try{
              parseBool(op.value,op.boolValue);
            }catch(BoolExprParsingExpr& e)
            {
              throw TemplateParsingException(e.msg,fr.fileName,line,col+e.col);
            }
            ops.push_back(op);
          }
        }break;
        case tcIfdef:
        case tcIfndef:
        {
          if(cmdEnd)
          {
            if(stack.empty() || stack.back().cmd!=it->second)
            {
              throw TemplateParsingException("Unexpected end of ifdef/ifndef command",fr.fileName,line,col);
            }
            ops[stack.back().idx].jidx=ops.size();
            stack.pop_back();
          }else
          {
            stack.push_back(CmdStack(it->second,ops.size(),line,col));
            Op op;
            op.op=it->second==tcIfdef?opIfdef:opIfndef;
            op.value=getContent(fr,"$",c);
            op.line=line;
            op.col=col;
            op.fidx=fr.file;
            ops.push_back(op);
          }
        }break;
        case tcElse:
        {
          if(stack.empty())
          {
            throw TemplateParsingException("Unexpected else",fr.fileName,line,col);
          }
          if(stack.back().cmd==tcIf || stack.back().cmd==tcIfdef || stack.back().cmd==tcIfndef)
          {
            ops[stack.back().idx].jidx=ops.size()+1;
            Op op;
            op.op=opJump;
            op.line=line;
            op.col=col;
            op.fidx=fr.file;
            stack.back().idx=ops.size();
            stack.back().line=line;
            stack.back().col=col;
            ops.push_back(op);

            /*Op op=ops[stack.back().idx];
            op.line=line;
            op.col=col;
            if(stack.back().cmd==tcIfdef)
            {
              op.op=opIfndef;
            }else if(stack.back().cmd==tcIfndef)
            {
              op.op=opIfdef;
            }
            Op op;

            stack.back().idx=ops.size();
            stack.back().line=line;
            stack.back().col=col;
            ops.push_back(op);*/
          }else
          {
            throw TemplateParsingException("Unexpected else",fr.fileName,line,col);
          }
        }break;
        case tcForEach:
        {
          if(cmdEnd)
          {
            if(stack.empty() || stack.back().cmd!=tcForEach)
            {
              throw TemplateParsingException("Unexpected end of foreach command",fr.fileName,line,col);
            }
            Op op;
            op.op=opJump;
            op.jidx=stack.back().idx;
            ops.push_back(op);
            ops[stack.back().idx].jidx=ops.size();
            stack.pop_back();
          }else
          {
            stack.push_back(CmdStack(tcForEach,ops.size(),line,col));
            Op op;
            op.op=opLoop;
            op.line=line;
            op.col=col;
            op.fidx=fr.file;
            op.value=getContent(fr,"$",c);
            ops.push_back(op);
          }
        }break;
        case tcSelect:
        {
          if(cmdEnd)
          {
            SelectMap sm;
            bool haveDefault=false;
            while(!stack.empty() && (stack.back().cmd==tcCase || stack.back().cmd==tcDefault))
            {
              if(stack.back().cmd==tcDefault && haveDefault)
              {
                throw TemplateParsingException("Celect cannot have two default cases",fr.fileName,line,col);
              }
              haveDefault=stack.back().cmd==tcDefault;
              sm.insert(SelectMap::value_type(stack.back().value,stack.back().idx));
              if(stack.back().idx2!=-1)
              {
                ops[stack.back().idx2].jidx=ops.size();
              }
              stack.pop_back();
            }
            if(stack.empty() || stack.back().cmd!=tcSelect)
            {
              throw TemplateParsingException("Unexpected end of select command",fr.fileName,line,col);
            }
            ops[stack.back().idx].smap=sm;
            stack.pop_back();
          }else
          {
            stack.push_back(CmdStack(tcSelect,ops.size(),line,col));
            Op op;
            op.op=opSelect;
            op.line=line;
            op.col=col;
            op.fidx=fr.file;
            op.value=getContent(fr,"$",c);
            ops.push_back(op);
          }
        }break;
        case tcCase:
        {
          if(!stack.empty() && (stack.back().cmd==tcCase || stack.back().cmd==tcDefault))
          {
            stack.back().idx2=ops.size();
            Op op;
            op.op=opJump;
            ops.push_back(op);
          }
          stack.push_back(CmdStack(tcCase,ops.size(),line,col,getContent(fr,"$",c)));
        }break;
        case tcDefault:
        {
          if(!stack.empty() && stack.back().cmd==tcCase)
          {
            stack.back().idx2=ops.size();
            Op op;
            op.op=opJump;
            ops.push_back(op);
          }
          stack.push_back(CmdStack(tcDefault,ops.size(),line,col,""));
        }break;
        case tcVar:
        {
          Op op;
          op.op=opVar;
          op.line=line;
          op.col=col;
          op.fidx=fr.file;
          op.value=getContent(fr,":$",c);
          if(c==':')
          {
            line=fr.line;
            col=fr.col;
            std::string flags=getContent(fr,"$",c);
            if(flags=="uc")
            {
              op.varFlag=varFlagUc;
            }else if(flags=="ucf")
            {
              op.varFlag=varFlagUcf;
            }else if(flags=="hex")
            {
              op.varFlag=varFlagHex;
            }
            else
            {
              throw TemplateParsingException("Unexpected var flag",fr.fileName,line,col);
            }
          }
          ops.push_back(op);
        }break;
        case tcSetBool:
        {
          Op op;
          op.op=opSetBool;
          op.line=line;
          op.col=col;
          op.fidx=fr.file;
          op.value=getContent(fr," $",c);
          if(c=='$')
          {
            throw TemplateParsingException("Value expected in setbool",fr.fileName,line,col);
          }
          skipSpaces(fr);
          std::string val=getContent(fr,"$",c);
          if(val!="true" && val!="false")
          {
            throw TemplateParsingException("Value in setbool can only be true or false",fr.fileName,line,col);
          }
          op.varFlag=val=="true";
          ops.push_back(op);
        }break;
        case tcSetVar:
        {
          Op op;
          op.op=opSetVar;
          op.line=line;
          op.col=col;
          op.fidx=fr.file;
          op.value=getContent(fr," $",c);
          if(c=='$')
          {
            throw TemplateParsingException("Value expected in setvar",fr.fileName,line,col);
          }
          skipSpaces(fr);
          int vl=fr.line;
          int vc=fr.col;
          std::string value=getContent(fr,"$",c);
          std::string::size_type lastPos=0,pos=0;
          while(lastPos!=std::string::npos)
          {
            pos=value.find('%',pos);
            if(pos!=std::string::npos)
            {
              if(pos+1==value.size())
              {
                throw TemplateParsingException("Unexpected '%' symbol",fr.fileName,vl,vc+pos);
              }
              if(value[pos+1]=='%')
              {
                value.erase(pos,1);
                ++pos;
                continue;
              }
              if(lastPos!=pos)
              {
                Op vop;
                vop.op=opText;
                vop.value=value.substr(lastPos,pos-lastPos);
                op.varValue.push_back(vop);
              }
              ++pos;
              std::string::size_type endPos=value.find('%',pos);
              if(endPos==std::string::npos)
              {
                throw TemplateParsingException("setvar variable not closed",fr.fileName,vl,vc+pos);
              }
              Op vop;
              vop.op=opVar;
              vop.line=vl;
              vop.col=vc+pos;
              vop.fidx=fr.file;
              vop.value=value.substr(pos,endPos-pos);
              op.varValue.push_back(vop);
              pos=endPos+1;
            }else
            {
              Op vop;
              vop.op=opText;
              vop.value=value.substr(lastPos);
              op.varValue.push_back(vop);
            }
            lastPos=pos;
          }
          op.varValue.push_back(Op());
          op.varValue.back().op=opEnd;
          ops.push_back(op);
        }break;
        case tcInclude:
        {
          std::string file=getContent(fr,"$",c);
          if(ff)
          {
            file=ff->findFile(file);
          }
          FileReader ifr;
          ifr.Open(file);
          ifr.file=files.size();
          files.push_back(file);
          Parse(ifr);
        }break;
        case tcExpand:
        {
          macroName=getContent(fr," $",c);
          std::vector<std::string> args;
          while(c==' ')
          {
            skipSpaces(fr);
            args.push_back(getContent(fr," $",c));
          }
          MacroMap::iterator mit=macroMap.find(macroName);
          if(mit==macroMap.end())
          {
            throw TemplateParsingException("macro "+macroName+" not found",fr.fileName,line,col);
          }
          try
          {
            std::string macroTxt=expandMacro(mit->second,args,fr.fileName,line,col);
            //printf("Expanding macro:'%s'->'%s'\n",mit->second.macroText.c_str(),macroTxt.c_str());
            //savedFr=fr;
            FileReader macroFr;
            macroFr.fileName=fr.fileName;
            macroFr.fileName+="::";
            macroFr.fileName+=macroName;
            macroFr.source.insert(macroFr.source.begin(),macroTxt.c_str(),macroTxt.c_str()+macroTxt.length());
            macroFr.fileSize=macroTxt.length();
            macroFr.file=files.size();
            files.push_back(macroFr.fileName);
            Parse(macroFr);
          } catch(InvalidArgumentIndex& e)
          {
            char buf[128];
            sprintf(buf,"not enough arguments for macro (at least %d expected)",e.idx+1);
            throw TemplateParsingException(buf,fr.fileName,line,col);
          }
        }break;
        case tcError:
        {
          Op op;
          op.op=opError;
          op.line=line;
          op.col=col;
          op.fidx=fr.file;
          op.value=getContent(fr,"$",c);
          ops.push_back(op);
        }break;
        case tcComment:
          for(;;)
          {
            c=fr.getChar();
            if(fr.eof())
            {
              throw TemplateParsingException("Comment end not found",fr.fileName,line,col);
            }
            if(c=='$')
            {
              c=fr.getChar();
              if(c!='-')continue;
              std::string opname=getContent(fr,"$",c);
              CmdMap::const_iterator it=cMap.find(opname);
              if(it!=cMap.end())
              {
                if(it->second==tcComment)
                {
                  break;
                }
              }
            }
          }
          break;
      }
    }else if(c==0x0a)
    {
      //printf("curline='%s'\n",curLine.c_str());
      trimLine(curLine);
      if(curLine.length()!=0 || lineHasVarCommand || lineHasStaticText || !lineHasCommand || macro)
      {
        curText+=curLine;
        curText+=0x0a;
      }
      curLine="";
      lineHasVarCommand=false;
      lineHasStaticText=false;
      lineHasCommand=false;
    }else
    {
      curLine+=c;
      if(c!=' ' && c!='\t')
      {
        lineHasStaticText=true;
      }
    }
  }
  if(curText.length() || curLine.length())
  {
    Op op;
    op.op=opText;
    op.value=curText+curLine;
    ops.push_back(op);
    curText="";
    curLine="";
  }
  /*if(macroExpansion)
    {
      fr=savedFr;
      macroExpansion=false;
    }else
    {
      break;
    }*/
  if(!stack.empty())
  {
    std::string cmd;
    for(size_t i=0;i<sizeof(cmdMapArray)/sizeof(cmdMapArray[0]);i++)
    {
      if(stack.back().cmd==cmdMapArray[i].cmd)
      {
        cmd=cmdMapArray[i].cmdName;
        break;
      }
    }
    throw TemplateParsingException("Command "+cmd+" not ended at the end of template",fr.fileName,stack.back().line,stack.back().col);
  }
}

void Template::dump()
{
  for(size_t i=0;i<ops.size();i++)
  {
    printf("%d:",(int)i);
    switch(ops[i].op)
    {
      case opText:printf("text:'%s'\n",ops[i].value.c_str());break;
      case opVar:printf("var:%s\n",ops[i].value.c_str());break;
      case opLoop:printf("loop:%s:%d\n",ops[i].value.c_str(),ops[i].jidx);break;
      case opIf:printf("if:%s:%d\n",ops[i].value.c_str(),ops[i].jidx);break;
      case opIfdef:printf("ifdef:%s:%d\n",ops[i].value.c_str(),ops[i].jidx);break;
      case opIfndef:printf("ifndef:%s:%d\n",ops[i].value.c_str(),ops[i].jidx);break;
      case opJump:printf("jump:%d\n",ops[i].jidx);break;
      case opSelect:printf("select:%s\n",ops[i].value.c_str());break;
      case opPack:printf("pack\n");break;
      case opPackEnd:printf("packend\n");break;
      case opSetBool:printf("setbool:%s:%s\n",ops[i].value.c_str(),ops[i].varFlag?"true":"false");break;
      case opSetVar:
      {
        printf("setvar:%s:",ops[i].value.c_str());
        OpVector& v=ops[i].varValue;
        ops.swap(v);
        dump();
        ops.swap(v);
      }break;
      case opError:printf("error:%s\n",ops[i].value.c_str());break;
      case opEnd:printf("end\n");break;
    }
  }
}


}

