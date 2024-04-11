//
// Created by Anonymous on 2024/4/8.
//

#include "view/video_view.hpp"

#include "fragment/player_setting.hpp"

#define CHECK_OSD(shake)                                                              \
    if (is_osd_lock) {                                                                \
        if (isOSDShown()) {                                                           \
            brls::Application::giveFocus(this->osdLockBox);                           \
            if (shake) this->osdLockBox->shakeHighlight(brls::FocusDirection::RIGHT); \
        } else {                                                                      \
            this->showOSD(true);                                                      \
        }                                                                             \
        return true;                                                                  \
    }

static int getSeekRange(int current) {
    current = abs(current);
    if (current < 60) return 5;
    if (current < 300) return 10;
    if (current < 600) return 20;
    if (current < 1200) return 60;
    return current / 15;
}

VideoView::VideoView() {
    this->inflateFromXMLRes("xml/view/video_view.xml");

    this->mpvCore = &MPVCore::instance();

    this->input = brls::Application::getPlatform()->getInputManager();

    this->osdSlider->getInputProgressEvent()->subscribe([this](float progress) {
        if (seeking_iter > 0) {
            brls::cancelDelay(seeking_iter);
            seeking_iter = 0;
        }

        auto value = (float)this->mpvCore->duration * progress;
        int64_t sp = (int64_t)(value - this->seeking_last_play_time);
        if (sp >= 0 && sp < 5) {
            sp = 5;
        } else if (sp < 0 && sp > -5) {
            sp = -5;
        }

        if (this->seeking_last_play_time + sp < 0) {
            this->mpvCore->seek(0);
        } else if (this->seeking_last_play_time + sp > this->mpvCore->duration) {
            this->mpvCore->seek(this->mpvCore->duration);
        } else {
            this->mpvCore->seek(this->seeking_last_play_time + sp);
        }

        this->seeking_last_play_time = 0;

        if (osdCenterBox2->getVisibility() != brls::Visibility::GONE) {
            hideCenterHint();
        }
    });

    this->osdSlider->getProgressEvent()->subscribe([this](float progress) {
        if (this->isSeeking()) {
            auto value = (float)this->mpvCore->duration * progress;

            this->setPlaybackTime(value);

            if (this->seeking_last_play_time == 0) {
                this->seeking_last_play_time = this->mpvCore->playback_time;
            }

            showOSD(false);

            if (osdCenterBox2->getVisibility() != brls::Visibility::VISIBLE) {
                showCenterHint();
                setCenterHintIcon("svg/arrow-left-right.svg");
            }

            int64_t sp = (int64_t)(value - this->seeking_last_play_time);
            if (sp >= 0 && sp < 5) {
                sp = 5;
            } else if (sp < 0 && sp > -5) {
                sp = -5;
            }
            setCenterHintText(fmt::format("{:+d} s", sp));

            if (seeking_iter <= 0) {
                ASYNC_RETAIN
                seeking_iter = brls::delay(300, [ASYNC_TOKEN, sp]() {
                    ASYNC_RELEASE
                    if (this->seeking_last_play_time + sp < 0) {
                        this->mpvCore->seek(0);
                        this->seeking_last_play_time = 0;
                    } else if (this->seeking_last_play_time + sp > this->mpvCore->duration) {
                        this->mpvCore->seek(this->mpvCore->duration);
                        this->seeking_last_play_time = this->mpvCore->duration;
                    } else {
                        this->mpvCore->seek(this->seeking_last_play_time + sp);
                    }
                    brls::cancelDelay(seeking_iter);
                    seeking_iter = 0;
                });
            }
        }
    });

    /// 播放/暂停 按钮
    this->btnToggle->addGestureRecognizer(
            new brls::TapGestureRecognizer(this->btnToggle, [this]() { this->togglePlay(); }));
    this->btnToggle->registerClickAction([this](...) {
        this->togglePlay();
        return true;
    });

    /// 音量按钮
    this->btnVolumeIcon->getParent()->registerClickAction([this](brls::View* view) {
        // 一直显示 OSD
        this->showOSD(false);
        auto theme     = brls::Application::getTheme();
        auto container = new brls::Box();
        container->setHideClickAnimation(true);
        container->addGestureRecognizer(new brls::TapGestureRecognizer(container, [this, container]() {
            // 几秒后自动关闭 OSD
            this->showOSD(true);
            container->dismiss();
        }));

        // 滑动条背景
        auto sliderBox = new brls::Box();
        sliderBox->setAlignItems(brls::AlignItems::CENTER);
        sliderBox->setHeight(60);
        sliderBox->setCornerRadius(4);
        sliderBox->setBackgroundColor(theme.getColor("color/grey_1"));
        float sliderX = view->getX() - 100;
        if (sliderX < 0) sliderX = 20;
        if (sliderX > brls::Application::ORIGINAL_WINDOW_WIDTH - 132)
            sliderX = brls::Application::ORIGINAL_WINDOW_WIDTH - 132;
        sliderBox->setTranslationX(sliderX);
        sliderBox->setTranslationY(view->getY() - 70);

        // 滑动条
        auto slider = new brls::Slider();
        slider->setMargins(8, 16, 8, 16);
        slider->setWidth(200);
        slider->setHeight(40);
        slider->setProgress((float)this->mpvCore->getVolume() / 100.0f);
        slider->getProgressEvent()->subscribe([this](float progress) { this->mpvCore->setVolume(progress * 100); });
        sliderBox->addView(slider);
        container->addView(sliderBox);
        auto frame = new brls::AppletFrame(container);
        frame->setInFadeAnimation(true);
        frame->setHeaderVisibility(brls::Visibility::GONE);
        frame->setFooterVisibility(brls::Visibility::GONE);
        frame->setBackgroundColor(theme.getColor("brls/backdrop"));
        container->registerAction("hints/back"_i18n, brls::BUTTON_B, [this, container](...) {
            // 几秒后自动关闭 OSD
            this->showOSD(true);
            container->dismiss();
            return true;
        });
        brls::Application::pushActivity(new brls::Activity(frame));

        // 手动将焦点赋给音量组件
        brls::sync([container]() { brls::Application::giveFocus(container); });
        return true;
    });
    this->btnVolumeIcon->getParent()->addGestureRecognizer(
            new brls::TapGestureRecognizer(this->btnVolumeIcon->getParent()));

    if (mpvCore->volume <= 0) {
        this->btnVolumeIcon->setImageFromRes("svg/bpx-svg-sprite-volume-off.svg");
    }

    this->osdLockBox->registerClickAction([this](...) {
        this->toggleOSDLock();
        return true;
    });
    this->osdLockBox->addGestureRecognizer(new brls::TapGestureRecognizer(this->osdLockBox));

    this->registerAction(
            "toggleOSD", brls::ControllerButton::BUTTON_Y,
            [this](brls::View* view) -> bool {
                // 拖拽进度时不要影响显示 OSD
                if (this->isSeeking()) return true;
                this->toggleOSD();
                return true;
            },
            true);

    this->registerAction(
            "lockView", brls::ControllerButton::BUTTON_X,
            [this](brls::View* view) -> bool {
                this->OSDLock();
                this->showOSD(true);
                return true;
            },
            true);

    // 暂停
    this->registerAction(
            "toggle", brls::ControllerButton::BUTTON_SPACE,
            [this](...) -> bool {
                CHECK_OSD(true);
                this->togglePlay();
                return true;
            },
            true);

    /// 组件触摸事件
    /// 单击控制 OSD
    /// 双击控制播放与暂停
    /// 长按加速
    /// 滑动调整进度
    /// 左右侧滑动调整音量，在支持调节背光的设备上左侧滑动调节背光亮度，右侧调节音量
    this->addGestureRecognizer(new OsdGestureRecognizer([this](OsdGestureStatus status) {
        switch (status.osdGestureType) {
            case OsdGestureType::TAP:
                this->toggleOSD();
                break;
            case OsdGestureType::DOUBLE_TAP_END:
                if (is_osd_lock) {
                    this->toggleOSD();
                    break;
                }
                this->togglePlay();
                break;
            case OsdGestureType::LONG_PRESS_START: {
                if (is_osd_lock) break;
                float SPEED = MPVCore::VIDEO_SPEED == 100 ? 2.0 : MPVCore::VIDEO_SPEED * 0.01f;
                this->setSpeed(SPEED);
                // 绘制临时加速标识
                this->speedHintLabel->setText(fmt::format(fmt::runtime("main/player/current_speed"_i18n), SPEED));
                this->speedHintBox->setVisibility(brls::Visibility::VISIBLE);
                break;
            }
            case OsdGestureType::LONG_PRESS_CANCEL:
            case OsdGestureType::LONG_PRESS_END:
                if (is_osd_lock) {
                    this->toggleOSD();
                    break;
                }
                if (mpvCore->getSpeed() != 1.0f) {
                    this->setSpeed(1.0f);
                }
                if (this->speedHintBox->getVisibility() != brls::Visibility::GONE) {
                    this->speedHintBox->setVisibility(brls::Visibility::GONE);
                }
                break;
            case OsdGestureType::HORIZONTAL_PAN_START:
                if (is_osd_lock) break;
                this->showCenterHint();
                this->setCenterHintIcon("svg/arrow-left-right.svg");
                break;
            case OsdGestureType::HORIZONTAL_PAN_UPDATE:
                if (is_osd_lock) break;
                this->requestSeeking(fmin(120.0f, mpvCore->duration) * status.deltaX);
                break;
            case OsdGestureType::HORIZONTAL_PAN_CANCEL:
                if (is_osd_lock) break;
                // 立即取消
                this->requestSeeking(VIDEO_CANCEL_SEEKING, VIDEO_SEEK_IMMEDIATELY);
                break;
            case OsdGestureType::HORIZONTAL_PAN_END:
                if (is_osd_lock) {
                    this->toggleOSD();
                    break;
                }
                // 立即跳转
                this->requestSeeking(fmin(120.0f, mpvCore->duration) * status.deltaX, VIDEO_SEEK_IMMEDIATELY);
                break;
            case OsdGestureType::LEFT_VERTICAL_PAN_START:
                if (is_osd_lock) break;
                if (brls::Application::getPlatform()->canSetBacklightBrightness()) {
                    this->brightness_init = brls::Application::getPlatform()->getBacklightBrightness();
                    this->showCenterHint();
                    this->setCenterHintIcon("svg/sun-fill.svg");
                    break;
                }
            case OsdGestureType::RIGHT_VERTICAL_PAN_START:
                if (is_osd_lock) break;
                this->volume_init = (int)this->mpvCore->volume;
                this->showCenterHint();
                this->setCenterHintIcon("svg/bpx-svg-sprite-volume.svg");
                break;
            case OsdGestureType::LEFT_VERTICAL_PAN_UPDATE:
                if (is_osd_lock) break;
                if (brls::Application::getPlatform()->canSetBacklightBrightness()) {
                    this->requestBrightness(this->brightness_init + status.deltaY);
                    break;
                }
            case OsdGestureType::RIGHT_VERTICAL_PAN_UPDATE:
                if (is_osd_lock) break;
                this->requestVolume(this->volume_init + status.deltaY * 100);
                break;
            case OsdGestureType::LEFT_VERTICAL_PAN_CANCEL:
            case OsdGestureType::LEFT_VERTICAL_PAN_END:
                if (is_osd_lock) {
                    this->toggleOSD();
                    break;
                }
                if (brls::Application::getPlatform()->canSetBacklightBrightness()) {
                    this->hideCenterHint();
                    break;
                }
            case OsdGestureType::RIGHT_VERTICAL_PAN_CANCEL:
            case OsdGestureType::RIGHT_VERTICAL_PAN_END:
                if (is_osd_lock) {
                    this->toggleOSD();
                    break;
                }
                this->hideCenterHint();
                break;
            default:
                break;
        }
    }));

    this->registerAction(
            "volumeUp", brls::ControllerButton::BUTTON_NAV_UP,
            [this](brls::View* view) -> bool {
                CHECK_OSD(true);
                brls::ControllerState state{};
                input->updateUnifiedControllerState(&state);
                if (state.buttons[brls::BUTTON_RT]) {
                    this->requestVolume((int)MPVCore::instance().volume + 5, 400);
                    return true;
                }
                return false;
            },
            true, true);

    this->registerAction(
            "volumeDown", brls::ControllerButton::BUTTON_NAV_DOWN,
            [this](brls::View* view) -> bool {
                CHECK_OSD(true);
                brls::ControllerState state{};
                input->updateUnifiedControllerState(&state);
                if (state.buttons[brls::BUTTON_RT]) {
                    this->requestVolume((int)MPVCore::instance().volume - 5, 400);
                    return true;
                }
                return false;
            },
            true, true);

    this->registerAction(
        "cancel", brls::ControllerButton::BUTTON_B,
        [this](brls::View* view) -> bool {
            if (is_osd_lock) {
                this->toggleOSD();
                return true;
            }

            if (isOSDShown()) {
                this->toggleOSD();
                return true;
            } else {
                auto dialog = new brls::Dialog("main/player/exit_hint"_i18n);
                dialog->addButton("hints/cancel"_i18n, []() {});
                dialog->addButton("hints/ok"_i18n, [this]()
                {
                    brls::Application::popActivity(brls::TransitionAnimation::NONE);
                });
                dialog->open();
            }

            return true;
        },
        true);

    this->registerAction("togglePlay",brls::ControllerButton::BUTTON_A, [this](brls::View* view) {
        CHECK_OSD(false);
        this->showOSD(true);
        this->togglePlay();
        return true;
    });

    this->registerAction(
            "\uE08F", brls::ControllerButton::BUTTON_LB,
            [this](brls::View* view) -> bool {
                CHECK_OSD(true);
                seeking_range -= getSeekRange(seeking_range);
                this->requestSeeking(seeking_range);
                return true;
            },
            true, true);

    this->registerAction(
            "\uE08E", brls::ControllerButton::BUTTON_RB,
            [this](brls::View* view) -> bool {
                CHECK_OSD(true);
                brls::ControllerState state{};
                input->updateUnifiedControllerState(&state);
                bool buttonY =
                        brls::Application::isSwapInputKeys() ? state.buttons[brls::BUTTON_X] : state.buttons[brls::BUTTON_Y];
                if (buttonY) {
                    seeking_range -= getSeekRange(seeking_range);
                } else {
                    seeking_range += getSeekRange(seeking_range);
                }
                this->requestSeeking(seeking_range);
                return true;
            },
            true, true);

    this->registerAction("PLAYER_SETTING", brls::ControllerButton::BUTTON_START,
     [this](brls::View* view) -> bool {
         CHECK_OSD(true);

         return this->openPlayerSetting();
     });

    this->btnSettingIcon->getParent()->registerClickAction([this](View *view) {
        return this->openPlayerSetting();
    });

    this->videoSpeed->registerClickAction([this](View *view){
        auto speed = this->mpvCore->getSpeed();
        auto index = std::find(videoSpeedValues.begin(), videoSpeedValues.end(), speed);
        if (index >= videoSpeedValues.end()) {
            return true;
        }

        auto defaultOption = index - videoSpeedValues.begin();

        brls::Dropdown* dropdown = new brls::Dropdown(
                "main/player/speed"_i18n, videoSpeedOptions, [](int selected) {
                }, defaultOption, [this](int selected) {
                    this->mpvCore->setSpeed(videoSpeedValues[selected]);
                });
        brls::Application::pushActivity(new brls::Activity(dropdown));
        return true;
    });
    this->videoSpeed->addGestureRecognizer(new brls::TapGestureRecognizer(this->btnToggle));

    this->registerMPVEvent();
}

