cmake_minimum_required(VERSION 3.5)

if (LTIO_BUILD_UNITTESTS)
  ADD_SUBDIRECTORY(base)
  ADD_SUBDIRECTORY(net_io)
  ADD_SUBDIRECTORY(component)
endif()
