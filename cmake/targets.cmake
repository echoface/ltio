
set(INSTALL_LIBS "")

add_library(ltio STATIC "")
#ltio_default_properties(ltio)
target_link_libraries(ltio PUBLIC ${LtIO_LINKER_LIBS})
target_compile_definitions(ltio PUBLIC ${EXPORT_CORO_COMPLILE_DEFINE})
target_include_directories(ltio PUBLIC
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/thirdparty
  )

list(APPEND INSTALL_LIBS ltio)

if (LTIO_BUILD_SHARED_LIBS)
  add_library(ltio_shared SHARED "")
  #ltio_default_properties(ltio_shared)
  target_compile_definitions(ltio_shared
      PUBLIC ${EXPORT_CORO_COMPLILE_DEFINE}
      )
  target_link_libraries(ltio_shared PUBLIC ${LtIO_LINKER_LIBS})
  target_include_directories(ltio_shared
      PUBLIC
      ${PROJECT_SOURCE_DIR}
      ${PROJECT_SOURCE_DIR}/thirdparty
      )
  set_target_properties(ltio PROPERTIES CLEAN_DIRECT_OUTPUT 1)
  set_target_properties(ltio_shared PROPERTIES OUTPUT_NAME "ltio")
  set_target_properties(ltio_shared PROPERTIES CLEAN_DIRECT_OUTPUT 1)

  list(APPEND INSTALL_LIBS ltio_shared)
endif(LTIO_BUILD_SHARED_LIBS)

#include(GNUInstallDirs)
#install(TARGETS ${INSTALL_LIBS}
#  EXPORT ltio-export
#  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
#)

ADD_SUBDIRECTORY(base)

ADD_SUBDIRECTORY(net_io)

ADD_SUBDIRECTORY(components)

ADD_SUBDIRECTORY(integration)

ADD_SUBDIRECTORY(thirdparty)

if (LTIO_BUILD_EXAMPLES)
  ADD_SUBDIRECTORY(examples)
endif()

