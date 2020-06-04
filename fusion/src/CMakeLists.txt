add_library(fusionlib SHARED
	fuseLibraryPass.cpp
	BBAnalysis.cpp
	BBManipulation.cpp
  BBVisualization.cpp
  FuseSupport.cpp
  types/FusedBB.cpp)

#This is wrong but will work for now

#add_subdirectory(types)

target_link_libraries(fusionlib gvc cgraph cdt)
