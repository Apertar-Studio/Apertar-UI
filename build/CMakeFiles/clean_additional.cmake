# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "ApertarUI_autogen"
  "CMakeFiles/ApertarUI_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/ApertarUI_autogen.dir/ParseCache.txt"
  )
endif()