VideoView::~VideoView() {
    mpvCore->getEvent()->unsubscribe(eventSubscribeID);
    mpvCore->stop();
}

MPVCore *VideoView::getCore() {
    return this->mpvCore;
}

bool VideoView::openPlayerSetting() {
    auto setting = new PlayerSetting(this->mpvCore);

    setting->setInFadeAnimation(true);

    brls::Application::pushActivity(new brls::Activity(setting));
    brls::sync([setting]() { brls::Application::giveFocus(setting); });

    return true;
}

bool VideoView::isSeeking() {
    return this->is_seeking ? this->is_seeking : this->osdSlider->isProgressing();
}

void VideoView::requestBrightness(float brightness) {
    if (brightness < 0) brightness = 0.0f;
    if (brightness > 1) brightness = 1.0f;
    brls::Application::getPlatform()->setBacklightBrightness(brightness);
    setCenterHintText(fmt::format("{} %", (int)(brightness * 100)));
}

void VideoView::requestVolume(int volume, int delay) {
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    this->mpvCore->setVolume(volume);
    setCenterHintText(fmt::format("{} %", volume));
    if (delay == 0) return;
    if (volume_iter == 0) {
        this->showCenterHint();
        this->setCenterHintIcon("svg/bpx-svg-sprite-volume.svg");
    } else {
        brls::cancelDelay(volume_iter);
    }
    ASYNC_RETAIN
    volume_iter = brls::delay(delay, [ASYNC_TOKEN]() {
        ASYNC_RELEASE
        this->hideCenterHint();
        this->volume_iter = 0;
    });
}

