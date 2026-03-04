#pragma once

#include <string>
#include <vector>

namespace DarkTube {
namespace Domain {

    struct ServerIP {
        std::string id;
        std::string name;
        std::string address;
    };

    struct VideoItem {
        std::string id;
        std::string title;
        std::string author;
        std::string thumbnailUrl;
        std::string thumbnailUrlMedium;
        std::string streamUrl;
        std::string description;
        std::string views;
        std::string date;
    };

    struct StreamFormat {
        std::string formatId;
        std::string resolution;
        std::string url;
        std::string proxyUrl;
        std::string quality;
        std::string type;
    };

    struct StreamInfo {
        std::string title;
        std::string url; // default url
        std::string proxyUrl;
        std::string audioUrl;
        std::string audioProxyUrl;
        std::string thumbnailUrl;
        int duration = 0;
        std::vector<StreamFormat> formats;
    };

} // namespace Domain
} // namespace DarkTube
