#include "Project.hpp"
#include <algorithm>

namespace protogen {

namespace {

bool split(std::string& source, char delim, std::string& tail)
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
    if(ext == ":")
    {
        sv.clear();
        sv.push_back(val);
        return;
    }
    size_t idx = std::stoul(ext) - 1;
    if(idx > sv.size())
    {
        throw std::runtime_error("Invalid index '" + ext + "' for option '" + opt + "'");
    }
    sv.resize(idx + 1);
    sv[idx] = val;
}

bool pkgMatch(const std::string& pkgName, const std::string& pkgMask)
{
    if(!pkgMask.empty() && pkgMask.back() == '*')
    {
        return pkgName.substr(0, pkgMask.length() - 1) == pkgMask.substr(0, pkgMask.length() - 1);
    }
    else
    {
        return pkgName == pkgMask;
    }
}

void addPathEndSlash(std::string& path)
{
    if(!path.empty() && path.back() != '/' && path.back() != '\\')
    {
        path += '/';
    }
}

}

bool Project::load(const std::string& fileName, const StrVector& optionsOverride)
{
    FILE* f = fopen(fileName.c_str(), "rt");
    if(!f)
    {
        print("Failed to open %s for reading\n", fileName);
        return false;
    }
    std::unique_ptr<FILE, decltype(&fclose)> fileGuard(f, &fclose);
    char buf[1024];
    StrVector projectSrc;
    while(fgets(buf, sizeof(buf), f))
    {
        projectSrc.push_back(buf);
    }

    std::string basePath;

    {
        auto slashPos = fileName.rfind('/');
        if(slashPos == std::string::npos)
        {
            slashPos = fileName.rfind('\\');
        }
        if(slashPos != std::string::npos)
        {
            basePath = fileName.substr(0, slashPos + 1);
            m_searchPaths.push_back(basePath);
        }
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
            print("Unrecognized line:'%s'\n", line);
            return false;
        }
        std::string name;
        std::string value;
        std::string ext;
        name = line.substr(0, pos);
        value = line.substr(pos + 1);
        split(name, ':', ext);
        if(name == "message.template")
        {
            setOptValue(m_msgTemplates, name, ext, value);
        }
        else if(name == "protocol.template")
        {
            setOptValue(m_protoTemplates, name, ext, value);
        }
        else if(name == "enum.template")
        {
            setOptValue(m_enumTemplates, name, ext, value);
        }
        else if(name == "fieldset.template")
        {
            setOptValue(m_fsTemplates, name, ext, value);
        }
        else if(name == "out.global.dir")
        {
            m_globalOutDir = value;
            addPathEndSlash(m_globalOutDir);
        }
        else if(name == "out.extension")
        {
            setOptValue(m_protoExtensions, name, ext, value);
            setOptValue(m_msgExtensions, name, ext, value);
            setOptValue(m_enumExtensions, name, ext, value);
            setOptValue(m_fsExtensions, name, ext, value);
        }
        else if(name == "out.protocol.extension")
        {
            setOptValue(m_protoExtensions, name, ext, value);
        }
        else if(name == "out.message.extension")
        {
            setOptValue(m_msgExtensions, name, ext, value);
        }
        else if(name == "out.enum.extension")
        {
            setOptValue(m_enumExtensions, name, ext, value);
        }
        else if(name == "out.fieldset.extension")
        {
            setOptValue(m_fsExtensions, name, ext, value);
        }
        else if(name == "out.protocol.prefix")
        {
            setOptValue(m_protoPrefix, name, ext, value);
        }
        else if(name == "out.protocol.suffix")
        {
            setOptValue(m_protoSuffix, name, ext, value);
        }
        else if(name == "out.message.prefix")
        {
            setOptValue(m_msgPrefix, name, ext, value);
        }
        else if(name == "out.message.suffix")
        {
            setOptValue(m_msgSuffix, name, ext, value);
        }
        else if(name == "out.enum.prefix")
        {
            setOptValue(m_enumPrefix, name, ext, value);
        }
        else if(name == "out.enum.suffix")
        {
            setOptValue(m_enumSuffix, name, ext, value);
        }
        else if(name == "out.fieldset.prefix")
        {
            setOptValue(m_fsPrefix, name, ext, value);
        }
        else if(name == "out.fieldset.suffix")
        {
            setOptValue(m_fsSuffix, name, ext, value);
        }
        else if(name == "option")
        {
            if(value != "true" && value != "false")
            {
                print("option value can be 'true' or 'false'\n");
                return false;
            }
            m_dataSource.setBool(ext, value == "true");
        }
        else if(name == "idxoption")
        {
            if(value != "true" && value != "false")
            {
                print("option value can be 'true' or 'false'\n");
                return false;
            }
            std::string var = ext;
            if(split(var, ':', ext))
            {
                int idx = std::stoi(ext) - 1;
                if(idx<0)
                {
                    print("Invalid extension index %d", idx);
                    return false;
                }
                m_idxOptions[var].resize(static_cast<size_t>(idx + 1));
                m_idxOptions[var][idx] = value == "true";
            }
            else
            {
                m_idxOptions[var].push_back(value == "true");
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
                    protogen::DataSource::Loop& lp = m_dataSource.createLoop(var, false);
                    lp.getItem(std::stoul(ext) - 1).addVar(loopVar, value);
                    //ds.setLoopVar(var,atoi(ext.c_str())-1,loopVar,value);
                }
                else
                {
                    protogen::DataSource::Loop& lp = m_dataSource.createLoop(var, false);
                    lp.getItem(std::stoul(ext) - 1).addVar(var, value);
                    //ds.setLoopVar(var,atoi(ext.c_str())-1,var,value);
                }
            }
            else
            {
                m_dataSource.setVar(var, value);
            }
        }
        else if(name == "idxdata")
        {
            std::string var = ext;
            if(split(var, ':', ext))
            {
                size_t idx = std::stoul(ext) - 1;
                m_idxData[var].resize(idx + 1);
                m_idxData[var][idx] = value;
            }
            else
            {
                m_idxData[ext].push_back(value);
            }
        }
        else if(name == "source")
        {
            setOptValue(m_sources, name, ext, value);
        }
        else if(name == "generate.protocol")
        {
            setOptValue(m_protoToGen, name, ext, value);
        }
        else if(name == "generate.message")
        {
            setOptValue(m_msgToGen, name, ext, value);
        }
        else if(name == "generate.enum")
        {
            setOptValue(m_enumToGen, name, ext, value);
        }
        else if(name == "generate.package")
        {
            setOptValue(m_pkgToGen, name, ext, value);
        }
        else if(name == "generate.fieldset")
        {
            setOptValue(m_fsToGen, name, ext, value);
        }
        else if(name == "search.path")
        {
            setOptValue(m_searchPaths, name, ext, value);
            if(!basePath.empty() && !m_searchPaths.empty() &&
               !m_searchPaths.back().empty() && m_searchPaths.back().front() != '/')
            {
                m_searchPaths.push_back(basePath + m_searchPaths.back());
            }
        }
        else if(name == "out.dir")
        {
            std::string dir = value;
            addPathEndSlash(dir);
            setOptValue(m_protoOutDir, name, ext, dir);
            setOptValue(m_msgOutDir, name, ext, dir);
            setOptValue(m_enumOutDir, name, ext, dir);
            setOptValue(m_fsOutDir, name, ext, dir);
        }
        else if(name == "out.protocol.dir")
        {
            std::string dir = value;
            addPathEndSlash(dir);
            setOptValue(m_protoOutDir, name, ext, dir);
        }
        else if(name == "out.message.dir")
        {
            std::string dir = value;
            addPathEndSlash(dir);
            setOptValue(m_msgOutDir, name, ext, dir);
        }
        else if(name == "out.enum.dir")
        {
            std::string dir = value;
            addPathEndSlash(dir);
            setOptValue(m_enumOutDir, name, ext, dir);
        }
        else if(name == "out.fieldset.dir")
        {
            std::string dir = value;
            addPathEndSlash(dir);
            setOptValue(m_fsOutDir, name, ext, dir);
        }
        else if(name == "requireMessageVersion")
        {
            m_reqMsgVersion = value == "true";
        }
        else if(name == "dryRun")
        {
            m_dryrun = true;
        }
        else if(name == "printDeps")
        {
            m_printDeps = true;
        }
        else if(name == "printGen")
        {
            m_printGen = true;
        }
        else if(name == "debugMode")
        {
            m_debugMode = value == "true";
        }
        else if(name == "verbose")
        {
            m_verbose = value == "true";
        }
        else if(name == "genFieldsets")
        {
            m_genFieldsets = value == "true";
        }
        else if(name == "searchInCurDir")
        {
            m_searchInCurDur = value == "true";
        }
        else
        {
            print("Unrecognized line:'%s'\n", line);
        }
    }

    if(m_sources.empty())
    {
        print("No sources to load\n");
        return false;
    }

    {
        StrVector* outDir[4] = {&m_protoOutDir, &m_msgOutDir, &m_enumOutDir, &m_fsOutDir};
        StrVector* ext[4] = {&m_protoExtensions, &m_msgExtensions, &m_enumExtensions, &m_fsExtensions};
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

    m_parser.setVersionRequirement(m_reqMsgVersion);

    for(auto& searchPath : m_searchPaths)
    {
        addPathEndSlash(searchPath);
        m_parser.addSearchPath(searchPath);
    }
    if(!m_searchInCurDur)
    {
        m_parser.disableSearchInCurDir();
    }

#define VPRINTF(...) do{if(m_verbose){print(__VA_ARGS__);}} while(0)

    for(auto& source : m_sources)
    {
        VPRINTF("Parsing %s\n", source);
        m_parser.parseFile(source.c_str());
    }

    for(auto& it : m_pkgToGen)
    {
        const auto& proto = m_parser.getProtocols();
        for(auto pit = proto.begin(); pit != proto.end(); pit++)
        {
            if(pkgMatch(pit->second.pkg, it))
            {
                VPRINTF("Add protocol %s to generation list via package\n", pit->first);
                m_protoToGen.push_back(pit->first);
            }
        }
        const protogen::Parser::MessageMap& msgs = m_parser.getMessages();
        for(auto mit = msgs.begin(), mend = msgs.end(); mit != mend; ++mit)
        {
            if(pkgMatch(mit->second.pkg, it))
            {
                VPRINTF("Add message %s to generation list via package\n", mit->first);
                m_msgToGen.push_back(mit->first);
            }
        }
        const protogen::EnumMap& enums = m_parser.getEnums();
        for(auto eit = enums.begin(), eend = enums.end(); eit != eend; ++eit)
        {
            if(pkgMatch(eit->second.pkg, it))
            {
                VPRINTF("Add enum %s to generation list via package\n", eit->first);
                m_enumToGen.push_back(eit->first);
            }
        }
        const protogen::FieldSetsList& fs = m_parser.getFieldSets();
        for(auto fit = fs.begin(), fend = fs.end(); fit != fend; ++fit)
        {
            if(pkgMatch(fit->pkg, it))
            {
                VPRINTF("Add fieldset %s to generation list via package\n", fit->name);
                m_fsToGen.push_back(fit->name);
            }
        }

    }

    if(m_protoToGen.empty() && m_pkgToGen.empty())
    {
        const protogen::ProtocolsMap& proto = m_parser.getProtocols();
        for(auto it = proto.begin(); it != proto.end(); it++)
        {
            VPRINTF("Add protocol %s to generation list by default\n", it->first);
            m_protoToGen.push_back(it->first);
        }
    }

    {
        std::sort(m_protoToGen.begin(), m_protoToGen.end());
        auto end = std::unique(m_protoToGen.begin(), m_protoToGen.end());
        m_protoToGen.erase(end, m_protoToGen.end());
    }
    if(m_printDeps)
    {
        print("%s\n", fileName);
    }
    return true;
}

