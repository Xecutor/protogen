cmake_minimum_required(VERSION 3.0)
project(protogen)

set(CMAKE_CXX_STANDARD 14)

set(PROTOGEN_SRC
    Format.cpp
    Parser.cpp
    Template.cpp
    protogen.cpp
    Project.cpp
    TemplateDataSource.cpp
    Utility.cpp)

add_executable(protogen ${PROTOGEN_SRC})

target_include_directories(protogen PRIVATE ".")

if(MSVC)
    target_compile_definitions(protogen PRIVATE -D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE)
endif()

if(PROTOGEN_INSTALL_DIR)
    install(
        TARGETS protogen
        CONFIGURATIONS Debug Release
        RUNTIME DESTINATION ${PROTOGEN_INSTALL_DIR})
endif()