void VideoView::onChildFocusGained(View* directChild, View* focusedView) {
    Box::onChildFocusGained(directChild, focusedView);

    if (is_osd_lock) {
        brls::Application::giveFocus(this->osdLockBox);
        return;
    }

    if (isOSDShown()) {
        // 当弹幕按钮隐藏时不可获取焦点
        if (focusedView->getParent()->getVisibility() == brls::Visibility::GONE) {
            brls::Application::giveFocus(this);
        }
        static View* lastFocusedView = nullptr;

        // 设定自定义导航
        if (focusedView == this->btnSettingIcon) {
            this->btnSettingIcon->setCustomNavigationRoute(
                    brls::FocusDirection::DOWN,
                    lastFocusedView == this->btnToggle ? "video/osd/toggle" : "video/osd/lock/box");
        }
        lastFocusedView = focusedView;
        return;
    }
    brls::Application::giveFocus(this);
}

void VideoView::showCenterHint() { osdCenterBox2->setVisibility(brls::Visibility::VISIBLE); }

void VideoView::hideCenterHint() { osdCenterBox2->setVisibility(brls::Visibility::GONE); }

void VideoView::setCenterHintIcon(const std::string& svg) { centerIcon2->setImageFromRes(svg); }

void VideoView::setCenterHintText(const std::string& text) { centerLabel2->setText(text); }

