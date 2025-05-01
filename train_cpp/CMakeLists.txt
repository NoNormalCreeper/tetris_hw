# Minimum CMake version required
cmake_minimum_required(VERSION 3.11) # FetchContent needs at least 3.11

# Project name
project(TetrisTrainCpp VERSION 1.0 LANGUAGES CXX)

# Set C++ standard to C++17 (required for std::optional, std::async etc.)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Optional: Disable compiler-specific extensions

# Find Threads package (needed for std::thread, std::async, std::mutex)
find_package(Threads REQUIRED)

# --- Fetch and configure spdlog using FetchContent ---
include(FetchContent)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1 # Or specify another tag/commit
)
# Optional: Set spdlog options if needed, *before* FetchContent_MakeAvailable
# set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "Build spdlog as static lib" FORCE) # Example: force static lib
# set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "Use external fmt" FORCE) # If you provide fmt separately

FetchContent_MakeAvailable(spdlog)
# --- End spdlog FetchContent ---

# Add compiler warnings (optional but recommended)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# --- Target for the original test executable ---
set(TEST_EXECUTABLE_NAME tetris_test)
set(TEST_SOURCE_FILES
    main.cpp          # Original main for testing/visualization
    models.cpp
    constants.cpp
    extractor.cpp
    game.cpp
    visualize.cpp
)
add_executable(${TEST_EXECUTABLE_NAME} ${TEST_SOURCE_FILES})
target_include_directories(${TEST_EXECUTABLE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# Link threads if main.cpp ever uses threading features (optional for current main.cpp)
# target_link_libraries(${TEST_EXECUTABLE_NAME} PRIVATE Threads::Threads)

# --- Target for the training executable ---
set(TRAIN_EXECUTABLE_NAME tetris_train)
set(TRAIN_SOURCE_FILES
    training.cpp      # The training logic implementation
    game.cpp          # Dependency of training.cpp (calls runGameForTraining)
    models.cpp        # Dependency of game.cpp and others
    constants.cpp     # Dependency of game.cpp and others
    extractor.cpp     # Dependency of game.cpp
    # visualize.cpp is likely NOT needed for training logic itself
)
add_executable(${TRAIN_EXECUTABLE_NAME} ${TRAIN_SOURCE_FILES})
target_include_directories(${TRAIN_EXECUTABLE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# Link Threads library required by training.cpp
# Link spdlog library (header-only if SPDLOG_HEADER_ONLY is ON, otherwise links the compiled lib)
target_link_libraries(${TRAIN_EXECUTABLE_NAME} PRIVATE Threads::Threads spdlog::spdlog)


# Optional: Add optimization flags for release builds
target_compile_options(${TEST_EXECUTABLE_NAME} PRIVATE $<$<CONFIG:Release>:-O3>)
target_compile_options(${TRAIN_EXECUTABLE_NAME} PRIVATE $<$<CONFIG:Release>:-O3>)

# Optional: Add debug flags for debug builds
target_compile_options(${TEST_EXECUTABLE_NAME} PRIVATE $<$<CONFIG:Debug>:-g>)
target_compile_options(${TRAIN_EXECUTABLE_NAME} PRIVATE $<$<CONFIG:Debug>:-g>)

# Print a message after configuration
message(STATUS "Configured ${PROJECT_NAME} version ${PROJECT_VERSION}")
message(STATUS "Test Executable target: ${TEST_EXECUTABLE_NAME}")
message(STATUS "Train Executable target: ${TRAIN_EXECUTABLE_NAME}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}") # Will be empty if not specified during configure step