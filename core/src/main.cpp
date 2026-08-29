#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

enum class NodeState { Starting, Ready };

const char* state_name(NodeState state) {
    switch (state) {
        case NodeState::Starting:
            return "starting";
        case NodeState::Ready:
            return "ready";
    }
    return "unknown";
}

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

const NodeConfig* find_peer(const std::vector<NodeConfig>& peers, int node_id) {
    for (const NodeConfig& peer : peers) {
        if (peer.node_id == node_id) {
            return &peer;
        }
    }
    return nullptr;
}

bool peers_are_valid(const NodeConfig& local,
                     const std::vector<NodeConfig>& peers) {
    std::unordered_set<int> peer_ids;
    for (const NodeConfig& peer : peers) {
        if (!peer.is_valid() || peer.node_id == local.node_id ||
            !peer_ids.insert(peer.node_id).second) {
            return false;
        }
    }
    return true;
}

struct Node {
    NodeConfig config;
    std::vector<NodeConfig> peers;

    bool is_valid() const {
        return config.is_valid() && peers_are_valid(config, peers);
    }

    std::vector<std::string> peer_endpoints() const {
        std::vector<std::string> endpoints;
        for (const auto& peer : peers) {
            endpoints.push_back(peer.endpoint());
        }
        return endpoints;
    }
};

int main() {
    Node node{{1, "127.0.0.1", 5000}, {{2, "127.0.0.1", 5001}}};
    NodeState state = NodeState::Starting;
    if (!node.is_valid()) {
        std::cerr << "Invalid node configuration\n";
        return 1;
    }
    state = NodeState::Ready;
    const std::size_t cluster_size = node.peers.size() + 1;
    const std::size_t quorum_size = cluster_size / 2 + 1;
    std::cout << "Node " << node.config.node_id << " listening on "
              << node.config.endpoint() << " (" << state_name(state) << ")\n";
    std::cout << "Configured peers: " << node.peers.size() << '\n';
    std::cout << "Cluster size: " << cluster_size
              << ", quorum: " << quorum_size << '\n';
    for (const auto& endpoint : node.peer_endpoints()) {
        std::cout << "  Peer endpoint: " << endpoint << '\n';
    }
    if (const NodeConfig* peer = find_peer(node.peers, 2)) {
        std::cout << "Peer 2 endpoint: " << peer->endpoint() << '\n';
    }
    return 0;
}
