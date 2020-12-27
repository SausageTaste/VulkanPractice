#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan.h>


namespace dal {

    std::string getCurrentDir(void);
    std::string findResPath(void);


    struct ImageData {
        uint32_t width, height, channels;
        VkFormat format;
        std::vector<uint8_t> buffer;
    };

    dal::ImageData open_image_stb(const char* const image_path);
    dal::ImageData open_image_astc(const char* const image_path);

}
