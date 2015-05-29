include_directories(".")

set( PROTOGEN_SRC
Format.cpp
Parser.cpp
Template.cpp
protogen.cpp
)

add_executable(protogen ${PROTOGEN_SRC})
