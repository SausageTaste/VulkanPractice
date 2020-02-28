cmake_minimum_required(VERSION 3.4.1)

project(Vulkan-Practice
    LANGUAGES CXX
)


add_executable(vulkan_practice
    main.cpp
)
target_compile_features(vulkan_practice PUBLIC cxx_std_17)


# Library

set(extern_dir ${CMAKE_CURRENT_SOURCE_DIR}/../extern)

add_subdirectory(${extern_dir}/glfw ${CMAKE_CURRENT_BINARY_DIR}/glfw)
target_link_libraries(vulkan_practice PUBLIC glfw)

add_subdirectory(${extern_dir}/glm ${CMAKE_CURRENT_BINARY_DIR}/glm)
target_link_libraries(vulkan_practice PUBLIC glm)

find_package(Vulkan REQUIRED FATAL_ERROR)
target_link_libraries (vulkan_practice PUBLIC ${VULKAN_LIB_LIST})
