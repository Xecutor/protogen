#ifndef __PROTOGEN_FILREADER_HPP__
#define __PROTOGEN_FILREADER_HPP__

#include <vector>
#include <string>
#include <stdexcept>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

namespace protogen{


typedef std::vector<std::string> StrVector;

inline std::string findFile(const StrVector& searchPath,const std::string& fileName)
{
  if(fileName.length() && fileName[0]=='/')
  {
    return fileName;
  }
  struct stat st;
  if(::stat(fileName.c_str(),&st)==0)
  {
    return fileName;
  }
  for(StrVector::const_iterator it=searchPath.begin();it!=searchPath.end();it++)
  {
    std::string fullPath=*it+fileName;
    if(::stat(fullPath.c_str(),&st)==0)
    {
      return fullPath;
    }
  }
  return fileName;
}


struct FileReader{
  FileReader():fileSize(0),pos(0),line(1),col(1),file(0),nextLine(false)
  {
  }
  std::vector<char> source;
  std::string fileName;
  long fileSize;
  int pos,line,col,file;
  bool nextLine;
  void Open(const std::string& argFileName)
  {
    fileName=argFileName;
    FILE* f=fopen(fileName.c_str(),"rb");
    if(!f)
    {
      std::string err="Failed to open file '";
      err+=fileName.c_str();
      err+="' for reading.";
      throw std::runtime_error(err);
    }
    fseek(f,0,SEEK_END);
    fileSize=ftell(f);
    fseek(f,0,SEEK_SET);
    source.resize(fileSize);
    fread(&source[0],fileSize,1,f);
    fclose(f);
  }
  char getChar()
  {
    if(pos>=fileSize)
    {
      throw std::runtime_error("attempt to read beyond eof");
    }
    if(nextLine)
    {
      line++;
      col=1;
      nextLine=false;
    }
    char rv=source[pos++];
    if(rv==0x0d || rv==0x0a)
    {
      if(rv==0x0d && pos<fileSize)
      {
        rv=source[pos++];
        if(rv!=0x0a)
        {
          pos--;
        }
      }
      nextLine=true;
      return 0x0a;
    }
    col++;
    return rv;
  }
  void putChar()
  {
    pos--;col--;
  }
  bool eof()
  {
    return pos>=fileSize;
  }
};

}

#endif

