#include "../include/data/ip_repository.hpp"

namespace DarkTube {
namespace Data {

    IPRepository::IPRepository() {
        // Start completely empty based on user feedback to default to empty
        // m_activeServer = {"mock", "Mock Server", "192.168.1.100"};
        m_activeServer = {"", "", ""};
    }

    std::vector<Domain::ServerIP> IPRepository::getSavedServers() {
        return m_servers;
    }

    void IPRepository::addServer(const Domain::ServerIP& server) {
        m_servers.push_back(server);
    }

    void IPRepository::removeServer(const std::string& id) {
        for (auto it = m_servers.begin(); it != m_servers.end(); ++it) {
            if (it->id == id) {
                m_servers.erase(it);
                break;
            }
        }
    }

    void IPRepository::setActiveServer(const Domain::ServerIP& server) {
        m_activeServer = server;
    }

    Domain::ServerIP IPRepository::getActiveServer() const {
        return m_activeServer;
    }

} // namespace Data
} // namespace DarkTube
