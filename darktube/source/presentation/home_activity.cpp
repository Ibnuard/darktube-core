#include "../include/presentation/home_activity.hpp"
#include "../include/presentation/server_list_activity.hpp"
#include "../include/presentation/player_activity.hpp"
#include "../include/core/theme.hpp"
#include "../include/data/ip_repository.hpp"
#include "../include/data/network_client.hpp"
#include <borealis.hpp>
#include "../include/presentation/ui_utils.hpp"

using namespace brls::literals;
#define _(x) brls::getStr(x)

namespace {
    class SidebarItem : public brls::Box {
    private:
        brls::Label* label;
    public:
        SidebarItem(const std::string& title) {
            this->setFocusable(true);
            this->setPadding(15, 20, 15, 20);
            this->setCornerRadius(8);
            this->setMarginBottom(10);
            
            label = new brls::Label();
            label->setText(title);
            label->setFontSize(22);
            label->setTextColor(DarkTube::Theme::TextSecondary); 
            
            this->addView(label);
        }

        void onFocusGained() override {
            brls::Box::onFocusGained();
            label->setTextColor(nvgRGB(255, 60, 60)); // Red text when focused
        }

        void onFocusLost() override {
            brls::Box::onFocusLost();
            label->setTextColor(DarkTube::Theme::TextSecondary); // Revert when lost focus
        }
    };
}

namespace DarkTube {
namespace Presentation {

    HomeActivity::HomeActivity() {
        brls::Logger::info("HomeActivity created");
    }

    HomeActivity::~HomeActivity() {
        brls::Logger::info("HomeActivity destroyed");
        *aliveFlag = false; // Invalidate all async callbacks
    }

    bool HomeActivity::isServerEmpty() {
        return Data::IPRepository::getInstance().getSavedServers().empty();
    }

    brls::View* HomeActivity::createContentView() {
        brls::Box* root = new brls::Box();
        root->setAxis(brls::Axis::COLUMN);
        root->setWidthPercentage(100);
        root->setHeightPercentage(100);
        root->setBackgroundColor(Theme::BackgroundDark); // Main background (navgRGB(15, 15, 15)) to contrast with sidebar
        
        // Main split: Sidebar + Content
        brls::Box* split = new brls::Box();
        split->setAxis(brls::Axis::ROW);
        split->setGrow(1.0f);
        split->setWidthPercentage(100);

        sidebar = createSidebar();
        mainContent = createMainContent();

        split->addView(sidebar);
        split->addView(mainContent);

        root->addView(split);
        root->addView(createFooterHints(false));

        // Initial fetch
        if (!isServerEmpty() && currentVideos.empty()) {
            fetchTrending();
        }

        // Global actions for this screen to toggle sidebar
        root->registerAction(_("main/toggle_sidebar"), brls::BUTTON_B, [this](brls::View* view) {
            this->toggleSidebar();
            return true;
        });

        // X Button to Change Server
        root->registerAction(_("main/change_server"), brls::BUTTON_X, [this](brls::View* view) {
            if (!this->isServerEmpty()) {
                this->promptForNewIP();
            }
            return true;
        });

        return root;
    }

    brls::Box* HomeActivity::createSidebarItem(const std::string& title, std::function<bool(brls::View*)> onClick) {
        SidebarItem* item = new SidebarItem(title);
        item->registerAction(_("main/select"), brls::BUTTON_A, onClick);

        // Highlight visual using background color change on focus is native
        item->registerAction("Focus", brls::BUTTON_NAV_UP, [](brls::View* v){return false;}); 

        return item;
    }

