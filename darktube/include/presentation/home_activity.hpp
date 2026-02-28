#pragma once

#include <borealis.hpp>

namespace DarkTube {
namespace Presentation {

    class HomeActivity : public brls::Activity {
    public:
        HomeActivity();
        ~HomeActivity() override = default;

        brls::View* createContentView() override;
    
    private:
        brls::Box* sidebar;
        brls::Box* mainContent;
        bool sidebarVisible = true;

        brls::Box* createSidebar();
        brls::Box* createSidebarItem(const std::string& title, std::function<bool(brls::View*)> onClick);
        
        brls::Box* createMainContent();
        brls::Box* createCategoryRow(const std::string& title);
        brls::Box* createEmptyStateView();
        
        brls::Box* createSettingsView();

        brls::Box* createFooterHints();

        void toggleSidebar();
        void promptForNewIP();
        void promptForSearch();
        void renderSettingsView();

        bool isServerEmpty();
    };

} // namespace Presentation
} // namespace DarkTube
