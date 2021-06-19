aux_source_directory(. COMPONENT_UT_SOURCES)

ADD_EXECUTABLE(component_unittest
  ${COMPONENT_UT_SOURCES}
)

TARGET_LINK_LIBRARIES(component_unittest
  ltio
)

set(copy_dest_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(copy_src_file "${CMAKE_CURRENT_SOURCE_DIR}/test_documents.txt")
add_custom_command(
    TARGET component_unittest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "${copy_src_file}" "${copy_dest_dir}"
    COMMENT "Copying ${copy_src_file} to build directory"
)
