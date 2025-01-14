# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/ClusterDisplay_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/ClusterDisplay_autogen.dir/ParseCache.txt"
  "ClusterDisplay_autogen"
  )
endif()
