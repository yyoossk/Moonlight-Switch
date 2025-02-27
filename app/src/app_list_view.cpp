//
//  app_list_view.cpp
//  Moonlight
//
//  Created by XITRIX on 26.05.2021.
//

#ifdef __SWITCH__
#include <switch.h>
#endif

#include "app_list_view.hpp"
#include "streaming_view.hpp"
#include "app_cell.hpp"
#include "helper.hpp"

AppListView::AppListView(Host host) :
    host(host)
{
    this->inflateFromXMLRes("xml/views/app_list_view.xml");
    
    Label* label = new brls::Label();
    label->setText(brls::Hint::getKeyIcon(ControllerButton::BUTTON_RB) + "  " + "main/app_list/run_current"_i18n);
    label->setFontSize(24);
    label->setMargins(4, 16, 4, 16);
    
    Box* holder = new Box();
    holder->addView(label);
    holder->setFocusable(true);
    holder->setCornerRadius(6);
    holder->setMargins(18, 0, 8, 0);
    holder->addGestureRecognizer(new TapGestureRecognizer(holder));
    
    hintView = holder;
    
    container->setHideHighlight(true);
    gridView = new GridView();
    container->addView(gridView);
    loader = new LoadingOverlay(this);
    
    auto playCuttentAction = [this, host](View* view) {
        if (currentApp.has_value())
        {
            AppletFrame* frame = new AppletFrame(new StreamingView(host, currentApp.value()));
            frame->setHeaderVisibility(brls::Visibility::GONE);
            frame->setFooterVisibility(brls::Visibility::GONE);
            Application::pushActivity(new Activity(frame));
        }
        return true;
    };
    
    hintView->registerClickAction(playCuttentAction);
    hintView->registerAction("", brls::ControllerButton::BUTTON_RB, playCuttentAction, true);
    registerAction("", brls::ControllerButton::BUTTON_RB, playCuttentAction, true);

// #ifdef __SWITCH__
//     registerAction("", brls::ControllerButton::BUTTON_LB, [](View* view) {
//         appletPerformSystemButtonPressingIfInFocus(AppletSystemButtonType_HomeButtonLongPressing);
//         return true;
//     }, true);
// #endif

    registerAction("main/app_list/reload_app_list"_i18n, BUTTON_X, [this](View* view) {
        this->updateAppList();
        return true;
    });
    blockInput(true);
}

void AppListView::blockInput(bool block)
{
    if (block && !inputBlocked)
    {
        inputBlocked = block;
        Application::blockInputs();
    }
    else if (!block && inputBlocked)
    {
        inputBlocked = block;
        Application::unblockInputs();
    }
}

void AppListView::terninateApp()
{
    if (loading)
        return;
    
    Dialog* dialog = new Dialog("main/app_list/terminate_prefix"_i18n + currentApp->name + "main/app_list/terminate_postfix"_i18n);
    
    dialog->addButton("main/common/cancel"_i18n, [] { });
    
    dialog->addButton("main/app_list/terminate"_i18n, [dialog, this] {
        if (loading)
            return;
        
        loading = true;
        gridView->clearViews();
        Application::giveFocus(this);
        loader->setHidden(false);
        unregisterAction(terminateIdentifier);
        blockInput(true);
        
        ASYNC_RETAIN
        GameStreamClient::instance().quit(host.address, [ASYNC_TOKEN](GSResult<bool> result) {
            ASYNC_RELEASE
            
            loading = false;
            loader->setHidden(true);
            
            if (!result.isSuccess())
                showError(result.error(), [this] {});
            
            updateAppList();
        });
    });
    
    dialog->open();
}

void AppListView::updateAppList()
{
    if (loading)
        return;
    
    loading = true;
    
    unregisterAction(terminateIdentifier);
    gridView->clearViews();
    Application::giveFocus(this);
    loader->setHidden(false);
    currentApp = std::nullopt;
    hintView->setVisibility(Visibility::GONE);
    blockInput(true);
    
    setTitle(host.hostname);
    
    ASYNC_RETAIN
    GameStreamClient::instance().connect(host.address, [ASYNC_TOKEN](GSResult<SERVER_DATA> result) {
        ASYNC_RELEASE
        
        if (result.isSuccess())
        {
            int currentGame = result.value().currentGame;
            
            ASYNC_RETAIN
            GameStreamClient::instance().applist(host.address, [ASYNC_TOKEN, currentGame](GSResult<AppInfoList> result) {
                ASYNC_RELEASE
                
                loading = false;
                loader->setHidden(true);
                blockInput(false);
                
                if (result.isSuccess())
                {
                    for (AppInfo app : result.value())
                    {
                        if (app.app_id == currentGame)
                            setCurrentApp(app);
                        
                        AppCell* cell = new AppCell(host, app, currentGame);
                        gridView->addView(cell);
                    }
                    Application::giveFocus(this);
                }
                else
                {
                    showError(result.error(), [this]
                    {
                        this->dismiss();
                    });
                }
            });
        }
        else
        {
            blockInput(false);
            showError(result.error(), [this]
            {
                this->dismiss();
            });
        }
    });
}

void AppListView::setCurrentApp(AppInfo app, bool update)
{
    this->currentApp = app;
    hintView->setVisibility(Visibility::VISIBLE);
    setTitle(host.hostname + " - " + "main/app_list/running"_i18n + " " + app.name);
    
    terminateIdentifier = registerAction("main/app_list/terminate_current_app"_i18n, BUTTON_BACK, [this](View* view) {
        this->terninateApp();
        return true;
    });
}

void AppListView::willAppear(bool resetState)
{
    Box::willAppear(resetState);
    updateAppList();
}

View* AppListView::getHintView()
{
    return hintView;
}

void AppListView::onLayout()
{
    Box::onLayout();
    
    if (loader)
        loader->layout();
}