    brls::Box* HomeActivity::createSidebar() {
        brls::Box* bar = new brls::Box();
        bar->setAxis(brls::Axis::COLUMN);
        bar->setWidth(320);
        bar->setHeightPercentage(100);
        bar->setBackgroundColor(nvgRGB(0x12, 0x12, 0x16)); // #121216 Sidebar background
        bar->setPadding(40, 20, 40, 20);
        bar->setFocusable(false); // Container should not be focusable itself

        brls::Image* logo = new brls::Image();
        logo->setImageFromFile("romfs:/img/logo_horizontal.png"); // Using PNG instead of JPG
        logo->setWidth(200);
        logo->setHeight(50);
        logo->setScalingType(brls::ImageScalingType::FIT);
        logo->setMarginBottom(40);
        logo->setMarginLeft(20);
        bar->addView(logo);

        std::vector<std::pair<std::string, std::string>> navItems;
        navItems.push_back({"Trending", _("main/trending")});
        // Only show search if there is a connected IP
        if (!isServerEmpty()) {
            navItems.push_back({"Search", _("main/search")});
        }
        navItems.push_back({"Settings", _("main/settings")});

        for (const auto& itemPair : navItems) {
            std::string item = itemPair.first;
            std::string label = itemPair.second;
            bar->addView(createSidebarItem(label, [item, this](brls::View* view) {
                brls::Logger::info("Navigated to " + item);
                if (item == "Search") {
                    this->promptForSearch();
                    this->updateFooter(false);
                } else if (item == "Settings") {
                    this->renderSettingsView();
                    this->updateFooter(true);
                } else if (item == "Trending") {
                    this->fetchTrending();
                    this->updateFooter(false);
                }
                return true;
            }));
        }

        return bar;
    }

    brls::Box* HomeActivity::createMainContent() {
        brls::Box* content = new brls::Box();
        content->setAxis(brls::Axis::COLUMN);
        content->setGrow(1.0f);
        content->setPadding(40, 40, 0, 40);

        if (isServerEmpty()) {
            content->addView(createEmptyStateView());
        } else {
            // Header (Network/Account status)
            brls::Box* header = new brls::Box();
            header->setAxis(brls::Axis::ROW);
            header->setWidthPercentage(100);
            header->setJustifyContent(brls::JustifyContent::SPACE_BETWEEN);
            header->setMarginBottom(30);

            this->miniLogo = new brls::Image();
            this->miniLogo->setImageFromFile("romfs:/img/logo_horizontal.png");
            this->miniLogo->setDimensions(140, 35);
            this->miniLogo->setScalingType(brls::ImageScalingType::FIT);
            this->miniLogo->setVisibility(sidebarVisible ? brls::Visibility::GONE : brls::Visibility::VISIBLE);
            header->addView(this->miniLogo);

            brls::Label* ipLabel = new brls::Label();
            auto activeServer = Data::IPRepository::getInstance().getActiveServer();
            ipLabel->setText("SERVER: " + activeServer.address);
            ipLabel->setFontSize(18);
            ipLabel->setTextColor(Theme::TextSecondary);
            header->addView(ipLabel);
            content->addView(header);

            // Scrolling Grid area
            this->gridContainer = new brls::ScrollingFrame();
            this->gridContainer->setGrow(1.0f);
            
            this->gridWrapper = new brls::Box();
            this->gridWrapper->setAxis(brls::Axis::COLUMN);

            this->gridWrapper->addView(createCategoryRow(currentTitle, currentVideos));

            this->gridContainer->setContentView(this->gridWrapper);
            content->addView(this->gridContainer);
        }

        return content;
    }

    brls::Box* HomeActivity::createEmptyStateView() {
        brls::Box* state = new brls::Box();
        state->setAxis(brls::Axis::COLUMN);
        state->setGrow(1.0f);
        state->setAlignItems(brls::AlignItems::CENTER);
        state->setJustifyContent(brls::JustifyContent::CENTER);

        brls::Image* logo = new brls::Image();
        logo->setImageFromFile("romfs:/img/logo_horizontal.png"); 
        logo->setWidth(300);
        logo->setHeight(80);
        logo->setScalingType(brls::ImageScalingType::FIT);
        logo->setMarginBottom(30);
        state->addView(logo);

        brls::Label* title = new brls::Label();
        title->setText(_("main/welcome"));
        title->setFontSize(36);
        title->setTextColor(Theme::TextPrimary);
        title->setMarginBottom(10);
        state->addView(title);

        brls::Label* desc = new brls::Label();
        desc->setText(_("main/welcome_desc"));
        desc->setFontSize(22);
        desc->setTextColor(Theme::TextSecondary);
        desc->setMarginBottom(40);
        state->addView(desc);

        brls::Box* btn = createSidebarItem(_("main/add_server_ip"), [this](brls::View* v) {
            this->promptForNewIP();
            return true;
        });
        btn->setBackgroundColor(nvgRGB(40, 40, 45)); 
        btn->setWidth(brls::View::AUTO); // Allow padding to dictate size naturally
        btn->setPadding(20, 60, 20, 60); // Rectangular but padded
        btn->setCornerRadius(10); // Nice rounded corners without being pill
        btn->setJustifyContent(brls::JustifyContent::CENTER);
        
        state->addView(btn);

        return state;
    }

