cmake_minimum_required (VERSION 2.6)
project (guanli_server)
#add_definitions(-fpermissive -O2)

include_directories(../pjproject-2.3/pjlib/include)
add_definitions( -g)

set(SRC_LIST  main.cpp src/job.cpp src/singleton.cpp
	)
link_directories(lib)
link_directories(../pjproject-2.3/pjlib/lib)
link_libraries(pthread uuid pj-x86_64-unknown-linux-gnu)
add_executable(guanli ${SRC_LIST})
