#include <iostream>
#include <string>
#include <vector>

enum class NodeState { Starting, Ready };

struct NodeConfig {
    int node_id;
    std::string host;
    int port;

    bool is_valid() const {
        return node_id > 0 && !host.empty() && port > 0 && port <= 65535;
    }

    std::string endpoint() const {
        return host + ':' + std::to_string(port);
    }
};

int main() {
    NodeConfig config{1, "127.0.0.1", 5000};
    std::vector<NodeConfig> peers{{2, "127.0.0.1", 5001}};
    NodeState state = NodeState::Starting;
    if (!config.is_valid()) {
        std::cerr << "Invalid node configuration\n";
        return 1;
    }
    bool peers_valid = true;
    for (const NodeConfig& peer : peers) {
        peers_valid = peers_valid && peer.is_valid() && peer.node_id != config.node_id;
    }
    if (!peers_valid) {
        std::cerr << "Invalid peer configuration\n";
        return 1;
    }
    state = NodeState::Ready;
    const char* state_label = state == NodeState::Ready ? "ready" : "starting";
    std::cout << "Node " << config.node_id << " listening on "
              << config.endpoint() << " (" << state_label << ")\n";
    std::cout << "Configured peers: " << peers.size() << '\n';
    return 0;
}
