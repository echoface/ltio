
aux_source_directory(. COMPONENT_UT_SOURCES)

ADD_EXECUTABLE(component_unittest
  ${COMPONENT_UT_SOURCES}
  )
ltio_default_properties(component_unittest)
#set_target_properties(component_unittest PROPERTIES EXCLUDE_FROM_ALL TRUE)

TARGET_LINK_LIBRARIES(component_unittest
  ${LtIO_LINKER_LIBS}
  PRIVATE ltnet
  PRIVATE ltbase
  PRIVATE ltcomponent
  )
