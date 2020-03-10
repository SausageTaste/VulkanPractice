#pragma once

#include <array>


namespace dal {

    constexpr unsigned WIN_WIDTH = 1280;
    constexpr unsigned WIN_HEIGHT = 720;

    const std::array<const char*, 1> VAL_LAYERS_TO_USE = {
       "VK_LAYER_KHRONOS_validation"
    };

    const std::array<const char*, 1> DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

}
