aux_source_directory(. COMPONENT_UT_SOURCES)

ADD_EXECUTABLE(component_unittest
  ${COMPONENT_UT_SOURCES}
  )

TARGET_LINK_LIBRARIES(component_unittest
  PUBLIC ltio
  )
