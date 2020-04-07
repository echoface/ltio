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

ADD_SUBDIRECTORY(unittests)

ADD_SUBDIRECTORY(examples)
