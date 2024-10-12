# Minimal CMake version 
cmake_minimum_required(VERSION 3.10)

# Project name and version
project(GameOfLife VERSION 1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -s -DNDEBUG -fno-tree-vectorize -fopenmp -msse")

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/main.cpp
    src/GameOfLife.cpp
    src/CLI.cpp
)

# Executable
add_executable(GameOfLife ${SOURCES})

# Include OpenCL directories
include_directories(/usr/include/gegl-0.4)

# Link OpenCL libraries
target_link_libraries(GameOfLife /usr/lib64 /usr/lib64/libOpenCL.so.1)

# Copy the kernel file to the build directory
configure_file(${CMAKE_SOURCE_DIR}/resources/evolution.cl ${CMAKE_BINARY_DIR}/evolution.cl COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/resources/comparison.cl ${CMAKE_BINARY_DIR}/comparison.cl COPYONLY)

# Add a custom target to run the executable
add_custom_target(run
    COMMAND GameOfLife
    DEPENDS GameOfLife
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running the Game of Life simulation"
)

# Add a custom clean target to remove all generated files
add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_BINARY_DIR}/*
    COMMENT "Cleaning all files generated in the build directory"
)