cmake_minimum_required(VERSION 3.5)

SET(UNITTESTS_BINARY_DEPS
  ltnet
  ltbase
  3rd
  coro
  ${LtIO_LINKER_LIBS}
  )

if (LTIO_BUILD_UNITTESTS)
  ADD_SUBDIRECTORY(base)
  ADD_SUBDIRECTORY(net_io)
  ADD_SUBDIRECTORY(component)
endif()
