add_definitions (-DTEST)

add_executable (cppdataflowtest test.cpp)

include_directories(../include)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")
#install (TARGETS cppdataflowtest DESTINATION bin)