
aux_source_directory(. COMPONENT_UT_SOURCES)

ADD_EXECUTABLE(component_unittest ${COMPONENT_UT_SOURCES})
ltio_default_properties(component_unittest)

TARGET_LINK_LIBRARIES(component_unittest
  PUBLIC ltio
)
