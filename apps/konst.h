#pragma once

#include <array>


namespace dal {

    constexpr unsigned WIN_WIDTH = 1280;
    constexpr unsigned WIN_HEIGHT = 720;

    constexpr unsigned MAX_FRAMES_IN_FLIGHT = 2;

    const std::array<const char*, 1> VAL_LAYERS_TO_USE = {
       "VK_LAYER_KHRONOS_validation"
    };

    const std::array<const char*, 1> DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    const std::array<const char*, 7> RAY_TRACING_EXTENSIONS = {
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_RAY_QUERY_EXTENSION_NAME,
        VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,

        "SPV_KHR_ray_tracing",
        "SPV_KHR_ray_query",
    };

}
