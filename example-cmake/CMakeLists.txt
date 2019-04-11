cmake_minimum_required(VERSION 3.0)

project("gen_test")

execute_process(
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/protogen
)

message("generator=${CMAKE_GENERATOR}")
execute_process(
  COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" 
    ${CMAKE_SOURCE_DIR}/protogen 
    -DCMAKE_BUILD_TYPE=Release 
    -DPROTOGEN_INSTALL_DIR=${CMAKE_BINARY_DIR}/protogen_bin
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/protogen
)

execute_process(
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/protogen
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/protogen
)

set(PROTOGEN_CMD ${CMAKE_BINARY_DIR}/protogen_bin/protogen)

execute_process(
  COMMAND ${PROTOGEN_CMD}
  ${PROJECT_SOURCE_DIR}/example.cgp --dryRun=true --printDeps=true "--printDepsDelimeter=;" --out.global.dir=${PROJECT_SOURCE_DIR}
  OUTPUT_VARIABLE DEPS_LIST)

execute_process(
  COMMAND ${PROTOGEN_CMD}
  ${PROJECT_SOURCE_DIR}/example.cgp --dryRun=true --printGen=true "--printGenDelimeter=;" --out.global.dir=${PROJECT_SOURCE_DIR}
  OUTPUT_VARIABLE GEN_LIST)

add_custom_command(
  OUTPUT ${GEN_LIST}
  COMMAND ${PROTOGEN_CMD} ${PROJECT_SOURCE_DIR}/example.cgp
  DEPENDS ${DEPS_LIST}
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )

add_executable(gen_test gen_test.cpp ${GEN_LIST})
