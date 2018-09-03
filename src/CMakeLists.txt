cmake_minimum_required(VERSION 3.0)
project(protogen)
include_directories(".")

set( PROTOGEN_SRC
  Format.cpp
  Parser.cpp
  Template.cpp
  protogen.cpp
  Project.cpp
  TemplateDataSource.cpp
  Utility.cpp)

add_executable(protogen ${PROTOGEN_SRC})

if(MSVC)
  target_compile_definitions(protogen PRIVATE -D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE)
endif()
