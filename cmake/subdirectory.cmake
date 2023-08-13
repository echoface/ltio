# top level include dir

# common include directory
include_directories(
  ${LtIO_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}/
  ${PROJECT_SOURCE_DIR}/thirdparty
)
ADD_SUBDIRECTORY(base)

ADD_SUBDIRECTORY(net_io)

ADD_SUBDIRECTORY(components)

ADD_SUBDIRECTORY(integration)

ADD_SUBDIRECTORY(thirdparty)

if (LTIO_BUILD_EXAMPLES)
  ADD_SUBDIRECTORY(examples)
endif()