void VideoView::requestSeeking(int seek, int delay) {
    if (mpvCore->duration <= 0) {
        seeking_range = 0;
        is_seeking    = false;
        return;
    }
    double progress = (this->mpvCore->playback_time + seek) / mpvCore->duration;

    if (progress < 0) {
        progress = 0;
        seek     = (int64_t)this->mpvCore->playback_time * -1;
    } else if (progress > 1) {
        progress = 1;
        seek     = mpvCore->duration;
    }

    showOSD(false);
    if (osdCenterBox2->getVisibility() != brls::Visibility::VISIBLE) {
        showCenterHint();
        setCenterHintIcon("svg/arrow-left-right.svg");
    }
    setCenterHintText(fmt::format("{:+d} s", seek));
    osdSlider->setProgress((float)progress);
    leftStatusLabel->setText(Utils::sec2Time(mpvCore->duration * progress));

    // 取消之前的延迟触发
    brls::cancelDelay(seeking_iter);
    if (delay <= 0) {
        this->hideCenterHint();
        seeking_range = 0;
        is_seeking    = false;
        if (seek == 0) return;
        mpvCore->seekRelative(seek);
    } else {
        // 延迟触发跳转进度
        is_seeking = true;
        ASYNC_RETAIN
        seeking_iter = brls::delay(delay, [ASYNC_TOKEN, seek]() {
            ASYNC_RELEASE
            this->hideCenterHint();
            seeking_range = 0;
            is_seeking    = false;
            if (seek == 0) return;
            mpvCore->seekRelative(seek);
        });
    }
}

