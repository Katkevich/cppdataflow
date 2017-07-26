cmake_minimum_required(VERSION 3.1)
project (cppdataflow)

find_package(Boost)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest")

add_subdirectory (tests)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")
#SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
#SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
#SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
#set_target_properties(cppdataflowtest
#    PROPERTIES
#    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/"
#    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/"
#    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/"
#)

#install (TARGETS cppdataflowtest DESTINATION bin)