include_directories(../../pjproject-2.3/pjlib/include)
add_definitions( -g)

set(SRC_LIST  main.cpp job.cpp singleton.cpp
	msgjob.cpp opus_codec.cpp conference.cpp
	)
link_directories(lib)
link_directories(../../../pjproject-2.3/pjlib/lib)
link_libraries(pthread uuid opus msgpack pj-x86_64-unknown-linux-gnu)
add_executable(conference ${SRC_LIST})
