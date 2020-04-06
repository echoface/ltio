# These lists are later turned into target properties on main LtIO library target
set(LtIO_LINKER_LIBS "")
set(LtIO_INCLUDE_DIRS "")
set(LtIO_DEFINITIONS "")
set(LtIO_COMPILE_OPTIONS "")

find_package(Tcmalloc REQUIRED)
list(APPEND LtIO_LINKER_LIBS PUBLIC ${Tcmalloc_LIBRARY})

# ---[ Google-glog
include("cmake/External/glog.cmake")
list(APPEND LtIO_INCLUDE_DIRS PUBLIC ${GLOG_INCLUDE_DIRS})
list(APPEND LtIO_LINKER_LIBS PUBLIC ${GLOG_LIBRARIES})

# ---[ Google-gflags
include("cmake/External/gflags.cmake")
list(APPEND LtIO_INCLUDE_DIRS PUBLIC ${GFLAGS_INCLUDE_DIRS})
list(APPEND LtIO_LINKER_LIBS PUBLIC ${GFLAGS_LIBRARIES})

# ---[ LevelDB
#if(USE_LEVELDB)
#  find_package(LevelDB REQUIRED)
#  list(APPEND LtIO_INCLUDE_DIRS PUBLIC ${LevelDB_INCLUDES})
#  list(APPEND LtIO_LINKER_LIBS PUBLIC ${LevelDB_LIBRARIES})
#  list(APPEND LtIO_DEFINITIONS PUBLIC -DUSE_LEVELDB)
#endif()

find_package(MYSQL  REQUIRED)
list(APPEND LtIO_LINKER_LIBS PUBLIC ${MYSQL_LIBRARIES})


find_package(Unwind REQUIRED)
list(APPEND LtIO_LINKER_LIBS PUBLIC ${LIBUNWIND_LIBRARIES})

find_package(ZLIB REQUIRED)
list(APPEND LtIO_LINKER_LIBS PUBLIC ${ZLIB_LIBRARIES})

# ---[ Threads
find_package(Threads REQUIRED)
list(APPEND LtIO_LINKER_LIBS PRIVATE ${CMAKE_THREAD_LIBS_INIT})

#list(REMOVE_DUPLICATES LtIO_LINKER_LIBS)
list(REMOVE_DUPLICATES LtIO_INCLUDE_DIRS)

# ---[ ccache
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif(CCACHE_FOUND)

