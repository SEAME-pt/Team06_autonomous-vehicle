# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/jetson/Team06_autonomous-vehicle/Middleware/build/_deps/googletest-src"
  "/home/jetson/Team06_autonomous-vehicle/Middleware/build/_deps/googletest-build"
  "/home/jetson/Team06_autonomous-vehicle/Middleware/build/_deps/googletest-subbuild/googletest-populate-prefix"
  "/home/jetson/Team06_autonomous-vehicle/Middleware/build/_deps/googletest-subbuild/googletest-populate-prefix/tmp"
  "/home/jetson/Team06_autonomous-vehicle/Middleware/build/_deps/googletest-subbuild/googletest-populate-prefix/src/googletest-populate-stamp"
  "/home/jetson/Team06_autonomous-vehicle/Middleware/build/_deps/googletest-subbuild/googletest-populate-prefix/src"
  "/home/jetson/Team06_autonomous-vehicle/Middleware/build/_deps/googletest-subbuild/googletest-populate-prefix/src/googletest-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/jetson/Team06_autonomous-vehicle/Middleware/build/_deps/googletest-subbuild/googletest-populate-prefix/src/googletest-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/jetson/Team06_autonomous-vehicle/Middleware/build/_deps/googletest-subbuild/googletest-populate-prefix/src/googletest-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
