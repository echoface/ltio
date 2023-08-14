SET(COMPONENT_SOURCES
  ./log_metrics/metrics_item.cc
  ./log_metrics/metrics_stash.cc
  ./log_metrics/metrics_container.cc

  ./count_min_sketch/count_min_sketch.cc

  ./bloomfilter/bloomfilter.cc

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

  #boolean indexer
  ./boolean_indexer/document.cc
  ./boolean_indexer/be_indexer.cc
  ./boolean_indexer/id_generator.cc
  ./boolean_indexer/posting_list.cc
  ./boolean_indexer/common_container.cc
  ./boolean_indexer/index_scanner.cc
  ./boolean_indexer/scanner_cursor.cc
  ./boolean_indexer/parser/number_parser.cc
  ./boolean_indexer/builder/be_indexer_builder.cc
)

target_sources(ltio
    PRIVATE
    ${COMPONENT_SOURCES}
    )
target_link_libraries(ltio
    PUBLIC
    ${LtIO_LINKER_LIBS}
    )
#target_sources(ltio_shared
#    PRIVATE
#    ${COMPONENT_SOURCES}
#    )
#target_link_libraries(ltio_shared
#    PUBLIC
#    ${LtIO_LINKER_LIBS}
#    )

SET(COMPONENT_UT_SOURCES
    inverted_indexer/indexer_unittest.cc

    utils/data_accumulator_unittest.cc

    lru_cache/lru_cache_unittest.cc

    source_loader/source_loader_unittest.cc

    boolean_indexer/testing_helper.cc
    boolean_indexer/be_indexer_unittest.cc

    bloomfilter/bloomfilter_unittest.cc
    )
add_executable(ut_component
    ${COMPONENT_UT_SOURCES}
    )
target_link_libraries(ut_component
    PRIVATE
    ltio
    profiler
    Catch2::Catch2WithMain
    )

set(copy_dest_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(copy_src_file "${CMAKE_CURRENT_SOURCE_DIR}/boolean_indexer/testing_docs.txt")
add_custom_command(TARGET ut_component POST_BUILD
    COMMENT "Copying ${copy_src_file} to build directory"
    COMMAND ${CMAKE_COMMAND} -E copy "${copy_src_file}" "${copy_dest_dir}"
    )
