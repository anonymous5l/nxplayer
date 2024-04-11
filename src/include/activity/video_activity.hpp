//
// Created by Anonymous on 2024/4/8.
//

#pragma once

#include <borealis.hpp>
#include <utility>

#include "view/video_view.hpp"

class VideoActivity : public brls::Activity {
public:
    // Declare that the content of this activity is the given XML file
    CONTENT_FROM_XML_RES("activity/video.xml");

    VideoActivity(std::string name, std::string path) :
        name(std::move(name)),
        path(std::move(path)) {};

    void onContentAvailable() override;
private:
    std::string name;
    std::string path;

    BRLS_BIND(VideoView, video, "video");
};
