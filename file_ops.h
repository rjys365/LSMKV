#pragma once

#include<string>
#include<sys/stat.h>
//#include<unistd.h>

namespace file_ops{
    bool isFile(const std::string &path) {
        struct stat buf;
        if (stat(path.c_str(), &buf) != 0) {
            return false;
        }
        return S_ISREG(buf.st_mode);
    }

    bool isDirectory(const std::string &path) {
        struct stat buf;
        if (stat(path.c_str(), &buf) != 0) {
            return false;
        }
        return S_ISDIR(buf.st_mode);
    }
}