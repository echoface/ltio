
set(INSTALL_LIBS "")

add_library(ltio STATIC
  $<TARGET_OBJECTS:ltnet_objs>
  $<TARGET_OBJECTS:ltbase_objs>
  $<TARGET_OBJECTS:ltcomponent_objs>
  )
ltio_default_properties(ltio)

list(APPEND INSTALL_LIBS ltio)

target_include_directories(ltio PUBLIC
  ${LtIO_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/thirdparty
    ${PROJECT_SOURCE_DIR}/integration
  )

if (LTIO_BUILD_SHARED_LIBS)
  add_library(ltio_shared SHARED
    $<TARGET_OBJECTS:ltnet_objs>
    $<TARGET_OBJECTS:ltbase_objs>
    $<TARGET_OBJECTS:ltcomponent_objs>
    )
  ltio_default_properties(ltio_shared)
  target_include_directories(ltio_shared PUBLIC
    ${LtIO_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/thirdparty
    ${PROJECT_SOURCE_DIR}/integration
    )

  set_target_properties(ltio PROPERTIES CLEAN_DIRECT_OUTPUT 1)
  set_target_properties(ltio_shared PROPERTIES OUTPUT_NAME "ltio")
  set_target_properties(ltio_shared PROPERTIES CLEAN_DIRECT_OUTPUT 1)

  list(APPEND INSTALL_LIBS ltio_shared)
endif(LTIO_BUILD_SHARED_LIBS)

include(GNUInstallDirs)
install(TARGETS ${INSTALL_LIBS}
  EXPORT ltio-export
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

