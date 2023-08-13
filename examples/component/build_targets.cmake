

ADD_EXECUTABLE(log_metrics_test
  component/log_metrics_test.cc
)

TARGET_LINK_LIBRARIES(log_metrics_test
  ltio
)

ADD_EXECUTABLE(be_index_bench
  component/be_index_bench.cc
  ${PROJECT_SOURCE_DIR}/components/boolean_indexer/testing_helper.cc
)

TARGET_LINK_LIBRARIES(be_index_bench
  ltio
  profiler
)

