ADD_DEFINITIONS(
  -DUSE_TCMALLOC
  -DUSENONSTD_STRING_VIEW
  -DNET_ENABLE_REUSER_PORT
)
SET(USE_TCMALLOC TRUE)
#SET(CMAKE_CXX_FLAGS "-Wall -fno-rtti")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2")
SET(CMAKE_CXX_FLAGS_DEBUG  "-O0 -g")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")


# ---[ Set debug postfix
set(LtIO_DEBUG_POSTFIX "-d")
set(LtIO_POSTFIX "")
IF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE RelWithDebInfo)
ENDIF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)


function(ltio_default_properties target)
  set_target_properties(${target} PROPERTIES
    DEBUG_POSTFIX ${LtIO_DEBUG_POSTFIX}
    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
  # make sure we build all external dependencies first
  if (DEFINED external_project_dependencies)
    add_dependencies(${target} ${external_project_dependencies})
  endif()
endfunction()
