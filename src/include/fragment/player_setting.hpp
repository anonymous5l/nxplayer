//
// Created by Anonymous on 2024/4/8.
//

#pragma once

#include <borealis.hpp>
#include "view/video_view.hpp"

using namespace brls::literals;

class PlayerSetting : public brls::Box {
public:
    PlayerSetting(MPVCore *core);

    void draw(NVGcontext* vg, float x, float y, float width, float height, brls::Style style,
              brls::FrameContext* ctx);
private:
    BRLS_BIND(brls::SelectorCell, cellVideoAspect, "setting/video/aspect");
    BRLS_BIND(brls::BooleanCell, cellProgress, "setting/video/progress");
    BRLS_BIND(brls::DetailCell, cellSleep, "setting/sleep");

    BRLS_BIND(brls::SliderCell, cellEqualizerBrightness, "setting/equalizer/brightness");
    BRLS_BIND(brls::SliderCell, cellEqualizerContrast, "setting/equalizer/contrast");
    BRLS_BIND(brls::SliderCell, cellEqualizerSaturation, "setting/equalizer/saturation");
    BRLS_BIND(brls::SliderCell, cellEqualizerGamma, "setting/equalizer/gamma");
    BRLS_BIND(brls::SliderCell, cellEqualizerHue, "setting/equalizer/hue");
    BRLS_BIND(brls::RadioCell, cellEqualizerReset, "setting/equalizer/reset");

    BRLS_BIND(brls::Header, trackAudioHeader, "setting/track/audio/header");
    BRLS_BIND(brls::Box, trackAudioBox, "setting/track/audio/box");

    BRLS_BIND(brls::Header, subtitleHeader, "setting/video/subtitle/header");
    BRLS_BIND(brls::Box, subtitleBox, "setting/video/subtitle/box");

    MPVCore *core;
    int selectedVideoAspect = 0;

    void updateSleepContent(size_t ts);
    void setupTrack();
    void registerHideBackground(brls::View *view);
    void setupEqualizerSetting(brls::SliderCell* cell, const std::string& title, int initValue, const std::function<void(int)>& callback);
};
