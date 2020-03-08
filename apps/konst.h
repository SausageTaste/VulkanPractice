#pragma once

#include <array>


namespace dal {

    constexpr unsigned WIN_WIDTH = 1280;
    constexpr unsigned WIN_HEIGHT = 720;

    const std::array<const char*, 1> VAL_LAYERS_TO_USE = {
       "VK_LAYER_KHRONOS_validation"
    };

}