void VideoView::OSDLock() {
    osdTopBox->setVisibility(brls::Visibility::INVISIBLE);
    osdBottomBox->setVisibility(brls::Visibility::INVISIBLE);
    // 锁定时上下按键不可用
    osdLockBox->setCustomNavigationRoute(brls::FocusDirection::UP, "video/osd/lock/box");
    osdLockBox->setCustomNavigationRoute(brls::FocusDirection::DOWN, "video/osd/lock/box");
    this->osdLockIcon->setImageFromRes("svg/player-lock.svg");
    is_osd_lock = true;
}

void VideoView::OSDUnlock() {
    // 手动设置上下按键的导航路线
    osdLockBox->setCustomNavigationRoute(brls::FocusDirection::UP, "video/osd/setting");
    osdLockBox->setCustomNavigationRoute(brls::FocusDirection::DOWN, "video/osd/icon/box");
    this->osdLockIcon->setImageFromRes("svg/player-unlock.svg");
    is_osd_lock = false;
}

void VideoView::toggleOSDLock() {
    if (is_osd_lock) {
        VideoView::OSDUnlock();
    } else {
        VideoView::OSDLock();
    }
    this->showOSD(true);
}

void VideoView::togglePlay() {
    if (this->mpvCore->isPaused()) {
        this->mpvCore->resume();
    } else {
        this->mpvCore->pause();
    }
}

