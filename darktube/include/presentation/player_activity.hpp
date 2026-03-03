#pragma once

#include <borealis.hpp>
#include "../domain/models.hpp"

namespace DarkTube {
namespace Presentation {

    class VideoPlayerView : public brls::Box {
    public:
        VideoPlayerView();
        void draw(NVGcontext* vg, float x, float y, float width, float height, brls::Style style, brls::FrameContext* ctx) override;
    };

    class PlayerOverlayView : public brls::Box {
    public:
        PlayerOverlayView(const Domain::StreamInfo& info);
        
        void onFocusGained() override;
        void onFocusLost() override;
        void toggleOSD(bool show);
        void frame(brls::FrameContext* ctx) override;

    private:
        brls::Box* topBar;
        brls::Box* bottomBar;
        brls::Box* progressFill;
        brls::Label* statusLabel;
        brls::Label* timeLabel;
        brls::ProgressSpinner* bufferingLoader;
        brls::Box* centerContainer;
        Domain::StreamInfo streamInfo;
        bool isPlaying = true;
        bool osdVisible = false;
        
        brls::Time osdLastTick = 0;
        const brls::Time osdTimeout = 3000; // 3 seconds

        brls::Box* createTopBar();
        brls::Box* createBottomBar();
        brls::Box* createProgressBar();
        brls::Box* createControlBar();
        
        void updatePlaybackInfo();
        std::string formatTime(double seconds);
        void openQualitySelector();
    };

    class PlayerActivity : public brls::Activity {
    public:
        PlayerActivity(const Domain::StreamInfo& info);
        ~PlayerActivity() override;

        brls::View* createContentView() override;

    private:
        Domain::StreamInfo streamInfo;
    };

} // namespace Presentation
} // namespace DarkTube
