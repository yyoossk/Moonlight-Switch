//
//  ingame_overlay.cpp
//  Moonlight
//
//  Created by Даниил Виноградов on 29.05.2021.
//

#include "ingame_overlay_view.hpp"
#include "streaming_input_overlay.hpp"
#include <libretro-common/retro_timers.h>

using namespace brls;

bool debug = false;

// MARK: - Ingame Overlay View
IngameOverlay::IngameOverlay(StreamingView* streamView) :
    streamView(streamView)
{
    brls::Application::registerXMLView("LogoutTab", [streamView]() { return new LogoutTab(streamView); });
    brls::Application::registerXMLView("OptionsTab", [streamView]() { return new OptionsTab(streamView); });
    
    this->inflateFromXMLRes("xml/views/ingame_overlay/overlay.xml");
    
    addGestureRecognizer(new TapGestureRecognizer([this](TapGestureStatus status, Sound* sound) {
        if (status.state == GestureState::END)
            this->dismiss();
    }));
    
    applet->addGestureRecognizer(new TapGestureRecognizer([this](TapGestureStatus status, Sound* sound) { }));
}

brls::AppletFrame* IngameOverlay::getAppletFrame()
{
    return applet;
}

IngameOverlay::~IngameOverlay() {
}

// MARK: - Logout Tab
LogoutTab::LogoutTab(StreamingView* streamView) :
    streamView(streamView)
{
    this->inflateFromXMLRes("xml/views/ingame_overlay/logout_tab.xml");
    
    disconnect->setText("main/streaming/disconnect"_i18n);
    disconnect->registerClickAction([this, streamView](View* view) {
        this->dismiss([streamView] {
            streamView->terminate(false);
        });
        return true;
    });

    terminateButton->setText("main/streaming/terminate"_i18n);
    terminateButton->registerClickAction([this, streamView](View* view) {
        this->dismiss([streamView] {
            streamView->terminate(true);
        });
        return true;
    });
}

// MARK: - Debug Tab
OptionsTab::OptionsTab(StreamingView* streamView) :
    streamView(streamView)
{
    this->inflateFromXMLRes("xml/views/ingame_overlay/options_tab.xml");
    
    volumeHeader->setSubtitle(std::to_string(Settings::instance().get_volume()) + "%");
    float amplification = Settings::instance().get_volume_amplification() ? 500.0f : 100.0f;
    float progress = Settings::instance().get_volume() / amplification;
    volumeSlider->getProgressEvent()->subscribe([this, amplification](float progress) {
        int volume = progress * amplification;
        Settings::instance().set_volume(volume);
        volumeHeader->setSubtitle(std::to_string(volume) + "%");
    });
    volumeSlider->setProgress(progress);
    
    inputOverlayButton->setText("main/streaming/mouse_input"_i18n);
    inputOverlayButton->registerClickAction([this, streamView](View* view) {
        this->dismiss([this]() {
            StreamingInputOverlay* overlay = new StreamingInputOverlay(this->streamView);
            Application::pushActivity(new Activity(overlay));
        });
        return true;
    });
    
    onscreenLogButton->init("main/streaming/show_logs"_i18n, Settings::instance().write_log(), [](bool value) {
        Settings::instance().set_write_log(value);
        brls::Application::enableDebuggingView(value);
    });
    
    debugButton->init("main/streaming/debug_info"_i18n, streamView->draw_stats, [streamView](bool value) {
        streamView->draw_stats = value;
    });
}