void VideoView::refreshToggleIcon() {
    if (this->isSeeking()) return;

    if (!mpvCore->isPlaying()) {
        btnToggleIcon->setImageFromRes("svg/bpx-svg-sprite-play.svg");
    } else {
        btnToggleIcon->setImageFromRes("svg/bpx-svg-sprite-pause.svg");
    }
}

void VideoView::showOSD(bool temp) {
    if (temp) {
        this->osdLastShowTime = Utils::unix_time() + VideoView::OSD_SHOW_TIME;
        this->osd_state       = OSDState::SHOWN;
    } else {
        this->osdLastShowTime = (std::numeric_limits<std::time_t>::max)();
        this->osd_state = OSDState::ALWAYS_ON;
    }
}

void VideoView::hideOSD() {
    this->osdLastShowTime = 0;
    this->osd_state       = OSDState::HIDDEN;
}

void VideoView::toggleOSD() {
    if (isOSDShown()) {
        this->hideOSD();
    } else {
        this->showOSD(true);
    }
}

bool VideoView::isOSDShown() {
    return this->is_osd_shown;
}

void VideoView::showLoading() {
    if (osdCenterBox2->getVisibility() != brls::Visibility::VISIBLE && osdCenterBox->getVisibility() != brls::Visibility::VISIBLE)
        osdCenterBox->setVisibility(brls::Visibility::VISIBLE);
}

void VideoView::hideLoading() {
    if (osdCenterBox->getVisibility() != brls::Visibility::GONE)
        osdCenterBox->setVisibility(brls::Visibility::GONE);
}

