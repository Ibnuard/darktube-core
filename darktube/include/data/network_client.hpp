#pragma once

#include <string>
#include <vector>
#include <functional>
#include "../domain/models.hpp"
#include <borealis/core/singleton.hpp>

namespace DarkTube {
namespace Data {

    class NetworkClient : public brls::Singleton<NetworkClient> {
    public:
        NetworkClient();
        ~NetworkClient();

        using Callback = std::function<void(const std::vector<Domain::VideoItem>&, const std::string& nextPageToken, const std::string& error)>;
        using StreamCallback = std::function<void(const Domain::StreamInfo& info, const std::string& error)>;

        void getTrending(Callback cb, const std::string& pageToken = "");
        void search(const std::string& query, Callback cb, const std::string& pageToken = "");
        void getStream(const std::string& videoId, StreamCallback cb);
        void fetchImage(const std::string& url, std::function<void(const unsigned char* data, size_t size)> cb);

    private:
        std::string getBaseUrl();
        std::string performGet(const std::string& url);
    };

} // namespace Data
} // namespace DarkTube
