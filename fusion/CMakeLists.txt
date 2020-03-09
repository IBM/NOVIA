cmake_minimum_required(VERSION 3.10)

project(fusion VERSION 1.0 DESCRIPTION "LLVM fusion passes")

#set(LLVM_ROOT "${CMAKE_CURRENT_SOURCE_DIR}../../llvm-project/build/" CACHE PATH "Root of LLVM install.")
set(CMAKE_PREFIX_PATH "$ENV{LLVM_ROOT}/bin/")

if( NOT EXISTS $ENV{LLVM_ROOT}/include/llvm )
	message(FATAL_ERROR "LLVM_ROOT ($ENV{LLVM_ROOT}) is not a valid LLVM install.
	Did you source env.sh?")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "$ENV{LLVM_ROOT}/lib/cmake/llvm/")
include(LLVMConfig)
include(AddLLVM)

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

add_compile_options(-fno-rtti)
add_compile_options(-ggdb)


add_subdirectory(src)
