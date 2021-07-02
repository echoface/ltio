ADD_DEFINITIONS(
  -DUSE_TCMALLOC
  -DUSENONSTD_STRING_VIEW
  -DNET_ENABLE_REUSER_PORT
)
if (LTIO_WITH_OPENSSL)
ADD_DEFINITIONS(
  -DLTIO_HAVE_SSL
  -DLTIO_WITH_OPENSSL
)
endif(LTIO_WITH_OPENSSL)

#SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -fsanitize=address")
#SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -fsanitize=address")
#SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -fsanitize=address")
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

# These lists are later turned into target properties on main LtIO library target
set(LtIO_LINKER_LIBS "")
set(LtIO_INCLUDE_DIRS "")
set(LtIO_COMPILE_OPTIONS "")

SET(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (LTIO_USE_ACO_CORO_IMPL)
  SET(CORO_LIBRARY aco)
  ADD_DEFINITIONS(-DUSE_LIBACO_CORO_IMPL)
  SET(EXPORT_CORO_COMPLILE_DEFINE "-DUSE_LIBACO_CORO_IMP")
else()
  SET(CORO_LIBRARY coro)
  SET(EXPORT_CORO_COMPLILE_DEFINE "")
endif()
MESSAGE(STATUS "Using Coroutine library (${CORO_LIBRARY})")

# ---[ Set debug postfix
IF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE RelWithDebInfo)
ENDIF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)

if (NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
endif()
if (NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
endif()
if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
endif()

function(ltio_default_properties target)
  set_target_properties(${target} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    )
  # make sure we build all external dependencies first
  if (DEFINED external_project_dependencies)
    add_dependencies(${target} ${external_project_dependencies})
  endif()
endfunction()
