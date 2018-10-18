cmake_minimum_required(VERSION 2.8)

SET(CMAKE_C_COMPILER g++)

ADD_DEFINITIONS(-std=c++1z)
ADD_DEFINITIONS(-fPIC)

SET(LTCOMPONENT_DEPENDENCY
  3rd
)

SET(COMPONENT_SOURCES
	./bloomfilter/bloom_filter.cc
	./count_min_sketch/count_min_sketch.cc

  ./source_loader/source.cc
	./source_loader/parser/parser.cc
	./source_loader/loader/loader_factory.cc
	./source_loader/loader/file_loader.cc
	./source_loader/parser/column_parser.cc
  ./source_loader/parser/column_parser.cc

  #inverted_indexer
  ./inverted_indexer/indexer.cc
  ./inverted_indexer/expression.cc
  ./inverted_indexer/bitmap_merger.cc
	./inverted_indexer/posting_field/field.cc
	./inverted_indexer/posting_field/general_str_field.cc
  ./inverted_indexer/posting_field/bitmap_posting_list.cc
	./inverted_indexer/posting_field/posting_list_manager.cc
)

SET(COMPONENT_LINKLIBS
	3rd
	ltbase
	${GLOG_LIBRARY})

link_libraries(
	${COMPONENT_LINKLIBS}
)

ADD_LIBRARY(ltcomponent STATIC
	${COMPONENT_SOURCES}
)

SET(COMPONENT_EXECUTE_LINKLIBS
	3rd
	ltbase
	ltcomponent
	${GLOG_LIBRARY})

link_libraries(
	${COMPONENT_EXECUTE_LINKLIBS}
)

# all follow target or library will dependency the ltcomponent
add_dependencies(
  ltcomponent
  ${LTCOMPONENT_DEPENDENCY}
)

ADD_EXECUTABLE(bloomfilter_unittest
  ./bloomfilter/bloom_filter_test.cc)
set_target_properties(bloomfilter_unittest PROPERTIES EXCLUDE_FROM_ALL TRUE)

ADD_EXECUTABLE(countminsketch_unittest
  ./count_min_sketch/count_min_sketch_unittests.cc)
set_target_properties(countminsketch_unittest PROPERTIES EXCLUDE_FROM_ALL TRUE)

ADD_EXECUTABLE(lrucache_unittest
  lru_cache/lru_cache_unittest.cc)
set_target_properties(lrucache_unittest PROPERTIES EXCLUDE_FROM_ALL TRUE)

ADD_EXECUTABLE(source_loader_unittest
	source_loader/unittest/source_loader_unittest.cc)
set_target_properties(source_loader_unittest PROPERTIES EXCLUDE_FROM_ALL TRUE)

ADD_EXECUTABLE(inverted_indexer_unittest
	./inverted_indexer/test/inverted_indexer_unittest.cc)
set_target_properties(inverted_indexer_unittest PROPERTIES EXCLUDE_FROM_ALL TRUE)