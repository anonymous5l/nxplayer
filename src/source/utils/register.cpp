//
// Created by Anonymous on 2024/4/5.
//

#include "view/video_view.hpp"

#include "utils/register.hpp"

#include "fragment/local_driver.hpp"

void Register::initCustomView() {
    brls::Application::registerXMLView("LocalDriver", LocalDriver::create);
    brls::Application::registerXMLView("VideoView", VideoView::create);
}

void Register::initCustomTheme() {
    brls::Theme::getLightTheme().addColor("color/grey_1", nvgRGB(245, 246, 247));
    brls::Theme::getDarkTheme().addColor("color/grey_1", nvgRGB(51, 52, 53));
}
