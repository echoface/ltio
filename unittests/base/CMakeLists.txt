
aux_source_directory(. BASE_UT_SOURCES)

ADD_EXECUTABLE(base_unittest
  ${BASE_UT_SOURCES}
  )
ltio_default_properties(base_unittest)
target_link_libraries(base_unittest
  PUBLIC ltio
  profiler
  )
