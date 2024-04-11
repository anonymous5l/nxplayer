//
// Created by Anonymous on 2024/4/8.
//

#pragma once

#include <borealis.hpp>

#include "utils/gesture_helper.hpp"
#include "view/mpv_core.hpp"
#include "fragment/player_setting.hpp"

using namespace brls::literals;

#define VIDEO_CANCEL_SEEKING 0
#define VIDEO_SEEK_IMMEDIATELY 0

enum class OSDState {
    HIDDEN    = 0,
    SHOWN     = 1,
    ALWAYS_ON = 2,
};

class VideoView : public brls::Box {
public:
    VideoView();
    ~VideoView();

    inline static bool BOTTOM_BAR = true;

    void setUrl(const std::string& url);
    void setTitle(const std::string& title);

    void onChildFocusGained(View* directChild, View* focusedView) override;
    bool openPlayerSetting();

    MPVCore *getCore();

    View *getDefaultFocus() override;

    void draw(NVGcontext* vg, float x, float y, float width, float height, brls::Style style,
              brls::FrameContext* ctx) override;

    static VideoView *create();
private:
    MPVCore* mpvCore;

    void registerMPVEvent();
    void setDuration(size_t duration);
    void setPlaybackTime(size_t playback);
    void setProgress(float value);
    void setSpeed(float speed);
    void togglePlay();
    void refreshToggleIcon();
    void showOSD(bool temp);
    void hideOSD();
    void toggleOSD();
    bool isOSDShown();
    void toggleOSDLock();
    void OSDLock();
    void OSDUnlock();
    void showLoading();
    void hideLoading();
    void buttonProcessing();
    void onOSDStateChanged(bool state);
    void showCenterHint();
    void hideCenterHint();
    void setCenterHintIcon(const std::string& svg);
    void setCenterHintText(const std::string& text);
    void requestSeeking(int seek, int delay = 400);
    void requestBrightness(float brightness);
    void requestVolume(int volume, int delay = 0);
    bool isSeeking();

    MPVEvent::Subscription eventSubscribeID;

    bool is_seeking = false;
    size_t seeking_iter = 0;      // 请求跳转的延迟函数 handle
    int seeking_range   = 0;      // 跳转的目标进度, 跳转结束后归零
    int seeking_last_play_time   = 0;

    float brightness_init = 0.0f;

    int volume_init    = 0;
    size_t volume_iter = 0;  // 音量UI关闭的延迟函数 handle

    brls::InputManager* input;

    time_t osdLastShowTime     = 0;
    const time_t OSD_SHOW_TIME = 5;  //默认显示五秒
    OSDState osd_state         = OSDState::HIDDEN;
    bool is_osd_shown          = false;
    bool is_osd_lock           = false;
    bool hide_lock_button      = false;

    std::vector<std::string> videoSpeedOptions = {"4.0x", "3.0x", "2.0x", "1.75x", "1.5x", "1.25x", "1.0x", "0.75x", "0.5x", "0.25x"};
    std::vector<double> videoSpeedValues = {4, 3, 2, 1.75, 1.5, 1.25, 1, 0.75, 0.5, 0.25};

    NVGcolor bottomBarColor = brls::Application::getTheme().getColor("brls/accent");

    BRLS_BIND(brls::Image, osdLockIcon, "video/osd/lock/icon");
    BRLS_BIND(brls::Box, osdTopBox, "video/osd/top/box");
    BRLS_BIND(brls::Box, osdBottomBox, "video/osd/bottom/box");
    BRLS_BIND(brls::Box, osdCenterBox, "video/osd/center/box");
    BRLS_BIND(brls::Box, osdLockBox, "video/osd/lock/box");

    // 用于通用的提示信息
    BRLS_BIND(brls::Box, osdCenterBox2, "video/osd/center/box2");
    BRLS_BIND(brls::Label, centerLabel2, "video/osd/center/label2");
    BRLS_BIND(brls::Image, centerIcon2, "video/osd/center/icon2");

    BRLS_BIND(brls::Label, videoSpeed, "video/speed");

    BRLS_BIND(brls::Label, centerLabel, "video/osd/center/label");

    BRLS_BIND(brls::Label, title, "video/osd/title");
    BRLS_BIND(brls::Box, speedHintBox, "video/speed/hint/box");
    BRLS_BIND(brls::Label, speedHintLabel, "video/speed/hint/label");
    BRLS_BIND(brls::Image, btnToggleIcon, "video/osd/toggle/icon");

    BRLS_BIND(brls::Slider, osdSlider, "video/osd/bottom/progress");
    BRLS_BIND(brls::Label, leftStatusLabel, "video/left/status");
    BRLS_BIND(brls::Label, centerStatusLabel, "video/center/status");
    BRLS_BIND(brls::Label, rightStatusLabel, "video/right/status");

    BRLS_BIND(brls::Box, btnToggle, "video/osd/toggle");
    BRLS_BIND(brls::Image, btnVolumeIcon, "video/osd/volume/icon");
    BRLS_BIND(brls::Image, btnSettingIcon, "video/osd/setting/icon");
};

