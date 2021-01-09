#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan.h>


namespace dal {

    std::string getCurrentDir(void);
    std::string findResPath(void);
    const std::string& get_res_path(void);

    std::vector<char> readFile(const char* const path);
    std::vector<char> readFile(const std::string& path);


    struct ImageData {
        uint32_t width, height, channels;
        VkFormat format;
        std::vector<uint8_t> buffer;
    };

    dal::ImageData open_image_stb(const char* const image_path);
    dal::ImageData open_image_astc(const char* const image_path);

}
