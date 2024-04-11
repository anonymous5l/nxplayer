//
// Created by Anonymous on 2024/4/8.
//

#pragma once

#include <chrono>

namespace Utils {
    inline std::string pre0(size_t num, size_t length) {
        std::string str = std::to_string(num);
        if (length <= str.length()) {
            return str;
        }
        return std::string(length - str.length(), '0') + str;
    }

    size_t getUnixTime();
    std::string sec2Time(size_t t);

    static inline time_t unix_time() { return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); }
}
