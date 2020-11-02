
#aux_source_directory(. COMPONENT_UT_SOURCES)

ADD_EXECUTABLE(component_unittest 
  ./catch_main.cc
  ./boolean_indexer_unittest.cc
)

TARGET_LINK_LIBRARIES(component_unittest
  PUBLIC ltio
)
