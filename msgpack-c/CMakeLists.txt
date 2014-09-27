
include_directories(src)
include_directories(src/msgpack)

set(SRC_LIST src/gcc_atomic.cpp  src/objectc.c  src/object.cpp  src/unpack.c  src/version.c  src/vrefbuffer.c  src/zone.c 
	)
#link_directories(lib)
#link_directories(../../pjproject-2.3/pjlib/lib)
#link_libraries(pthread uuid pj-x86_64-unknown-linux-gnu)
add_library(msgpack ${SRC_LIST})
