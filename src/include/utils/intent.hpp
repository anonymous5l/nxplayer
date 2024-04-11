//
// Created by Anonymous on 2024/4/3.
//

#pragma once

#include <borealis.hpp>

class Intent {
public:
    static void openMain();
    static void openVideo(std::string name, std::string url);

//    static void _registerFullscreen(brls::Activity* activity);
};


//#if defined(__linux__) || defined(_WIN32) || defined(__APPLE__)
//#define ALLOW_FULLSCREEN
//#endif
//
//#ifdef ALLOW_FULLSCREEN
//#define registerFullscreen(activity) Intent::_registerFullscreen(activity)
//#else
//#define registerFullscreen(activity) (void)activity
//#endif
