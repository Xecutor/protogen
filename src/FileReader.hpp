#ifndef __PROTOGEN_FILREADER_HPP__
#define __PROTOGEN_FILREADER_HPP__

#include <vector>
#include <string>
#include <stdexcept>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

namespace protogen {


typedef std::vector<std::string> StrVector;


struct FileReader {

    std::vector<char> source;
    std::string fileName;
    size_t fileSize = 0;
    size_t pos = 0;
    size_t line = 1;
    size_t col = 1;
    int file = 0;
    bool nextLine = false;

    void Open(const std::string& argFileName)
    {
        fileName = argFileName;
        FILE* f = fopen(fileName.c_str(), "rb");
        if(!f)
        {
            std::string err = "Failed to open file '";
            err += fileName;
            err += "' for reading.";
            throw std::runtime_error(err);
        }
        fseek(f, 0, SEEK_END);
        fileSize = static_cast<size_t>(ftell(f));
        fseek(f, 0, SEEK_SET);
        source.resize(fileSize);
        fread(&source[0], fileSize, 1, f);
        fclose(f);
    }

    char getChar()
    {
        if(pos >= fileSize)
        {
            throw std::runtime_error("attempt to read beyond eof");
        }
        if(nextLine)
        {
            line++;
            col = 1;
            nextLine = false;
        }
        char rv = source[pos++];
        if(rv == 0x0d || rv == 0x0a)
        {
            if(rv == 0x0d && pos < fileSize)
            {
                rv = source[pos++];
                if(rv != 0x0a)
                {
                    pos--;
                }
            }
            nextLine = true;
            return 0x0a;
        }
        col++;
        return rv;
    }

    void putChar()
    {
        pos--;
        col--;
    }

    bool eof()
    {
        return pos >= fileSize;
    }
    static std::string findFile(const StrVector& searchPath, const std::string& fileName, bool searchInCurDir = true)
    {
        if(fileName.length() && fileName[0] == '/')
        {
            return fileName;
        }
        struct stat st = {};
        if(searchInCurDir)
        {
            if(::stat(fileName.c_str(), &st) == 0)
            {
                return fileName;
            }
        }
        for(const auto& it : searchPath)
        {
            std::string fullPath = it + fileName;
            if(::stat(fullPath.c_str(), &st) == 0)
            {
                return fullPath;
            }
        }
        return fileName;
    }
};

}

#endif