    brls::Box* HomeActivity::createCategoryRow(const std::string& title, const std::vector<Domain::VideoItem>& videos) {
        brls::Box* section = new brls::Box();
        section->setAxis(brls::Axis::COLUMN);
        section->setMarginBottom(40);

        brls::Label* titleLabel = new brls::Label();
        titleLabel->setText(title);
        titleLabel->setFontSize(28);
        titleLabel->setTextColor(Theme::TextPrimary);
        titleLabel->setMarginBottom(20);
        titleLabel->setMarginLeft(10);
        section->addView(titleLabel);

        if (videos.empty()) {
            brls::Box* loadingRow = new brls::Box();
            loadingRow->setAxis(brls::Axis::ROW);
            for (int i = 0; i < 4; i++) {
                loadingRow->addView(UIUtils::createSkeletonVideoCard());
            }
            section->addView(loadingRow);
        } else {
            brls::Box* currentRow = nullptr;
            for (size_t i = 0; i < videos.size(); ++i) {
                // Create a new row every 4 items
                if (i % 4 == 0) {
                    currentRow = new brls::Box();
                    currentRow->setAxis(brls::Axis::ROW);
                    section->addView(currentRow);
                }

                const auto& video = videos[i];
                brls::Box* cardContainer = new brls::Box();
                cardContainer->setAxis(brls::Axis::COLUMN);
                cardContainer->setMarginRight(25);
                cardContainer->setMarginBottom(30);
                cardContainer->setMarginLeft(10);
                cardContainer->setWidth(260); // Fixed width for grid alignment

                // Video Thumbnail (16:9)
                brls::Image* thumbnail = new brls::Image();
                thumbnail->setDimensions(256, 144);
                thumbnail->setScalingType(brls::ImageScalingType::FILL); // Use FILL/COVER
                thumbnail->setBackgroundColor(Theme::SurfaceDark);
                thumbnail->setFocusable(true);
                thumbnail->setCornerRadius(12);
                thumbnail->addGestureRecognizer(new brls::TapGestureRecognizer(thumbnail));


                // Asynchronously fetch medium thumbnail
                if (!video.thumbnailUrlMedium.empty()) {
                    Data::NetworkClient::instance().fetchImage(video.thumbnailUrlMedium, [thumbnail](const unsigned char* data, size_t size) {
                        if (data && size > 0) thumbnail->setImageFromMem(data, size);
                        else thumbnail->setImageFromFile("romfs:/img/video_placeholder.png");
                    });
                } else {
                    thumbnail->setImageFromFile("romfs:/img/video_placeholder.png");
                }

                    thumbnail->registerAction("Play", brls::BUTTON_A, [video](brls::View* view) {
                        brls::Logger::info("Play Video clicked: " + video.title);
                        
                        // Show loading dialog
                        brls::Dialog* loadingDialog = new brls::Dialog("main/getting_stream"_i18n);
                        loadingDialog->setCancelable(false);
                        
                        // Fix focus overlap issue
                        // Make Dialog focusable so it consumes the focus ring, 
                        // but hide the highlight so it doesn't draw a red box around itself.
                        loadingDialog->setFocusable(true);
                        loadingDialog->setHideHighlight(true);
                        
                        loadingDialog->open();

                        Data::NetworkClient::instance().getStream(video.id, [loadingDialog, video](const Domain::StreamInfo& info, const std::string& error) {
                            loadingDialog->close([info, error, video]() {
                                if (!error.empty()) {
                                    brls::Logger::error("Failed to fetch stream: {}", error);
                                    brls::Dialog* errorDialog = new brls::Dialog("Failed to get stream: " + error);
                                    errorDialog->addButton("OK", []() {});
                                    errorDialog->open();
                                    return;
                                }
                            
                            // Push player with fetched info
                            brls::Application::pushActivity(new PlayerActivity(info));
                        });
                    });
                    return true;
                });

                // Load more when focusing on items in last 2 rows
                if (videos.size() >= 8 && i + 8 >= videos.size()) {
                    thumbnail->registerAction("LoadMore", brls::BUTTON_NAV_DOWN, [this](brls::View* v) {
                        this->fetchMore();
                        return false; // Let navigation continue
                    });
                }

                cardContainer->addView(thumbnail);

                // Metadata
                brls::Box* metadata = new brls::Box();
                metadata->setAxis(brls::Axis::COLUMN);
                metadata->setMarginTop(12);
                metadata->setWidth(256);

                brls::Label* vidTitle = new brls::Label();
                vidTitle->setText(video.title);
                vidTitle->setFontSize(18);
                vidTitle->setTextColor(Theme::TextPrimary);
                vidTitle->setMarginBottom(4);
                vidTitle->setSingleLine(true);
                metadata->addView(vidTitle);

                brls::Label* vidChannel = new brls::Label();
                std::string channelText = video.author;
                if (video.views != "SEARCH_HIDDEN") {
                    channelText += " • " + UIUtils::formatViewCount(video.views);
                }
                vidChannel->setText(channelText);
                vidChannel->setFontSize(14);
                vidChannel->setTextColor(Theme::TextSecondary);
                metadata->addView(vidChannel);

                cardContainer->addView(metadata);

                currentRow->addView(cardContainer);
            }
        }

        return section;
    }

