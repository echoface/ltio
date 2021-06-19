# cmake file for build glog alongside with gflags

if (UNIX)
  set(GLOG_EXTRA_COMPILER_FLAGS "-fPIC")
  set(GFLAGS_EXTRA_COMPILER_FLAGS "-fPIC")
endif()
set(GFLAGS_C_FLAGS ${CMAKE_C_FLAGS} ${GFLAGS_EXTRA_COMPILER_FLAGS})
set(GFLAGS_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${GFLAGS_EXTRA_COMPILER_FLAGS})

set(GLOG_C_FLAGS ${CMAKE_C_FLAGS} ${GLOG_EXTRA_COMPILER_FLAGS})
set(GLOG_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${GLOG_EXTRA_COMPILER_FLAGS})

# Build gflags as an external project.
set(GFLAGS_INSTALL_DIR ${CMAKE_BINARY_DIR}/gflags)
set(GFLAGS_INCLUDE_DIR ${GFLAGS_INSTALL_DIR}/include)
set(GFLAGS_LIB_DIR ${GFLAGS_INSTALL_DIR}/lib)
ExternalProject_Add(gflags_external_project
                    SOURCE_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/gflags
                    PREFIX      ${GFLAGS_INSTALL_DIR}
                    INSTALL_DIR ${GFLAGS_INSTALL_DIR}
                    CMAKE_ARGS  -DBUILD_SHARED_LIBS=ON
                                -DBUILD_gflags_LIB=ON
                                -DCMAKE_C_FLAGS=${GFLAGS_C_FLAGS}
                                -DCMAKE_CXX_FLAGS=${GFLAGS_CXX_FLAGS}
                                -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                -DCMAKE_INSTALL_PREFIX:PATH=${GFLAGS_INSTALL_DIR}
                   )
link_directories(${GFLAGS_LIB_DIR})
include_directories(BEFORE SYSTEM ${GFLAGS_INCLUDE_DIR})

set(GFLAGS_FOUND TRUE)
set(GFLAGS_LIBRARY_DIRS ${GFLAGS_INSTALL_DIR}/lib)
set(GFLAGS_INCLUDE_DIRS ${GFLAGS_INSTALL_DIR}/include)

set(GFLAGS_LIBRARY ${GFLAGS_INSTALL_DIR}/lib/libgflags.so)
set(GFLAGS_LIBRARIES ${GFLAGS_INSTALL_DIR}/lib/libgflags.so ${CMAKE_THREAD_LIBS_INIT})

add_library(gflags::gflags SHARED IMPORTED GLOBAL)
set_target_properties(gflags::gflags
  PROPERTIES
  IMPORTED_LOCATION ${GFLAGS_LIBRARY}
  INTERFACE_LINK_LIBRARIES "${GFLAGS_LIBRARIES}"
  )

# Build glog as an external project.
set(GLOG_INSTALL_DIR ${CMAKE_BINARY_DIR}/glog)
set(GLOG_INCLUDE_DIR ${GLOG_INSTALL_DIR}/include)
set(GLOG_LIB_DIR ${GLOG_INSTALL_DIR}/lib)
ExternalProject_Add(glog_external_project
                    DEPENDS     gflags_external_project
                    SOURCE_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glog
                    PREFIX      ${GLOG_INSTALL_DIR}
                    INSTALL_DIR ${GLOG_INSTALL_DIR}
                    CMAKE_ARGS  -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                -DCMAKE_C_FLAGS=${GLOG_C_FLAGS}
                                -DCMAKE_CXX_FLAGS=${GLOG_CXX_FLAGS}
                                -DCMAKE_INSTALL_PREFIX:PATH=${GLOG_INSTALL_DIR}
                                -DBUILD_SHARED_LIBS=ON
                   )

link_directories(${GLOG_LIB_DIR})
include_directories(BEFORE SYSTEM ${GLOG_INCLUDE_DIR})

set(GLOG_FOUND TRUE)
set(GLOG_LIBRARY_DIRS ${GLOG_INSTALL_DIR}/lib)
set(GLOG_INCLUDE_DIRS ${GLOG_INSTALL_DIR}/include)
set(GLOG_LIBRARY ${GLOG_INSTALL_DIR}/lib/libglog.so)
set(GLOG_LIBRARIES ${GLOG_INSTALL_DIR}/lib/libglog.so)

add_library(glog::glog SHARED IMPORTED GLOBAL)
set_target_properties(glog::glog
  PROPERTIES
  IMPORTED_LOCATION ${GLOG_LIBRARIES}
  INTERFACE_LINK_LIBRARIES ${GLOG_LIBRARIES}
  )

list(APPEND external_project_dependencies glog_external_project)
list(APPEND external_project_dependencies gflags_external_project)
