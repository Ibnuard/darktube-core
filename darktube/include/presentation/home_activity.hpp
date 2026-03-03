#pragma once

#include <borealis.hpp>
#include "../domain/models.hpp"
#include <vector>

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
        brls::Box* createCategoryRow(const std::string& title, const std::vector<Domain::VideoItem>& videos);
        brls::Box* createEmptyStateView();
        
        void fetchTrending();
        void renderVideoGrid(const std::vector<Domain::VideoItem>& videos);

        std::string currentTitle = "Trending Entertainment";
        std::vector<Domain::VideoItem> currentVideos;
        
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
