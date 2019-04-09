#pragma once

#include "DataSource.hpp"
#include "Utility.hpp"
#include "Parser.hpp"

namespace protogen {

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

    void initForMessage(protogen::Parser& p, const std::string& messageName);

    void initForFieldSet(protogen::Parser& p, const protogen::FieldSet& fs)
    {
        setVar("fieldset.name", fs.name);
        fillPackage("fieldset.package", fs.pkg);
        fillFields(p, createLoop("field"), fs.fields);
    }


    void initForProtocol(const protogen::Parser& p, const std::string& protoName);

    void initForEnum(protogen::Parser& p, const std::string& enumName);

    void fillFields(protogen::Parser& p, Loop& ld, const protogen::FieldsVector& fields);

    void dumpContext()
    {
        printf("Current context vars dump:\n");
        for(auto& var : top->vars)
        {
            printf("%s='%s'\n", var.first.c_str(), var.second.c_str());
        }
    }
};

} // namespace protogen
