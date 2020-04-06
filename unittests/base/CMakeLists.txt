
aux_source_directory(. BASE_UT_SOURCES)

ADD_EXECUTABLE(base_unittest
  ${BASE_UT_SOURCES}
)
ltio_default_properties(base_unittest)

TARGET_LINK_LIBRARIES(base_unittest
  PUBLIC ${UNITTESTS_BINARY_DEPS}
)
