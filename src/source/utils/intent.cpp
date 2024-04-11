//
// Created by Anonymous on 2024/4/3.
//

#include "utils/intent.hpp"

#include "activity/main_activity.hpp"
#include "activity/video_activity.hpp"

void Intent::openMain() {
    auto activity = new MainActivity();
    brls::Application::pushActivity(activity);
}

void Intent::openVideo(std::string name, std::string url) {
    auto activity = new VideoActivity(name, url);
    brls::Application::pushActivity(activity, brls::TransitionAnimation::NONE);
}
