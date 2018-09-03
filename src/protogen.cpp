#include <set>
#include <string>
#include <stdio.h>

#include "Project.hpp"

const char* sccs_version = "@(#) protogen 1.8.0 " __DATE__;

int main(int argc, char* argv[])
{
    using namespace protogen;
    try
    {
        if(argc == 1)
        {
            printf("Usage: protogen [--options] projectfile [--options]\n");
            return EXIT_SUCCESS;
        }
        std::string projectFileName;
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
                return EXIT_FAILURE;
            }
            projectFileName = argv[i];
        }
        if(!projectFileName.length())
        {
            printf("Expected project file name in command line\n");
            return EXIT_FAILURE;
        }

        Project prj;

        {
            const char* envsp = getenv("PROTOGEN_SEARCH_PATH");
            if(envsp)
            {
                std::string sp = envsp;
                if(!sp.empty() && sp.back() != '/' && sp.back()!='\\')
                {
                    sp += '/';
                }
                prj.addSearchPath(std::move(sp));
            }
        }
        prj.setOutputFunc([](const char* msg){
           printf("%s",msg);
        });
        if(!prj.load(projectFileName, optionsOverride))
        {
            printf("Failed to load project: '%s'\n", projectFileName.c_str());
            return EXIT_FAILURE;
        }
        if(!prj.generate())
        {
            printf("Failed to generate project: '%s'\n", projectFileName.c_str());
            return EXIT_FAILURE;
        }
    }
    catch(std::exception& e)
    {
        printf("Exception:\"%s\"\n", e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
