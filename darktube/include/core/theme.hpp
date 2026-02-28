#pragma once

#include <borealis.hpp>

// Define YouTube TV Dark theme colors and constants
namespace DarkTube {
namespace Theme {

    // Backgrounds
    const NVGcolor BackgroundDark = nvgRGB(15, 15, 15);     // Main app background
    const NVGcolor SurfaceDark = nvgRGB(33, 33, 33);        // Cards/Items background
    const NVGcolor SurfaceHover = nvgRGB(61, 61, 61);       // Highlighted item

    // Text
    const NVGcolor TextPrimary = nvgRGB(255, 255, 255);     // Main text
    const NVGcolor TextSecondary = nvgRGB(170, 170, 170);   // Subtitles, hints

    // Accents
    const NVGcolor AccentRed = nvgRGB(255, 0, 0);           // Selection borders or highlights

    // Overlay (for Player)
    const NVGcolor OverlayBackground = nvgRGBA(0, 0, 0, 180);

    // Apply global theme overrides to Borealis if needed
    inline void applyTheme() {
        brls::Theme::getLightTheme().addColor("brls/background", BackgroundDark);
        brls::Theme::getDarkTheme().addColor("brls/background", BackgroundDark);
        
        brls::Theme::getLightTheme().addColor("brls/text", TextPrimary);
        brls::Theme::getDarkTheme().addColor("brls/text", TextPrimary);

        brls::Theme::getLightTheme().addColor("brls/list/item_bg", SurfaceDark);
        brls::Theme::getDarkTheme().addColor("brls/list/item_bg", SurfaceDark);
    }

} // namespace Theme
} // namespace DarkTube
