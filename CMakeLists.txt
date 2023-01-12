CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(cdt2cmake)

add_library(tinyxml STATIC
    tinyxml/tinyxmlparser.cpp
    tinyxml/tinystr.cpp
    tinyxml/tinyxml.cpp
    tinyxml/tinyxmlerror.cpp
)
target_compile_definitions(tinyxml PUBLIC TIXML_USE_STL=1)
target_include_directories(tinyxml PUBLIC tinyxml)

ADD_SUBDIRECTORY(src)