    void HomeActivity::fetchTrending() {
        this->currentVideos.clear();
        this->currentTitle = _("main/trending");
        this->nextPageToken = "";
        this->isLoadingMore = false;
        this->currentMode = "trending";
        this->currentSearchQuery = "";
        this->renderVideoGrid({}); // Show skeletons

        auto flag = this->aliveFlag;
        Data::NetworkClient::instance().getTrending([this, flag](const std::vector<Domain::VideoItem>& videos, const std::string& nextToken, const std::string& error) {
            if (!*flag) return;
            if (!error.empty()) {
                brls::Logger::error("Failed to fetch trending: {}", error);
                return;
            }
            this->currentVideos = videos;
            this->nextPageToken = nextToken;
            this->renderVideoGrid(videos);
        });
    }

    void HomeActivity::fetchMore() {
        if (isLoadingMore || nextPageToken.empty()) return;

        // Memory safety: cap total items to prevent OOM on Switch
        if (currentVideos.size() >= 80) {
            brls::Logger::info("Max items reached ({}), stopping pagination", currentVideos.size());
            return;
        }

        isLoadingMore = true;

        brls::Logger::info("Fetching more: mode={}, pageToken={}", currentMode, nextPageToken);

        // Show loading indicator at bottom
        if (gridWrapper) {
            loadingIndicator = new brls::Box();
            loadingIndicator->setAxis(brls::Axis::ROW);
            loadingIndicator->setJustifyContent(brls::JustifyContent::CENTER);
            loadingIndicator->setAlignItems(brls::AlignItems::CENTER);
            loadingIndicator->setHeight(60);
            loadingIndicator->setWidthPercentage(100);
            loadingIndicator->setMarginTop(10);

            brls::Label* loadingLabel = new brls::Label();
            loadingLabel->setText("Loading more...");
            loadingLabel->setFontSize(18);
            loadingLabel->setTextColor(Theme::TextSecondary);
            loadingIndicator->addView(loadingLabel);

            gridWrapper->addView(loadingIndicator);
        }

        // Use alive flag to guard against dangling this pointer
        auto flag = this->aliveFlag;
        auto callback = [this, flag](const std::vector<Domain::VideoItem>& videos, const std::string& nextToken, const std::string& error) {
            if (!*flag) return; // HomeActivity was destroyed

            // Remove loading indicator
            if (loadingIndicator && gridWrapper) {
                gridWrapper->removeView(loadingIndicator, true);
                loadingIndicator = nullptr;
            }

            isLoadingMore = false;

            if (!error.empty()) {
                brls::Logger::error("Failed to fetch more: {}", error);
                return;
            }

            this->nextPageToken = nextToken;
            // Append new videos
            for (const auto& v : videos) {
                this->currentVideos.push_back(v);
            }
            this->appendVideosToGrid(videos);
        };

        if (currentMode == "trending") {
            Data::NetworkClient::instance().getTrending(callback, nextPageToken);
        } else if (currentMode == "search") {
            Data::NetworkClient::instance().search(currentSearchQuery, callback, nextPageToken);
        }
    }

