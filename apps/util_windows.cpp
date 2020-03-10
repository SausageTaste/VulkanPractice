#include "util_windows.h"

#include <iostream>

#include <windows.h>


namespace dal {

    std::string getCurrentDir(void) {
        char buffer[MAX_PATH];
        GetModuleFileName(NULL, buffer, MAX_PATH);
        const auto pos = std::string{ buffer }.find_last_of("\\/");
        return std::string{ buffer }.substr(0, pos);
    }

}
