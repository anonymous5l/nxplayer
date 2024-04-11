//
// Created by Anonymous on 2024/4/8.
//

#include "fragment/player_setting.hpp"

#include "utils/utils.hpp"


class TrackCell : public brls::RadioCell {
public:
    explicit TrackCell(int64_t id, std::string type): id(id), type(std::move(type)) {}
    int64_t getId() {
        return this->id;
    }
    std::string getType() {
        return this->type;
    }
private:
    int64_t id;
    std::string type;
};

PlayerSetting::PlayerSetting(MPVCore *mpvCore) {
    this->inflateFromXMLRes("xml/fragment/player_setting.xml");

    this->core = mpvCore;

    setupTrack();

    cellSleep->setText("main/setting/sleep"_i18n);
    this->updateSleepContent(Utils::getUnixTime());
    cellSleep->registerClickAction([this](View* view) {
        std::vector<int> timeList           = {15, 30, 60, 90, 120};
        std::string min                     = "main/common/minute"_i18n;
        std::vector<std::string> optionList = {"15 " + min, "30 " + min, "60 " + min, "90 " + min, "120 " + min};
        bool countdownStarted               = MPVCore::CLOSE_TIME != 0 && Utils::getUnixTime() < MPVCore::CLOSE_TIME;
        if (countdownStarted) {
            // 添加关闭选项
            timeList.insert(timeList.begin(), -1);
            optionList.insert(optionList.begin(), "hints/off"_i18n);
        }

        brls::Dropdown* dropdown = new brls::Dropdown(
            "main/setting/sleep"_i18n, optionList,
            [](int selected) {

            }, -1, [this, timeList, countdownStarted](int selected){
                brls::Logger::debug("selected {}", selected);
                if (selected == 0 && countdownStarted) {
                    MPVCore::CLOSE_TIME = 0;
                } else {
                    MPVCore::CLOSE_TIME = Utils::getUnixTime() + timeList[countdownStarted ? selected-1 : selected] * 60;
                }
                this->updateSleepContent(Utils::getUnixTime());
            });
        brls::Application::pushActivity(new brls::Activity(dropdown));
        return true;
    });

    cellProgress->init("main/setting/bottom_progress"_i18n,
                       VideoView::BOTTOM_BAR, [this](bool s) {
        VideoView::BOTTOM_BAR = s;
    });

    const float _16_9 = 16.0f/9.0f;
    const float _4_3 = 4.0f/3.0f;

    if (mpvCore->video_aspect == -1) {
        this->selectedVideoAspect = 0;
    } else if (mpvCore->video_aspect == -2) {
        this->selectedVideoAspect = 1;
    } else if (mpvCore->video_aspect == -3) {
        this->selectedVideoAspect = 2;
    } else if (mpvCore->video_aspect == _4_3) {
        this->selectedVideoAspect = 3;
    } else if (mpvCore->video_aspect == _16_9) {
        this->selectedVideoAspect = 4;
    } else {
        this->selectedVideoAspect = 0;
    }

    cellVideoAspect->init("main/setting/aspect/header"_i18n, {
        "main/setting/aspect/auto"_i18n, "main/setting/aspect/stretch"_i18n,
        "main/setting/aspect/crop"_i18n, "4:3", "16:9"
    }, this->selectedVideoAspect, [this](int selected){
        this->selectedVideoAspect = selected;
        auto theme = brls::Application::getTheme();
        cellVideoAspect->setDetailTextColor(theme["brls/list/listItem_value_color"]);

        if (this->selectedVideoAspect == 0) {
            this->core->setAspect("-1");
        } else if (this->selectedVideoAspect == 1) {
            this->core->setAspect("-2");
        } else if (this->selectedVideoAspect == 2) {
            this->core->setAspect("-3");
        } else if (this->selectedVideoAspect == 3) {
            this->core->setAspect("4:3");
        } else if (this->selectedVideoAspect == 4) {
            this->core->setAspect("16:9");
        } else {
            this->core->setAspect("-1");
        }
    });

    cellEqualizerReset->registerClickAction([this](View* view) {
        cellEqualizerBrightness->slider->setProgress(0.5f);
        cellEqualizerContrast->slider->setProgress(0.5f);
        cellEqualizerSaturation->slider->setProgress(0.5f);
        cellEqualizerGamma->slider->setProgress(0.5f);
        cellEqualizerHue->slider->setProgress(0.5f);
        return true;
    });

    registerHideBackground(cellEqualizerReset);

    setupEqualizerSetting(cellEqualizerBrightness, "main/setting/equalizer/brightness"_i18n,
                          mpvCore->getBrightness(), [mpvCore](int value) {
        mpvCore->setBrightness(value);
    });
    setupEqualizerSetting(cellEqualizerContrast, "main/setting/equalizer/contrast"_i18n,
                          mpvCore->getContrast(), [mpvCore](int value) {
        mpvCore->setContrast(value);
    });
    setupEqualizerSetting(cellEqualizerSaturation, "main/setting/equalizer/saturation"_i18n,
                          mpvCore->getSaturation(), [mpvCore](int value) {
        mpvCore->setSaturation(value);
    });
    setupEqualizerSetting(cellEqualizerGamma, "main/setting/equalizer/gamma"_i18n,
                          mpvCore->getGamma(), [mpvCore](int value){
        mpvCore->setGamma(value);
    });
    setupEqualizerSetting(cellEqualizerHue, "main/setting/equalizer/hue"_i18n,
                          mpvCore->getHue(), [mpvCore](int value) {
        mpvCore->setHue(value);
    });

    this->registerAction("hints/cancel"_i18n, brls::BUTTON_B, [](...) {
        brls::Application::popActivity();
        return true;
    });

    this->registerClickAction([](...){
        brls::Application::popActivity();
        return true;
    });
    this->addGestureRecognizer(new brls::TapGestureRecognizer(this));
}


