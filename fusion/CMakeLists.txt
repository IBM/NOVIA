cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0076 NEW)

project(fusion VERSION 1.0 DESCRIPTION "LLVM fusion passes")

#set(LLVM_ROOT "${CMAKE_CURRENT_SOURCE_DIR}../../llvm-project/build/" CACHE PATH "Root of LLVM install.")
set(CMAKE_PREFIX_PATH "$ENV{LLVM_ROOT}/bin/")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_CXX_STANDARD 17)

if( NOT EXISTS $ENV{LLVM_ROOT}/include/llvm )
	message(FATAL_ERROR "LLVM_ROOT ($ENV{LLVM_ROOT}) is not a valid LLVM install.
	Did you source env.sh?")
endif()

if( NOT EXISTS $ENV{GRAPHVIZ_ROOT}/include )
  message(FATAL_ERROR "GRAPHVIZ_ROOT ($ENV{GRAPHVIZ_ROOT}) is not a valid GRAPHVIZ install.
	Did you source env.sh?")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "$ENV{LLVM_ROOT}/lib/cmake/llvm/")
include(LLVMConfig)
include(AddLLVM)

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

include_directories($ENV{GRAPHVIZ_ROOT}/include/graphviz/)
link_directories($ENV{GRAPHVIZ_ROOT}/lib/)

add_compile_options(-fno-rtti)
add_compile_options(-ggdb)
#add_compile_options(-O3)
add_compile_options(-fPIC)

add_subdirectory(src)
