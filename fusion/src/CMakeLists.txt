add_library(fusionlib SHARED
	fuseLibraryPass.cpp
	BBAnalysis.cpp
	BBManipulation.cpp
  BBVisualization.cpp
  FuseSupport.cpp
  types/FusedBB.cpp)

add_library(instrumentlib SHARED
  instrumentLibraryPass.cpp)

#This is wrong but will work for now

add_subdirectory(arch)

target_link_libraries(fusionlib gvc cgraph cdt)
