cmake_minimum_required (VERSION 2.6)
project (guanli_server)
add_definitions(-O2)
include_directories(pjproject-2.3/pjlib/include)
include_directories(opus-1.1/include)

add_subdirectory (msgpack-c  lib)
add_subdirectory (opus-1.1  lib)
add_subdirectory (src  bin)
#include_directories(../pjproject-2.3/pjlib/include)
#add_definitions( -g)

#set(SRC_LIST  src/main.cpp src/job.cpp src/singleton.cpp
#	src/msgjob.cpp
#	)
#link_directories(lib)
#link_directories(../../pjproject-2.3/pjlib/lib)
#link_libraries(pthread uuid pj-x86_64-unknown-linux-gnu)
#add_executable(conference ${SRC_LIST})
