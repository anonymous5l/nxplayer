#include <borealis.hpp>

#include "utils/register.hpp"
#include "utils/intent.hpp"
#include "view/mpv_core.hpp"

int main(int argc, char* argv[]) {
    MPVCore::HARDWARE_DEC = true;

    brls::Platform::APP_LOCALE_DEFAULT = brls::LOCALE_ZH_HANS;

    if (!brls::Application::init()) {
        brls::Logger::error("Unable to init application");
        return EXIT_FAILURE;
    }

    brls::Logger::setLogLevel(brls::LogLevel::LOG_DEBUG);

    Register::initCustomView();
    Register::initCustomTheme();

    // only for NX
    brls::Application::getPlatform()->exitToHomeMode(true);
    brls::Application::createWindow("nxplayer");
    brls::Application::getPlatform()->disableScreenDimming(false);

    if (brls::Application::getPlatform()->isApplicationMode()) {
        Intent::openMain();
    } else {
        return EXIT_FAILURE;
    }

    while (brls::Application::mainLoop()) {
    };

    return EXIT_SUCCESS;
}
