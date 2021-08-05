cmake_minimum_required (VERSION 3.12)

project ("GraphiT" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE "Release" CACHE PATH "Build Type" FORCE)
endif()

find_package(Vulkan)
if(NOT Vulkan_FOUND)
    message("")
    message("-- Vulkan not found! Vulkan context is disabled")
    message("")
else()
    message("-- Vulkan context enabled")
endif()

set(LINK_LIBS
    ${Vulkan_LIBRARIES}
)
set(INC_DIRS
    "${PROJECT_SOURCE_DIR}/include"
    ${Vulkan_INCLUDE_DIRS}
)

# Used to inline kernel/shaders.
add_executable(bin2c
    "bin2c.cpp"
    "include/assert.hpp"
    "src/assert.cpp"
    "include/util.hpp"
    "src/util.cpp")

# The testbench library.
include_directories(${INC_DIRS})
file(GLOB SRCS "${PROJECT_SOURCE_DIR}/src/*")
file(GLOB INCS "${PROJECT_SOURCE_DIR}/include/*")
add_library(GraphiT STATIC ${SRCS} ${INCS})
target_link_libraries(GraphiT ${LINK_LIBS})
add_dependencies(GraphiT bin2c)

# Testbench apps.
make_directory("${CMAKE_BINARY_DIR}/assets/")
add_subdirectory("${PROJECT_SOURCE_DIR}/apps")
