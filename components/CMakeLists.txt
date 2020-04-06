cmake_minimum_required(VERSION 2.8)

ADD_DEFINITIONS(-std=c++11)
SET(CMAKE_POSITION_INDEPENDENT_CODE ON)

aux_source_directory(logmetrics LOGMETRICS_SOURCES)
aux_source_directory(bloomfilter BLOOMFILTER_SOURCES)
aux_source_directory(count_min_sketch COUNTMINSKETCH_SOURCES)
aux_source_directory(source_loader SOURCELOADER_SOURCES)

SET(COMPONENT_SOURCES
  ${LOGMETRICS_SOURCES}
  ${BLOOMFILTER_SOURCES}
  ${COUNTMINSKETCH_SOURCES}

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
  ./inverted_indexer/posting_field/range_field.cc
  ./inverted_indexer/posting_field/general_str_field.cc
  ./inverted_indexer/posting_field/bitmap_posting_list.cc
  ./inverted_indexer/posting_field/posting_list_manager.cc
)

ADD_LIBRARY(ltcomponent STATIC ${COMPONENT_SOURCES})
ltio_default_properties(ltcomponent)

ADD_LIBRARY(ltcomponent_shared SHARED ${COMPONENT_SOURCES})
ltio_default_properties(ltcomponent_shared)
set_target_properties(ltcomponent_shared PROPERTIES OUTPUT_NAME "ltcomponent")

set_target_properties(ltcomponent PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(ltcomponent_shared PROPERTIES CLEAN_DIRECT_OUTPUT 1)
