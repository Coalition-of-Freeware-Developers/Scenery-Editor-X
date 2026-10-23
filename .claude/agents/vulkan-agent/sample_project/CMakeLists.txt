cmake_minimum_required(VERSION 3.16)
project(vulkan_sample_project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Allow user to pass Vulkan SDK path via VULKAN_SDK or rely on find_package
if(NOT DEFINED ENV{VULKAN_SDK})
  message(STATUS "VULKAN_SDK not set. find_package(Vulkan) will be used.")
endif()

find_package(Vulkan REQUIRED)

add_executable(vulkan_sample src/main.cpp)
target_include_directories(vulkan_sample PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(vulkan_sample PRIVATE ${Vulkan_LIBRARIES})

# Debug: define macro to enable validation layers behavior in code
target_compile_definitions(vulkan_sample PRIVATE $<$<CONFIG:Debug>:ENABLE_VULKAN_VALIDATION>)

install(TARGETS vulkan_sample RUNTIME DESTINATION bin)
