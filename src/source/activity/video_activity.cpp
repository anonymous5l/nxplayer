//
// Created by Anonymous on 2024/4/8.
//

#include "activity/video_activity.hpp"

void VideoActivity::onContentAvailable() {
    this->video->setUrl(this->path);
    this->video->setTitle(this->name);
}