void VideoView::registerMPVEvent() {
    eventSubscribeID = this->mpvCore->getEvent()->subscribe([this](MpvEventEnum event) {
        switch (event) {
        case MpvEventEnum::MPV_IDLE:
            refreshToggleIcon();
            break;
        case MpvEventEnum::MPV_RESUME:
            this->showOSD(true);
            this->hideLoading();
            break;
        case MpvEventEnum::MPV_PAUSE:
            this->showOSD(false);
            break;
        case MpvEventEnum::START_FILE:
            this->showOSD(false);
            break;
        case MpvEventEnum::LOADING_START:
            this->showLoading();
            break;
        case MpvEventEnum::LOADING_END:
            this->hideLoading();
            break;
        case MpvEventEnum::MPV_STOP:
            this->hideLoading();
            this->showOSD(false);
            break;
        case MpvEventEnum::UPDATE_DURATION:
            if (!this->isSeeking()) {
                this->setDuration(this->mpvCore->duration);
                this->setProgress((float) mpvCore->playback_time / (float) this->mpvCore->duration);
                this->osdSlider->setStep((float) brls::Application::getFPS() / ((float) this->mpvCore->duration * 2.0f));
            }
            break;
        case MpvEventEnum::UPDATE_PROGRESS:
            if (!this->isSeeking()) {
                this->setPlaybackTime(this->mpvCore->video_progress);
                this->setProgress((float) mpvCore->playback_time / (float) this->mpvCore->duration);
                this->osdSlider->setStep((float) brls::Application::getFPS() / ((float) this->mpvCore->duration * 2.0f));
            }
        case MpvEventEnum::VIDEO_SPEED_CHANGE:
            if (fabs(mpvCore->video_speed - 1) < 1e-5) {
                this->videoSpeed->setText("main/player/speed"_i18n);
            } else {
                this->videoSpeed->setText(fmt::format("{}x", mpvCore->video_speed));
            }
            break;
        case MpvEventEnum::END_OF_FILE:
            this->showOSD(false);
        case MpvEventEnum::CACHE_SPEED_CHANGE:
            // 仅当加载圈已经开始转起的情况显示缓存
            if (this->osdCenterBox->getVisibility() != brls::Visibility::GONE) {
                if (this->centerLabel->getVisibility() != brls::Visibility::VISIBLE)
                    this->centerLabel->setVisibility(brls::Visibility::VISIBLE);
                this->centerLabel->setText(mpvCore->getCacheSpeed());
            }
            break;
        case MpvEventEnum::VIDEO_MUTE:
            this->btnVolumeIcon->setImageFromRes("svg/bpx-svg-sprite-volume-off.svg");
            break;
        case MpvEventEnum::VIDEO_UNMUTE:
            this->btnVolumeIcon->setImageFromRes("svg/bpx-svg-sprite-volume.svg");
            break;
        case MpvEventEnum::RESET:
            this->setProgress(0);
            this->setPlaybackTime(0);
            this->setDuration(0);
            break;
        default:
            break;
        }
    });
}

void VideoView::setProgress(float value) {
    if (this->isSeeking()) return;
    if (isnan(value)) return;
    this->osdSlider->setProgress(value);
}

void VideoView::setDuration(size_t duration) {
    this->rightStatusLabel->setText(Utils::sec2Time(duration));
}

void VideoView::setPlaybackTime(size_t duration) {
    this->leftStatusLabel->setText(Utils::sec2Time(duration));
}

void VideoView::setSpeed(float speed) { mpvCore->setSpeed(speed); }

void VideoView::setUrl(const std::string& url) {
    this->mpvCore->setUrl(url);
}

void VideoView::setTitle(const std::string& title) {
    this->title->setText(title);
}

brls::View *VideoView::getDefaultFocus() {
    if (isOSDShown()) {
        return this->btnToggle;
    }
    return this;
}

void VideoView::buttonProcessing() {
    // 获取按键数据
    brls::ControllerState state{};
    input->updateUnifiedControllerState(&state);

    // 当OSD显示时上下左右切换选择按钮，持续显示OSD
    if (isOSDShown() && (state.buttons[brls::BUTTON_NAV_RIGHT] || state.buttons[brls::BUTTON_NAV_LEFT] ||
                         state.buttons[brls::BUTTON_NAV_UP] || state.buttons[brls::BUTTON_NAV_DOWN])) {
        if (this->osd_state == OSDState::SHOWN) this->showOSD(true);
    }
    if (is_osd_lock) return;
}

