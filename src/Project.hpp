#pragma once

#include <functional>
#include <initializer_list>
#include "kst/Format.hpp"
#include "Utility.hpp"
#include "Template.hpp"
#include "Parser.hpp"
#include "TemplateDataSource.hpp"

namespace protogen {

class Project{
public:
    bool load(const std::string& fileName, const StrVector& optionsOverride);
    bool generate();
    void addSearchPath(std::string path)
    {
        m_searchPaths.push_back(std::move(path));
    }
    void setOutputFunc(std::function<void(const char*)> func)
    {
        m_outputFunc = func;
    }
private:
    class FileFinder : public protogen::IFileFinder {
    public:
        explicit FileFinder(StrVector& argSearchPath, bool argSearchInCurDir) :
            searchPath(argSearchPath), searchInCurDir(argSearchInCurDir)
        {

        }

        std::string findFile(const std::string& fileName) override
        {
            auto rv = FileReader::findFile(searchPath, fileName, searchInCurDir);
            if(!rv.empty())
            {
                foundFiles.insert(rv);
            }
            return rv;
        }

        StrVector& searchPath;
        std::set<std::string> foundFiles;
        bool searchInCurDir;
    };

    std::function<void(const char*)> m_outputFunc;


    template <class... Args>
    void print(Args... args)
    {
        if(m_outputFunc)
        {
            auto& argList = kst::FormatBuffer().getArgList();
            auto lst = {(argList,args)...};
            m_outputFunc(format(argList).Str());
        }
    }

    Parser m_parser;
    TemplateDataSource m_dataSource;

    bool m_reqMsgVersion = false;
    bool m_debugMode = false;
    bool m_verbose = false;
    bool m_dryrun = false;
    bool m_printDeps = false;
    bool m_printGen = false;
    bool m_genFieldsets = false;
    bool m_searchInCurDur = true;

    std::string m_globalOutDir;
    StrVector m_protoOutDir;
    StrVector m_msgOutDir;
    StrVector m_enumOutDir;
    StrVector m_fsOutDir;

    typedef std::vector<bool> BoolVector;
    typedef std::map<std::string, BoolVector> IdxOptions;
    typedef std::map<std::string, StrVector> IdxData;
    IdxOptions m_idxOptions;
    IdxData m_idxData;

    StrVector m_sources;

    StrVector m_protoToGen;
    StrVector m_msgToGen;
    StrVector m_enumToGen;
    StrVector m_fsToGen;
    StrVector m_pkgToGen;

    StrVector m_msgTemplates;
    StrVector m_protoTemplates;
    StrVector m_enumTemplates;
    StrVector m_fsTemplates;
    StrVector m_protoExtensions;
    StrVector m_msgExtensions;
    StrVector m_enumExtensions;
    StrVector m_fsExtensions;
    StrVector m_protoPrefix;
    StrVector m_protoSuffix;
    StrVector m_msgPrefix;
    StrVector m_msgSuffix;
    StrVector m_enumPrefix;
    StrVector m_enumSuffix;
    StrVector m_fsPrefix;
    StrVector m_fsSuffix;

    StrVector m_searchPaths;

};


}