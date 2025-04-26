# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name
project(TetrisTrainCpp VERSION 1.0 LANGUAGES CXX)

# Set C++ standard to C++17 (required for std::optional, etc.)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Optional: Disable compiler-specific extensions

# Add compiler warnings (optional but recommended)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Define the executable target name
set(EXECUTABLE_NAME tetris_test)

# List all your source files
set(SOURCE_FILES
    main.cpp
    models.cpp
    constants.cpp
    extractor.cpp
    game.cpp
    visualize.cpp
)

# Add the executable target
add_executable(${EXECUTABLE_NAME} ${SOURCE_FILES})

# Specify include directories (current directory for headers)
target_include_directories(${EXECUTABLE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Optional: Add optimization flags for release builds
# target_compile_options(${EXECUTABLE_NAME} PRIVATE $<$<CONFIG:Release>:-O3>)

# Optional: Add debug flags for debug builds
# target_compile_options(${EXECUTABLE_NAME} PRIVATE $<$<CONFIG:Debug>:-g>)

# Print a message after configuration
message(STATUS "Configured ${PROJECT_NAME} version ${PROJECT_VERSION}")
message(STATUS "Executable target: ${EXECUTABLE_NAME}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}") # Will be empty if not specified during configure step