void VideoView::onOSDStateChanged(bool state) {
    if ((!state && isChildFocused()) || (state && brls::Application::getCurrentFocus() == this)) {
        brls::Application::giveFocus(this);
    }
}

void VideoView::draw(NVGcontext* vg, float x, float y, float width, float height, brls::Style style,
                     brls::FrameContext* ctx) {
    if (!mpvCore->isValid()) return;

    time_t current = Utils::unix_time();
    bool drawOSD   = current < this->osdLastShowTime;
    auto alpha = this->getAlpha();

    mpvCore->draw(brls::Rect(x, y, width, height), alpha);

    // draw bottom bar
    if (BOTTOM_BAR) {
        bottomBarColor.a = alpha;
        float progress   = mpvCore->playback_time / (double)mpvCore->duration;
        progress         = progress > 1.0f ? 1.0f : progress;
        nvgFillColor(vg, bottomBarColor);
        nvgBeginPath(vg);
        nvgRect(vg, x, y + height - 2, width * progress, 2);
        nvgFill(vg);
    }

    if (drawOSD) {
        if (!is_osd_shown) {
            is_osd_shown = true;
            this->onOSDStateChanged(true);
        }

        if (!is_osd_lock) {
            if (osdTopBox->getVisibility() != brls::Visibility::VISIBLE)
                osdTopBox->setVisibility(brls::Visibility::VISIBLE);
            if (osdBottomBox->getVisibility() != brls::Visibility::VISIBLE)
                osdBottomBox->setVisibility(brls::Visibility::VISIBLE);
            osdTopBox->frame(ctx);
            osdBottomBox->frame(ctx);
        }

        // draw osd lock button
        if (osdLockBox->getVisibility() != brls::Visibility::VISIBLE)
            osdLockBox->setVisibility(brls::Visibility::VISIBLE);
        osdLockBox->frame(ctx);
    } else {
        if (is_osd_shown) {
            is_osd_shown = false;
            this->onOSDStateChanged(false);
        }

        if (osdTopBox->getVisibility() != brls::Visibility::INVISIBLE)
            osdTopBox->setVisibility(brls::Visibility::INVISIBLE);
        if (osdBottomBox->getVisibility() != brls::Visibility::INVISIBLE)
            osdBottomBox->setVisibility(brls::Visibility::INVISIBLE);
        if (osdLockBox->getVisibility() != brls::Visibility::INVISIBLE)
            osdLockBox->setVisibility(brls::Visibility::INVISIBLE);
    }

    // hot key
    this->buttonProcessing();

    // draw speed
    if (this->speedHintBox->getVisibility() == brls::Visibility::VISIBLE) {
        this->speedHintBox->frame(ctx);
        brls::Rect frame = speedHintLabel->getFrame();

        // a1-3 周期 800，范围 800 * 0.3 / 2 = 120, 0 - 120 - 0
        int a1 = ((brls::getCPUTimeUsec() >> 10) % 800) * 0.3;
        int a2 = (a1 + 40) % 240;
        int a3 = (a1 + 80) % 240;

        if (a1 > 120) a1 = 240 - a1;
        if (a2 > 120) a2 = 240 - a2;
        if (a3 > 120) a3 = 240 - a3;

        float tx                              = frame.getMinX() - 64;
        float ty                              = frame.getMinY() + 5;
        std::vector<std::pair<int, int>> data = {{0, a3 + 80}, {20, a2 + 80}, {40, a1 + 80}};

        for (auto& i : data) {
            nvgBeginPath(vg);
            nvgMoveTo(vg, tx + i.first, ty);
            nvgLineTo(vg, tx + i.first, ty + 16);
            nvgLineTo(vg, tx + i.first + 16, ty + 8);
            nvgFillColor(vg, a(nvgRGBA(255, 255, 255, i.second)));
            nvgClosePath(vg);
            nvgFill(vg);
        }
    }

    osdCenterBox2->frame(ctx);
}

VideoView *VideoView::create() {
    return new VideoView();
}