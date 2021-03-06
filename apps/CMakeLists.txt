cmake_minimum_required(VERSION 3.4.1)

project(Vulkan-Practice
    LANGUAGES CXX
)


add_executable(vulkan_practice
    main.cpp
    vkwindow.h          vkwindow.cpp
    vkdevice.h          vkdevice.cpp
    konst.h
    swapchain_images.h  swapchain_images.cpp
    shader.h            shader.cpp
    renderpass.h        renderpass.cpp
    util_windows.h      util_windows.cpp
    fbufmanager.h       fbufmanager.cpp
    vkommand.h          vkommand.cpp
    util_vulkan.h       util_vulkan.cpp
    physdevice.h        physdevice.cpp
    swapchain.h         swapchain.cpp
    logidevice.h        logidevice.cpp
    semaphore.h         semaphore.cpp
    timer.h             timer.cpp
    vert_data.h         vert_data.cpp
    uniform.h           uniform.cpp
    texture.h           texture.cpp
    command_pool.h      command_pool.cpp
    depth_image.h       depth_image.cpp
    model_data.h        model_data.cpp
    model_render.h      model_render.cpp
    view_camera.h       view_camera.cpp
    data_tensor.h
)
target_compile_features(vulkan_practice PUBLIC cxx_std_17)


# Library

set(extern_dir ${CMAKE_CURRENT_SOURCE_DIR}/../extern)

add_subdirectory(${extern_dir}/glfw ${CMAKE_CURRENT_BINARY_DIR}/glfw)
target_link_libraries(vulkan_practice PRIVATE glfw)

add_subdirectory(${extern_dir}/io_scene_dalbaragi ${CMAKE_CURRENT_BINARY_DIR}/io_scene_dalbaragi)
target_link_libraries(vulkan_practice PRIVATE dalbaragi_model_parser)
target_include_directories(vulkan_practice PRIVATE ${extern_dir}/io_scene_dalbaragi/cpp)

add_subdirectory(${extern_dir}/glm ${CMAKE_CURRENT_BINARY_DIR}/glm)
target_include_directories(vulkan_practice PRIVATE ${extern_dir}/glm)
target_link_libraries(vulkan_practice PRIVATE glm)

target_include_directories(vulkan_practice PRIVATE ${extern_dir}/stb)

find_package(Vulkan REQUIRED)
target_compile_definitions(vulkan_practice PRIVATE VK_USE_PLATFORM_WIN32_KHR)
target_include_directories(vulkan_practice PRIVATE Vulkan::Vulkan)
target_link_libraries(vulkan_practice PRIVATE Vulkan::Vulkan)
