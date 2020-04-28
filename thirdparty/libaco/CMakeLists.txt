cmake_minimum_required(VERSION 3.5)

ADD_DEFINITIONS(-fPIC)

project (libaco VERSION 0.0.1 LANGUAGES C ASM CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set_source_files_properties(acosw.S PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")

add_library(aco
  aco.c
  acosw.S
)
#set_target_properties(aco
#  PROPERTIES
#  COMPILE_DEFINITIONS "-no-m32"
#)
