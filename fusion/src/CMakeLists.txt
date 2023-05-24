add_library(fusionlib SHARED
	fuseLibraryPass.cpp
	BBAnalysis.cpp
	BBManipulation.cpp
  BBVisualization.cpp
  FuseSupport.cpp
  types/FusedBB.cpp)

add_library(instrumentlib SHARED
  instrumentLibraryPass.cpp)

add_library(callgraphlib SHARED
  callgraphLibraryPass.cpp)

add_library(proflib SHARED
  profileLibraryPass.cpp)

add_library(reuselib SHARED
  reuseLibraryPass.cpp
  BBAnalysis.cpp
  types/FusedFunc.cpp)

add_library(memAnalysis SHARED
  memoryAnalysisPass.cpp)

#This is wrong but will work for now

add_subdirectory(arch)

target_link_libraries(fusionlib gvc cgraph cdt)
target_link_libraries(proflib LLVMCore LLVMBitWriter)
