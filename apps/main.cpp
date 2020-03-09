#include "vkwindow.h"

#include <iostream>


int main(int argc, char** argv) {
    try {
        dal::VulkanWindowGLFW window;
        std::cout << "Window and Vulkan is ready.\n";

        while ( !window.isOughtToClose() ) {
            window.update();
        }
    }
    catch ( const std::exception & e ) {
        std::cout << "Fatal Exception: " << e.what() << std::endl;
    }

    return 0;
}
