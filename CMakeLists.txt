# CMake required version
cmake_minimum_required (VERSION 3.5)

# project information
project(xml_parser)

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_C_FLAGS_DEBUG "$ENV{CFLAGS} -O0 -Wall -g -ggdb")

#set(CMAKE_BUILD_TYPE "Release")
#set(CMAKE_C_FLAGS_RELEASE "$ENV{CFLAGS} -O3 -Wall")

set(SRCS
     xml.c
     xml_parser.c
     xml_lexer.c
     xpath.c
   )
add_executable(xml_parser ${SRCS})
target_compile_definitions( xml_parser PRIVATE LEX_DEBUG=1 )
target_compile_definitions( xml_parser PRIVATE DEBUG=1 )