    void HomeActivity::appendVideosToGrid(const std::vector<Domain::VideoItem>& videos) {
        if (!gridWrapper || videos.empty()) return;

        // Build new rows and append to gridWrapper
        brls::Box* currentRow = nullptr;

        // Figure out how many items are in the last existing row
        // We always start from the count offset in the existing grid
        size_t existingCount = currentVideos.size() - videos.size(); // count before append
        size_t startOffset = existingCount % 4;

        for (size_t i = 0; i < videos.size(); ++i) {
            size_t gridPos = startOffset + i;
            if (gridPos % 4 == 0) {
                currentRow = new brls::Box();
                currentRow->setAxis(brls::Axis::ROW);
                gridWrapper->addView(currentRow);
            }

            const auto& video = videos[i];
            brls::Box* cardContainer = new brls::Box();
            cardContainer->setAxis(brls::Axis::COLUMN);
            cardContainer->setMarginRight(25);
            cardContainer->setMarginBottom(30);
            cardContainer->setMarginLeft(10);
            cardContainer->setWidth(260);

            brls::Image* thumbnail = new brls::Image();
            thumbnail->setDimensions(256, 144);
            thumbnail->setScalingType(brls::ImageScalingType::FILL);
            thumbnail->setBackgroundColor(Theme::SurfaceDark);
            thumbnail->setFocusable(true);
            thumbnail->setCornerRadius(12);
            thumbnail->addGestureRecognizer(new brls::TapGestureRecognizer(thumbnail));

            if (!video.thumbnailUrlMedium.empty()) {
                Data::NetworkClient::instance().fetchImage(video.thumbnailUrlMedium, [thumbnail](const unsigned char* data, size_t size) {
                    if (data && size > 0) thumbnail->setImageFromMem(data, size);
                    else thumbnail->setImageFromFile("romfs:/img/video_placeholder.png");
                });
            } else {
                thumbnail->setImageFromFile("romfs:/img/video_placeholder.png");
            }

            // Play action
            thumbnail->registerAction("Play", brls::BUTTON_A, [video](brls::View* view) {
                brls::Logger::info("Play Video clicked: " + video.title);
                brls::Dialog* loadingDialog = new brls::Dialog("main/getting_stream"_i18n);
                loadingDialog->setCancelable(false);
                loadingDialog->setFocusable(true);
                loadingDialog->setHideHighlight(true);
                loadingDialog->open();

                Data::NetworkClient::instance().getStream(video.id, [loadingDialog, video](const Domain::StreamInfo& info, const std::string& error) {
                    loadingDialog->close([info, error, video]() {
                        if (!error.empty()) {
                            brls::Logger::error("Failed to fetch stream: {}", error);
                            brls::Dialog* errorDialog = new brls::Dialog("Failed to get stream: " + error);
                            errorDialog->addButton("OK", []() {});
                            errorDialog->open();
                            return;
                        }
                        brls::Application::pushActivity(new PlayerActivity(info));
                    });
                });
                return true;
            });

            // Load more when focusing on items in the last 2 rows
            if (videos.size() >= 8 && i + 8 >= videos.size()) {
                thumbnail->registerAction("LoadMore", brls::BUTTON_NAV_DOWN, [this](brls::View* v) {
                    this->fetchMore();
                    return false; // Let navigation continue
                });
            }

            cardContainer->addView(thumbnail);

            brls::Box* metadata = new brls::Box();
            metadata->setAxis(brls::Axis::COLUMN);
            metadata->setMarginTop(12);
            metadata->setWidth(256);

            brls::Label* vidTitle = new brls::Label();
            vidTitle->setText(video.title);
            vidTitle->setFontSize(18);
            vidTitle->setTextColor(Theme::TextPrimary);
            vidTitle->setMarginBottom(4);
            vidTitle->setSingleLine(true);
            metadata->addView(vidTitle);

            brls::Label* vidChannel = new brls::Label();
            std::string channelText = video.author;
            if (video.views != "SEARCH_HIDDEN") {
                channelText += " • " + UIUtils::formatViewCount(video.views);
            }
            vidChannel->setText(channelText);
            vidChannel->setFontSize(14);
            vidChannel->setTextColor(Theme::TextSecondary);
            metadata->addView(vidChannel);

            cardContainer->addView(metadata);
            currentRow->addView(cardContainer);
        }
    }

    void HomeActivity::renderVideoGrid(const std::vector<Domain::VideoItem>& videos) {
        // Re-render main content
        this->gridWrapper = nullptr;
        this->gridContainer = nullptr;
        this->loadingIndicator = nullptr;
        if (mainContent) {
            brls::Box* split = dynamic_cast<brls::Box*>(mainContent->getParent());
            if (split) {
                split->removeView(mainContent, true);
                mainContent = createMainContent();
                split->addView(mainContent);
            }
        }
    }

