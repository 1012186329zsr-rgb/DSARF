cmake_minimum_required(VERSION 3.5)

project(topology LANGUAGES C)

# Use C11 (project is a C codebase)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Basic compile flags
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall")

# Find OpenMP and apply correctly for C
find_package(OpenMP)
if(OpenMP_C_FOUND)
    message(STATUS "Found OpenMP for C: ${OpenMP_C_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(PRJ_LIBS OpenMP::OpenMP_C)
else()
    message(STATUS "OpenMP not found - building without OpenMP")
    set(PRJ_LIBS "")
endif()

# Collect C sources in src/
file(GLOB SOURCES "${PROJECT_SOURCE_DIR}/src/*.c")

add_executable(topology ${SOURCES})

# Include headers
target_include_directories(topology PRIVATE ${PROJECT_SOURCE_DIR}/include)

# Link libraries (OpenMP if available)
if(TARGET OpenMP::OpenMP_C)
    target_link_libraries(topology PRIVATE OpenMP::OpenMP_C)
elseif(PRJ_LIBS)
    target_link_libraries(topology PRIVATE ${PRJ_LIBS})
endif()

# Link libm for math functions (exp, pow, etc.)
target_link_libraries(topology PRIVATE m)

# Recommended: export compile commands for tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
