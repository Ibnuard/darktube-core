#pragma once

#include <borealis.hpp>

namespace DarkTube {
namespace Presentation {

    class ServerListActivity : public brls::Activity {
    public:
        ServerListActivity();
        ~ServerListActivity() override = default;

        brls::View* createContentView() override;
    
    private:
        brls::Box* createEmptyStateView();
        brls::Box* createSavedServersView();

        void promptForNewIP();
    };

} // namespace Presentation
} // namespace DarkTube