bool Project::generate()
{
    FileFinder ff(m_searchPaths, m_searchInCurDur);
    for(auto& it : m_protoToGen)
    {
        m_dataSource.initForProtocol(m_parser, it);
        for(size_t idx = 0; idx < m_protoTemplates.size(); idx++)
        {
            VPRINTF("Parsing template %s for protocol %s\n", m_protoTemplates[idx], it);
            for(auto& idxOption : m_idxOptions)
            {
                if(idxOption.second.size() > idx)
                {
                    m_dataSource.setBool(idxOption.first, idxOption.second[idx]);
                }
            }
            for(auto& dit : m_idxData)
            {
                if(dit.second.size() > idx)
                {
                    m_dataSource.setVar(dit.first, dit.second[idx]);
                }
            }

            protogen::Template t;
            t.assignFileFinder(&ff);
            t.Parse(m_protoTemplates[idx]);
            if(m_debugMode)
            {
                print("Dump(%s):\n", m_protoTemplates[idx]);
                t.dump();
            }
            std::string result = t.Generate(m_dataSource);
            if(idx >= m_protoExtensions.size())
            {
                print("Extension for index %d not found\n", (int) idx);
                return false;
            }
            std::string fileName = it;
            if(m_protoPrefix.size() > idx)
            {
                fileName.insert(0, m_protoPrefix[idx]);
            }
            if(m_protoSuffix.size() > idx)
            {
                fileName += m_protoSuffix[idx];
            }
            fileName += "." + m_protoExtensions[idx];
            std::string fullPath = m_globalOutDir + m_protoOutDir[idx] + fileName;

            if(m_printGen)
            {
                print("%s\n", fullPath);
            }
            VPRINTF("Generating %s\n", fullPath);
            if(!m_dryrun)
            {
                FILE* outProto = fopen(fullPath.c_str(), "wb");
                if(!outProto)
                {
                    print("Failed to open file '%s' for writing\n", fullPath);
                    return false;
                }
                fwrite(result.c_str(), result.length(), 1, outProto);
                fclose(outProto);
            }
        }
        typedef protogen::Protocol::MessagesVector MsgVector;
        const protogen::Protocol& proto = m_parser.getProtocol(it);
        for(auto mit = proto.messages.begin(); mit != proto.messages.end(); mit++)
        {
            const protogen::Message& msg = m_parser.getMessage(mit->msgName);
            if(msg.pkg == proto.pkg)
            {
                VPRINTF("Add message %s to generation list via protocol %s\n", msg.name, proto.name);
                m_msgToGen.push_back(mit->msgName);
            }
        }
    }

    {
        if(std::find(m_msgToGen.begin(), m_msgToGen.end(), "*") != m_msgToGen.end())
        {
            m_msgToGen.clear();
            auto& msgs = m_parser.getMessages();
            for(auto& msg: msgs)
            {
                VPRINTF("Add message %s to generation list from explicit wildcard\n", msg.first);
                m_msgToGen.push_back(msg.first);
            }
        }

        std::set<std::string> processed;
        for(size_t idx = 0; idx < m_msgToGen.size(); idx++)
        {
            const protogen::Message& msg = m_parser.getMessage(m_msgToGen[idx]);
            if(!msg.parent.empty())
            {
                const protogen::Message& parentMsg = m_parser.getMessage(msg.parent);
                if(parentMsg.pkg == msg.pkg)
                {
                    VPRINTF("Add message %s to generation list as parent of %s\n", msg.parent, msg.name);
                    m_msgToGen.push_back(msg.parent);
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
                    const protogen::Message& nestedMsg = m_parser.getMessage(fit->ft.typeName);
                    if(nestedMsg.pkg == msg.pkg)
                    {
                        VPRINTF("Add message %s to generation list as field of %s\n", fit->ft.typeName,
                                msg.name);
                        m_msgToGen.push_back(fit->ft.typeName);
                    }
                }
                else if(fit->ft.fk == protogen::fkEnum)
                {
                    const protogen::Enum& en = m_parser.getEnum(fit->ft.typeName);
                    if(en.pkg == msg.pkg)
                    {
                        VPRINTF("Add enum %s to generation list as field of %s\n", fit->ft.typeName,
                                msg.name);
                        m_enumToGen.push_back(fit->ft.typeName);
                    }
                }
            }
        }
        if(m_genFieldsets)
        {
            const auto& lst = m_parser.getFieldSets();
            for(auto it = lst.begin(), end = lst.end(); it != end; ++it)
            {
                if(it->used)
                {
                    VPRINTF("Add fieldset %s to generation list as being used\n", it->name);
                    m_fsToGen.push_back(it->name);
                }
            }
        }

        auto fit = std::find(m_enumToGen.begin(), m_enumToGen.end(), "*");
        if(fit != m_enumToGen.end())
        {
            m_enumToGen.clear();
            auto& enums = m_parser.getEnums();
            for(auto& e:enums)
            {
                VPRINTF("Add enum %s to generation list from explicit wildcard\n", e.first);
                m_enumToGen.push_back(e.first);
            }
        }

        //VPRINTF("m_msgToGen.size=%u\n", m_msgToGen.size());

        std::sort(m_msgToGen.begin(), m_msgToGen.end());
        auto end = std::unique(m_msgToGen.begin(), m_msgToGen.end());
        m_msgToGen.erase(end, m_msgToGen.end());
        std::sort(m_enumToGen.begin(), m_enumToGen.end());
        end = std::unique(m_enumToGen.begin(), m_enumToGen.end());
        m_enumToGen.erase(end, m_enumToGen.end());
        std::sort(m_fsToGen.begin(), m_fsToGen.end());
        end = std::unique(m_fsToGen.begin(), m_fsToGen.end());
        m_fsToGen.erase(end, m_fsToGen.end());
    }

    for(auto& it : m_msgToGen)
    {
        VPRINTF("Generating msg %s\n", it);
        m_dataSource.initForMessage(m_parser, it);
        for(size_t idx = 0; idx < m_msgTemplates.size(); idx++)
        {
            VPRINTF("Parsing template %s for message %s\n", m_msgTemplates[idx], it);
            protogen::Template t;
            t.assignFileFinder(&ff);
            t.Parse(m_msgTemplates[idx]);
            if(m_debugMode)
            {
                print("Dump(%s):\n", m_msgTemplates[idx]);
                t.dump();
            }
            std::string result = t.Generate(m_dataSource);
            if(idx >= m_msgExtensions.size())
            {
                print("Extension for index %d not found\n", (int) idx);
                return false;
            }
            std::string fileName = it;
            if(m_msgPrefix.size() > idx)
            {
                fileName.insert(0, m_msgPrefix[idx]);
            }
            if(m_msgSuffix.size() > idx)
            {
                fileName += m_msgSuffix[idx];
            }
            fileName += "." + m_msgExtensions[idx];
            std::string fullPath = m_globalOutDir + m_msgOutDir[idx] + fileName;
            if(m_printGen)
            {
                print("%s\n", fullPath);
            }
            VPRINTF("Generating %s\n", fullPath);
            if(!m_dryrun)
            {
                FILE* outMsg = fopen(fullPath.c_str(), "wb");
                if(!outMsg)
                {
                    print("Failed to open file '%s' for writing\n", fullPath);
                    return false;
                }
                fwrite(result.c_str(), result.length(), 1, outMsg);
                fclose(outMsg);
            }
        }
    }

    for(auto& it : m_enumToGen)
    {
        m_dataSource.initForEnum(m_parser, it);
        for(size_t idx = 0; idx < m_enumTemplates.size(); idx++)
        {
            VPRINTF("Parsing template %s for enum %s\n", m_enumTemplates[idx], it);
            protogen::Template t;
            t.assignFileFinder(&ff);
            t.Parse(m_enumTemplates[idx]);
            if(m_debugMode)
            {
                print("Dump(%s):\n", m_fsTemplates[idx]);
                t.dump();
            }
            std::string result = t.Generate(m_dataSource);
            if(idx >= m_enumExtensions.size())
            {
                print("Enum extension for index %d not found\n", (int) idx);
                return false;
            }
            std::string fileName = it;
            if(m_enumPrefix.size() > idx)
            {
                fileName.insert(0, m_enumPrefix[idx]);
            }
            if(m_enumSuffix.size() > idx)
            {
                fileName += m_enumSuffix[idx];
            }
            fileName += "." + m_enumExtensions[idx];
            std::string fullPath = m_globalOutDir + m_enumOutDir[idx] + fileName;
            VPRINTF("Generating %s\n", fullPath);
            if(!m_dryrun)
            {
                FILE* outEnum = fopen(fullPath.c_str(), "wb");
                if(!outEnum)
                {
                    print("Failed to open file '%s' for writing\n", fullPath);
                    return false;
                }
                fwrite(result.c_str(), result.length(), 1, outEnum);
                fclose(outEnum);
            }
        }
    }

    for(auto& it : m_fsToGen)
    {
        m_dataSource.initForFieldSet(m_parser, m_parser.getFieldset(it));
        for(size_t idx = 0; idx < m_fsTemplates.size(); idx++)
        {
            VPRINTF("Parsing template %s for fieldset %s\n", m_fsTemplates[idx], it);
            protogen::Template t;
            t.assignFileFinder(&ff);
            t.Parse(m_fsTemplates[idx]);
            if(m_debugMode)
            {
                print("Dump(%s):\n", m_fsTemplates[idx]);
                t.dump();
            }
            std::string result = t.Generate(m_dataSource);
            if(idx >= m_fsExtensions.size())
            {
                print("FieldSet extension for index %d not found\n", (int) idx);
                return false;
            }
            std::string fileName = it;
            if(m_fsPrefix.size() > idx)
            {
                fileName.insert(0, m_fsPrefix[idx]);
            }
            if(m_fsSuffix.size() > idx)
            {
                fileName += m_fsSuffix[idx];
            }
            fileName += "." + m_fsExtensions[idx];
            std::string fullPath = m_globalOutDir + m_fsOutDir[idx] + fileName;
            if(m_printGen)
            {
                print("%s\n", fullPath);
            }
            VPRINTF("Generating %s\n", fullPath);
            if(!m_dryrun)
            {
                FILE* outFs = fopen(fullPath.c_str(), "wb");
                if(!outFs)
                {
                    print("Failed to open file '%s' for writing\n", fullPath);
                    return false;
                }
                fwrite(result.c_str(), result.length(), 1, outFs);
                fclose(outFs);
            }
        }
    }
    if(m_printDeps)
    {
        for(auto& file:m_parser.getAllFiles())
        {
            print("%s\n", file);
        }
        for(auto& file:ff.foundFiles)
        {
            print("%s\n", file);
        }
    }
    return true;
}

} // namespace protogen
