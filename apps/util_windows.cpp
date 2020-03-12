#include "util_windows.h"

#include <vector>
#include <sstream>
#include <algorithm>
#include <iterator>

#include <windows.h>


namespace {

    template <class Container>
    void split2(const std::string& str, Container& cont, const char delim)
    {
        std::stringstream ss(str);
        std::string token;
        while ( std::getline(ss, token, delim) ) {
            cont.push_back(token);
        }
    }

    template <typename _Iter>
    std::string join(const char delim, _Iter begin, const _Iter end) {
        std::string result;

        result += *begin;
        ++begin;

        for ( ; begin != end; ++begin ) {
            result += delim;
            result += *begin;
        }

        return result;
    }

    // https://stackoverflow.com/questions/612097/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c
    std::vector<std::string> get_all_dir_within_folder(std::string folder)
    {
        std::vector<std::string> names;
        std::string search_path = folder + "/*.*";
        WIN32_FIND_DATA fd;
        HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
        if ( hFind != INVALID_HANDLE_VALUE ) {
            do {
                // read all (real) files in current folder
                // , delete '!' read other 2 default folder . and ..
                if ( (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) {
                    names.push_back(fd.cFileName);
                }
            } while ( ::FindNextFile(hFind, &fd) );
            ::FindClose(hFind);
        }
        return names;
    }

}


namespace dal {

    std::string getCurrentDir(void) {
        char buffer[MAX_PATH];
        GetModuleFileName(NULL, buffer, MAX_PATH);
        const auto pos = std::string{ buffer }.find_last_of("\\/");
        return std::string{ buffer }.substr(0, pos);
    }

    std::string findResPath(void) {
        const auto cd = getCurrentDir();
        std::vector<std::string> splitted;
        split2(cd, splitted, '\\');

        const auto begin = splitted.begin();
        for ( size_t i = 0; i < splitted.size(); ++i ) {
            const auto lvlCount = splitted.size() - i;
            const auto thisPath = join('/', begin, begin + lvlCount);

            for ( const auto& dir : get_all_dir_within_folder(thisPath) ) {
                if ( dir == "resource" ) {
                    return thisPath + "/resource";
                }
            }
        }

        throw std::runtime_error{ "failed to find resource folder." };
    }

}