    void HomeActivity::promptForSearch() {
        brls::Logger::info("Prompting for Search via IME...");
        
        brls::Application::getPlatform()->getImeManager()->openForText(
            [this](std::string text) {
                if (!text.empty()) {
                    brls::Logger::info("Searching for: " + text);
                    
                    this->currentVideos.clear();
                    this->currentTitle = _("main/searching") + ": " + text;
                    this->nextPageToken = "";
                    this->isLoadingMore = false;
                    this->currentMode = "search";
                    this->currentSearchQuery = text;
                    this->renderVideoGrid({}); // Show skeletons

                    auto flag = this->aliveFlag;
                    Data::NetworkClient::instance().search(text, [this, text, flag](const std::vector<Domain::VideoItem>& videos, const std::string& nextToken, const std::string& error) {
                        if (!*flag) return;
                        if (!error.empty()) {
                            brls::Logger::error("Search failed: {}", error);
                            return;
                        }
                        this->currentTitle = _("main/search") + ": " + text;
                        this->currentVideos = videos;
                        this->nextPageToken = nextToken;
                        this->renderVideoGrid(videos);
                    });
                }
            },
            _("main/search_darktube"),
            "",
            255,
            "",
            0
        );
    }

    brls::Box* HomeActivity::createSettingsView() {
        brls::Box* content = new brls::Box();
        content->setAxis(brls::Axis::COLUMN);
        content->setGrow(1.0f);
        content->setPadding(40, 40, 0, 40);

        brls::Label* title = new brls::Label();
        title->setText(_("main/settings_info"));
        title->setFontSize(36);
        title->setTextColor(Theme::TextPrimary);
        title->setMarginBottom(30);
        content->addView(title);

        brls::ScrollingFrame* scroll = new brls::ScrollingFrame();
        scroll->setGrow(1.0f);
        
        brls::Box* inner = new brls::Box();
        inner->setAxis(brls::Axis::COLUMN);

        auto addSection = [&inner](const std::string& headerText, const std::string& bodyText) {
            brls::Label* header = new brls::Label();
            header->setText(headerText);
            header->setFontSize(24);
            header->setTextColor(Theme::TextPrimary);
            header->setMarginTop(20);
            header->setMarginBottom(10);
            inner->addView(header);

            brls::Label* body = new brls::Label();
            body->setText(bodyText);
            body->setFontSize(18);
            body->setTextColor(Theme::TextSecondary);
            body->setMarginBottom(20);
            inner->addView(body);
        };

        // --- IP LIST SECTION ---
        brls::Label* ipHeader = new brls::Label();
        ipHeader->setText(_("main/server_config"));
        ipHeader->setFontSize(24);
        ipHeader->setTextColor(Theme::TextPrimary);
        ipHeader->setMarginTop(10);
        ipHeader->setMarginBottom(10);
        inner->addView(ipHeader);

        brls::Label* activeIpLabel = new brls::Label();
        if (isServerEmpty()) {
            activeIpLabel->setText(_("main/active_server") + ": " + _("main/none"));
        } else {
            auto server = Data::IPRepository::getInstance().getActiveServer();
            activeIpLabel->setText(_("main/active_server") + ": " + server.address);
        }
        activeIpLabel->setFontSize(18);
        activeIpLabel->setTextColor(Theme::TextSecondary);
        activeIpLabel->setMarginBottom(20);
        inner->addView(activeIpLabel);

        // List servers
        auto servers = Data::IPRepository::getInstance().getSavedServers();
        if (!servers.empty()) {
            for (const auto& s : servers) {
                brls::Box* sBtn = createSidebarItem("Server: " + s.address, [s](brls::View* v) {
                    Data::IPRepository::getInstance().setActiveServer(s);
                    brls::Application::popActivity();
                    brls::Application::pushActivity(new HomeActivity());
                    return true;
                });
                sBtn->setMarginBottom(5);

                // Add Edit (X) and Delete (Y) actions
                sBtn->registerAction(_("main/edit_server"), brls::BUTTON_X, [this, s](brls::View* v) {
                    this->promptForEditIP(s);
                    return true;
                });

                sBtn->registerAction(_("main/delete_server"), brls::BUTTON_Y, [this, s](brls::View* v) {
                    Data::IPRepository::getInstance().removeServer(s.id);
                    this->renderSettingsView(); // Refresh settings view
                    return true;
                });

                inner->addView(sBtn);
            }
        }

        // Add new server button
        brls::Box* addBtn = createSidebarItem(_("main/add_new_server"), [this](brls::View* v) {
            this->promptForNewIP();
            return true;
        });
        addBtn->setMarginTop(5);
        addBtn->setMarginBottom(30);
        inner->addView(addBtn);

        // --- LANGUAGE SECTION ---
        brls::Label* langHeader = new brls::Label();
        langHeader->setText(_("main/language"));
        langHeader->setFontSize(24);
        langHeader->setTextColor(Theme::TextPrimary);
        langHeader->setMarginTop(10);
        langHeader->setMarginBottom(10);
        inner->addView(langHeader);

        std::string currentLang = Data::IPRepository::getInstance().getLanguage();

        // English Option
        brls::Box* enBtn = createSidebarItem("English" + (std::string)(currentLang == "en-US" ? " ✓" : ""), [this](brls::View* v) {
            if (Data::IPRepository::getInstance().getLanguage() != "en-US") {
                Data::IPRepository::getInstance().setLanguage("en-US");
                brls::Platform::APP_LOCALE_DEFAULT = "en-US";
                brls::loadTranslations();
                brls::Application::popActivity();
                brls::Application::pushActivity(new HomeActivity());
            }
            return true;
        });
        enBtn->setMarginBottom(5);
        inner->addView(enBtn);

        // Indonesian Option
        brls::Box* idBtn = createSidebarItem("Bahasa Indonesia" + (std::string)(currentLang == "id-ID" ? " ✓" : ""), [this](brls::View* v) {
            if (Data::IPRepository::getInstance().getLanguage() != "id-ID") {
                Data::IPRepository::getInstance().setLanguage("id-ID");
                brls::Platform::APP_LOCALE_DEFAULT = "id-ID";
                brls::loadTranslations();
                brls::Application::popActivity();
                brls::Application::pushActivity(new HomeActivity());
            }
            return true;
        });
        idBtn->setMarginBottom(30);
        inner->addView(idBtn);

        // --- PROXY SECTION ---
        brls::Label* proxyHeader = new brls::Label();
        proxyHeader->setText(_("main/proxy_settings"));
        proxyHeader->setFontSize(24);
        proxyHeader->setTextColor(Theme::TextPrimary);
        proxyHeader->setMarginTop(10);
        proxyHeader->setMarginBottom(10);
        inner->addView(proxyHeader);

        bool currentProxy = Data::IPRepository::getInstance().getUseProxy();
        std::string proxyLabel = "Use Proxy: " + std::string(currentProxy ? "ON ✓" : "OFF");
        brls::Box* proxyBtn = createSidebarItem(proxyLabel, [this](brls::View* v) {
            bool current = Data::IPRepository::getInstance().getUseProxy();
            Data::IPRepository::getInstance().setUseProxy(!current);
            this->renderSettingsView(); // Refresh to show updated state
            return true;
        });
        proxyBtn->setMarginBottom(30);
        inner->addView(proxyBtn);

        // --- INFO SECTIONS ---

        addSection(_("main/developer_info"), _("main/dev_desc"));
        addSection(_("main/instructions"), _("main/inst_desc"));
        addSection(_("main/github"), "https://github.com/DarkTube");
        addSection(_("main/changelog"), "- Initial Release\n- Custom YouTube TV Grid UI\n- MPV Hardware Accelerated Player\n- Settings Panel added");

        scroll->setContentView(inner);
        content->addView(scroll);

        return content;
    }

