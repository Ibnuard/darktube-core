#pragma once

#include <borealis.hpp>
#include "../domain/models.hpp"
#include <vector>
#include <memory>

namespace DarkTube {
namespace Presentation {

    class HomeActivity : public brls::Activity {
    public:
        HomeActivity();
        ~HomeActivity() override;

        brls::View* createContentView() override;
    
    private:
        brls::Box* sidebar;
        brls::Box* mainContent;
        brls::Image* miniLogo;
        bool sidebarVisible = true;

        // Pagination state
        std::string nextPageToken;
        bool isLoadingMore = false;
        std::string currentSearchQuery;
        std::string currentMode = "trending"; // "trending" or "search"
        brls::Box* gridWrapper = nullptr;
        brls::ScrollingFrame* gridContainer = nullptr;
        brls::Box* loadingIndicator = nullptr;
        std::shared_ptr<bool> aliveFlag = std::make_shared<bool>(true);

        brls::Box* createSidebar();
        brls::Box* createSidebarItem(const std::string& title, std::function<bool(brls::View*)> onClick);
        
        brls::Box* createMainContent();
        brls::Box* createCategoryRow(const std::string& title, const std::vector<Domain::VideoItem>& videos);
        brls::Box* createEmptyStateView();
        
        void fetchTrending();
        void fetchMore();
        void appendVideosToGrid(const std::vector<Domain::VideoItem>& videos);
        void renderVideoGrid(const std::vector<Domain::VideoItem>& videos);

        std::string currentTitle = "Trending Entertainment";
        std::vector<Domain::VideoItem> currentVideos;
        
        brls::Box* createSettingsView();

        brls::Box* createFooterHints(bool isSettings = false);

        void toggleSidebar();
        void promptForNewIP();
        void promptForEditIP(const Domain::ServerIP& server);
        void promptForSearch();
        void renderSettingsView();
        void updateFooter(bool isSettings);

        bool isServerEmpty();
    };

} // namespace Presentation
} // namespace DarkTube
