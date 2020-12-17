#include "vkwindow.h"

#include <iostream>


int main(int argc, char** argv) {
    dal::VulkanWindowGLFW window;
    std::cout << "Window and Vulkan is ready.\n";

    while ( !window.isOughtToClose() ) {
        window.update();
    }

    window.waitSafeExit();
    return 0;
}