    brls::Box* HomeActivity::createFooterHints(bool isSettings) {
        brls::Box* footer = new brls::Box();
        footer->setAxis(brls::Axis::ROW);
        footer->setHeight(30); // Made more compact
        footer->setWidthPercentage(100);
        footer->setAlignItems(brls::AlignItems::CENTER);
        footer->setPadding(0, 20, 0, 20); // Reduced padding
        footer->setBackgroundColor(Theme::SurfaceDark);
        
        footer->addView(UIUtils::createHint(nvgRGB(50, 160, 60), "A", _("main/select")));
        footer->addView(UIUtils::createHint(nvgRGB(220, 180, 0), "B", _("main/toggle_sidebar")));
        
        if (isSettings) {
             footer->addView(UIUtils::createHint(nvgRGB(100, 100, 255), "X", _("main/edit")));
             footer->addView(UIUtils::createHint(nvgRGB(255, 60, 60), "Y", _("main/delete")));
        } else if (!isServerEmpty()) {
             footer->addView(UIUtils::createHint(nvgRGB(100, 100, 255), "X", _("main/change_server")));
        }

        return footer;
    }

    void HomeActivity::toggleSidebar() {
        sidebarVisible = !sidebarVisible;
        if (sidebarVisible) {
            sidebar->setVisibility(brls::Visibility::VISIBLE);
            sidebar->setWidth(320); // Restore width
            
            // Re-enable focus on children
            for (auto child : sidebar->getChildren()) {
                child->setFocusable(true);
            }

            if (miniLogo) miniLogo->setVisibility(brls::Visibility::GONE);
            
            // Re-focus the first item in sidebar 
            if (sidebar->getChildren().size() > 1) { // logo + nav items
                 brls::Application::giveFocus(sidebar->getChildren()[1]); 
            }
        } else {
            // Invisible still takes space occasionally in flex, but Width=0 solves that
            sidebar->setVisibility(brls::Visibility::GONE); 
            sidebar->setWidth(0); 

            // Disable focus on children to prevent accidental focus on "empty" area
            for (auto child : sidebar->getChildren()) {
                child->setFocusable(false);
            }

            if (miniLogo) miniLogo->setVisibility(brls::Visibility::VISIBLE);

            // Put focus on main content area
            brls::Application::giveFocus(mainContent);
        }
    }

