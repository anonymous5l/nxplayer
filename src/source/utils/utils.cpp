//
// Created by Anonymous on 2024/4/8.
//

#include "utils/utils.hpp"

size_t Utils::getUnixTime() {
    auto now = std::chrono::system_clock::now();
    return std::chrono::system_clock::to_time_t(now);
}

std::string Utils::sec2Time(size_t t) {
    size_t hour   = t / 3600;
    size_t minute = t / 60 % 60;
    size_t sec    = t % 60;
    return Utils::pre0(hour, 2) + ":" + Utils::pre0(minute, 2) + ":" + Utils::pre0(sec, 2);
}

