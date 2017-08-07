cmake_minimum_required(VERSION 3.1)
project (cppdataflow)

find_package(Boost)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest")

add_subdirectory (tests)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")