    void HomeActivity::promptForNewIP() {
        brls::Logger::info("Prompting for New IP via IME...");
        
        brls::Application::getPlatform()->getImeManager()->openForText(
            [this](std::string text) {
                if (!text.empty()) {
                    auto servers = Data::IPRepository::getInstance().getSavedServers();
                    if (servers.size() >= 4) {
                        brls::Dialog* dialog = new brls::Dialog(_("main/server_limit_reached"));
                        dialog->addButton(_("hints/ok"), []() {});
                        dialog->open();
                        return;
                    }

                    Domain::ServerIP newIp;
                    newIp.id = std::to_string(brls::getCPUTimeUsec());
                    newIp.name = "My Server";
                    newIp.address = text;

                    Data::IPRepository::getInstance().addServer(newIp);
                    Data::IPRepository::getInstance().setActiveServer(newIp);

                    brls::Logger::info("IP added. Pushing HomeActivity again to reload layout.");
                    brls::Application::popActivity(); // Pop self
                    brls::Application::pushActivity(new HomeActivity()); // Reload
                }
            },
            _("main/enter_server_ip"),
            "", // Default empty
            255,
            "",
            0
        );
    }

    void HomeActivity::promptForEditIP(const Domain::ServerIP& server) {
        brls::Logger::info("Prompting for Edit IP via IME...");
        
        brls::Application::getPlatform()->getImeManager()->openForText(
            [this, server](std::string text) {
                if (!text.empty()) {
                    Domain::ServerIP updatedServer = server;
                    updatedServer.address = text;

                    Data::IPRepository::getInstance().updateServer(updatedServer);
                    this->renderSettingsView(); // Refresh settings view
                }
            },
            _("main/edit_server_ip"),
            server.address,
            255,
            "",
            0
        );
    }

    void HomeActivity::renderSettingsView() {
        // Replace main content with settings view
        brls::Box* rootBox = dynamic_cast<brls::Box*>(this->getContentView());
        if (rootBox && rootBox->getChildren().size() > 0) {
            brls::Box* split = dynamic_cast<brls::Box*>(rootBox->getChildren()[0]);
            if (split && split->getChildren().size() >= 2) {
                split->removeView(mainContent, true); // true to free memory
                mainContent = createSettingsView();
                split->addView(mainContent);

                // Update footer
                this->updateFooter(true);

                brls::Application::giveFocus(mainContent);
            }
        }
    }

    void HomeActivity::updateFooter(bool isSettings) {
        brls::Box* rootBox = dynamic_cast<brls::Box*>(this->getContentView());
        if (rootBox && rootBox->getChildren().size() >= 2) {
            brls::View* oldFooter = rootBox->getChildren()[1];
            rootBox->removeView(oldFooter, true);
            rootBox->addView(createFooterHints(isSettings));
        }
    }
} // namespace Presentation
} // namespace DarkTube
