# CMake required version
cmake_minimum_required (VERSION 3.5)

# project information
project(xml_parser)

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_C_FLAGS_DEBUG "$ENV{CFLAGS} -O0 -Wall -g -ggdb")

#set(CMAKE_BUILD_TYPE "Release")
#set(CMAKE_C_FLAGS_RELEASE "$ENV{CFLAGS} -O3 -Wall")

option(XMLPARSER_WITH_TESTS "Build tests" ON) 

set(SRCS
     xml.c
     xml_parser.c
     xml_lexer.c
     xpath.c
   )
add_executable(xml_parser ${SRCS})
target_compile_definitions( xml_parser PRIVATE LEX_DEBUG DEBUG) # new way
#add_definitions(-DLEX_DEBUG -DDEBUG) # old way

#######################################################
#                      TEST
#######################################################
# copy all *.xml to build directory
# SIMPLE WAY:
#file(GLOB XML_FILES "*.xml")
#file(COPY ${XML_FILES} DESTINATION ${CMAKE_BINARY_DIR})

# BETTER WAY:
file(GLOB XML_FILES "*.xml")
foreach(XmlFile IN LISTS XML_FILES)
  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${XmlFile}
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMENT "Copying xml file: ${XmlFile}")
endforeach()

if (XMLPARSER_WITH_TESTS)
  enable_testing()
  add_test(NAME TEST_DOT_XML COMMAND xml_parser ./test.xml)
  add_test(NAME TEST2_DOT_XML COMMAND xml_parser ./test2.xml)
  add_test(NAME TEST3_DOT_XML COMMAND xml_parser ./test3.xml)
  add_test(NAME CDATA_DOT_XML COMMAND xml_parser ./cdata.xml)
  add_test(NAME DOCTYPE_DOT_XML COMMAND xml_parser ./doctype.xml)
  add_test(NAME SIMPLE_DOT_XML COMMAND xml_parser ./simple.xml)
endif()


