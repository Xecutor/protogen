cmake_minimum_required(VERSION 3.0)

project(protogen)

include(ExternalProject)
ExternalProject_Add(Protogen
  GIT_REPOSITORY https://github.com/Xecutor/protogen.git
  SOURCE_SUBDIR src
  STEP_TARGETS build
  CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DPROTOGEN_INSTALL_DIR=${PROTOGEN_INSTALL_DIR}
)