void PlayerSetting::setupTrack() {
    auto mpv = this->core;
    if (!mpv->isValid()) {
        return;
    }

    std::vector<Track> tracks = mpv->getTracks();
    for (auto &track : tracks) {
        TrackCell *cell = nullptr;

        std::string lang;
        if (!track.lang.empty()) {
            lang = brls::internal::getRawStr(fmt::format("iso639/{}", track.lang));
        } else {
            lang = "iso639/und"_i18n;
        }

        if (track.type == "audio") {
            cell = new TrackCell(track.id, track.type);

            if (trackAudioBox->getVisibility() == brls::Visibility::GONE) {
                trackAudioBox->setVisibility(brls::Visibility::VISIBLE);
                trackAudioHeader->setVisibility(brls::Visibility::VISIBLE);
            }

            trackAudioBox->addView(cell);
        } else if (track.type == "sub") {
            cell = new TrackCell(track.id, track.type);

            registerHideBackground(cell);

            if (subtitleBox->getVisibility() == brls::Visibility::GONE) {
                subtitleBox->setVisibility(brls::Visibility::VISIBLE);
                subtitleHeader->setVisibility(brls::Visibility::VISIBLE);
            }

            subtitleBox->addView(cell);
        }

        if (cell != nullptr) {
            if (track.title.empty()) {
                cell->title->setText(lang);
            } else {
                cell->title->setText(fmt::format("{} - {}", lang, track.title));
            }

            cell->setSelected(track.selected);

            cell->registerClickAction([cell, mpv](...) {
                if (cell->getSelected()) return false;

                cell->setSelected(true);

                for (auto& child : cell->getParent()->getChildren()) {
                    auto* v = dynamic_cast<brls::RadioCell*>(child);
                    if (cell == v) continue;
                    if (v) v->setSelected(false);
                }

                if (cell->getType() == "audio") {
                    mpv->setAudioId(cell->getId());
                } else if (cell->getType() == "sub") {
                    mpv->setSubtitleId(cell->getId());
                }
                return true;
            });
        }
    }
}

void PlayerSetting::setupEqualizerSetting(brls::SliderCell* cell, const std::string& title, int initValue, const std::function<void(int)>& callback) {
    if (initValue < -100) initValue = -100;
    if (initValue > 100) initValue = 100;
    cell->detail->setWidth(50);
    cell->title->setWidth(116);
    cell->title->setMarginRight(0);
    cell->slider->setStep(0.05f);
    cell->slider->setMarginRight(0);
    cell->slider->setPointerSize(20);
    cell->setDetailText(std::to_string(initValue));
    cell->init(title, (initValue + 100) * 0.005f, [cell, callback](float value) {
        int data = (int)(value * 200 - 100);
        if (data < -100) data = -100;
        if (data > 100) data = 100;
        cell->detail->setText(std::to_string(data));
        callback(data);
    });
    registerHideBackground(cell->getDefaultFocus());
}

void PlayerSetting::draw(NVGcontext* vg, float x, float y, float width, float height, brls::Style style,
                         brls::FrameContext* ctx) {
    static size_t updateTime = 0;
    size_t now               = Utils::getUnixTime();
    if (now != updateTime) {
        updateTime = now;
        updateSleepContent(now);
    }
    Box::draw(vg, x, y, width, height, style, ctx);
}

void PlayerSetting::registerHideBackground(brls::View *view) {
    view->getFocusEvent()->subscribe([this](...) { this->setBackgroundColor(nvgRGBAf(0.0f, 0.0f, 0.0f, 0.0f)); });

    view->getFocusLostEvent()->subscribe(
            [this](...) { this->setBackgroundColor(brls::Application::getTheme().getColor("brls/backdrop")); });
}

void PlayerSetting::updateSleepContent(size_t now) {
    if (MPVCore::CLOSE_TIME == 0 || now > MPVCore::CLOSE_TIME) {
        cellSleep->setDetailTextColor(brls::Application::getTheme()["brls/text_disabled"]);
        cellSleep->setDetailText("hints/off"_i18n);
    } else {
        cellSleep->setDetailTextColor(brls::Application::getTheme()["brls/list/listItem_value_color"]);
        cellSleep->setDetailText(Utils::sec2Time(MPVCore::CLOSE_TIME - now));
    }
}
