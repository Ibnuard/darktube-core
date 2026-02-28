#include "../include/presentation/server_list_activity.hpp"
#include "../include/presentation/home_activity.hpp"
#include "../include/core/theme.hpp"
#include "../include/data/ip_repository.hpp"
#include "../include/domain/models.hpp"
#include <borealis.hpp>
#include "../include/presentation/ui_utils.hpp"

namespace DarkTube {
namespace Presentation {

    ServerListActivity::ServerListActivity() {
        // Activity construction
        brls::Logger::info("ServerListActivity created");
    }

    brls::View* ServerListActivity::createContentView() {
        brls::Box* root = new brls::Box();
        root->setAxis(brls::Axis::COLUMN);
        root->setWidthPercentage(100);
        root->setHeightPercentage(100);
        root->setBackgroundColor(Theme::BackgroundDark);
        root->setJustifyContent(brls::JustifyContent::SPACE_BETWEEN);
        root->setPadding(40, 60, 40, 60);

        // Header Title
        brls::Box* header = new brls::Box();
        header->setWidthPercentage(100);
        header->setAlignItems(brls::AlignItems::CENTER);
        header->setJustifyContent(brls::JustifyContent::CENTER);
        header->setMarginTop(80);

        brls::Label* title = new brls::Label();
        title->setText("Welcome to DarkTube");
        title->setFontSize(48);
        title->setTextColor(Theme::TextPrimary);
        header->addView(title);

        brls::Label* subtitle = new brls::Label();
        subtitle->setText("Please connect to your DarkTube server to continue.");
        subtitle->setFontSize(24);
        subtitle->setTextColor(Theme::TextSecondary);
        subtitle->setMarginTop(20);
        header->addView(subtitle);

        root->addView(header);

        // Central Input Area
        root->addView(createEmptyStateView());

        // Global Footer Hints
        brls::Box* footer = new brls::Box();
        footer->setAxis(brls::Axis::ROW);
        footer->setWidthPercentage(100);
        footer->setAlignItems(brls::AlignItems::CENTER);
        footer->setJustifyContent(brls::JustifyContent::CENTER);
        
        footer->addView(UIUtils::createHint(nvgRGB(50, 160, 60), "A", "Enter IP Address"));

        root->addView(footer);

        return root;
    }

    brls::Box* ServerListActivity::createEmptyStateView() {
        brls::Box* content = new brls::Box();
        content->setAxis(brls::Axis::COLUMN);
        content->setGrow(1.0f);
        content->setAlignItems(brls::AlignItems::CENTER);
        content->setJustifyContent(brls::JustifyContent::CENTER);

        brls::Box* panel = new brls::Box();
        panel->setAxis(brls::Axis::COLUMN);
        panel->setAlignItems(brls::AlignItems::CENTER);
        panel->setBackgroundColor(Theme::SurfaceDark);
        panel->setCornerRadius(16);
        panel->setPadding(60, 60, 60, 60);

        // Simulated Input Field
        brls::Box* inputBox = new brls::Box();
        inputBox->setAxis(brls::Axis::COLUMN);
        inputBox->setWidth(500);
        inputBox->setBackgroundColor(nvgRGB(45, 45, 45));
        inputBox->setCornerRadius(8);
        inputBox->setPadding(20, 25, 20, 25);

        brls::Label* inputHint = new brls::Label();
        inputHint->setText("Server IP (e.g., 192.168.1.100)");
        inputHint->setFontSize(20);
        inputHint->setTextColor(nvgRGB(150, 150, 150));
        inputHint->setMarginBottom(10);
        inputBox->addView(inputHint);

        brls::Label* val = new brls::Label();
        val->setText("Tap A to Input IP");
        val->setFontSize(32);
        val->setTextColor(Theme::TextPrimary);
        inputBox->addView(val);

        panel->addView(inputBox);

        brls::Button* btn = new brls::Button();
        btn->setStyle(&brls::BUTTONSTYLE_PRIMARY);
        btn->setText("Connect to Server");
        btn->setMarginTop(40);
        btn->setWidth(500);
        btn->setHeight(60);
        btn->registerAction("Connect", brls::BUTTON_A, [this](brls::View* view) {
            this->promptForNewIP();
            return true;
        });

        panel->addView(btn);
        content->addView(panel);
        
        return content;
    }

    brls::Box* ServerListActivity::createSavedServersView() {
        // Obsolete, main.cpp redirects direct to HomeActivity if saved servers exist.
        return new brls::Box();
    }

    void ServerListActivity::promptForNewIP() {
        brls::Logger::info("Prompting for New IP via IME...");
        
        brls::Application::getPlatform()->getImeManager()->openForText(
            [this](std::string text) {
                if (!text.empty()) {
                    Domain::ServerIP newIp;
                    newIp.id = std::to_string(brls::getCPUTimeUsec());
                    newIp.name = "My Server";
                    newIp.address = text;

                    Data::IPRepository::getInstance().addServer(newIp);
                    Data::IPRepository::getInstance().setActiveServer(newIp);

                    brls::Logger::info("IP added. Pushing HomeActivity.");
                    brls::Application::pushActivity(new HomeActivity());
                }
            },
            "Enter Server IP",
            "192.168.1.100",
            255,
            "",
            0
        );
    }

} // namespace Presentation
} // namespace DarkTube
