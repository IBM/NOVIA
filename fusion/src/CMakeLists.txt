add_library(fusionlib SHARED
	fuseLibraryPass.cpp
	BBAnalysis.cpp
	BBManipulation.cpp
  BBVisualization.cpp)
 
target_link_libraries(fusionlib gvc cgraph cdt)
