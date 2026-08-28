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

int main() {
    NodeConfig config{1, "127.0.0.1", 5000};
    std::vector<NodeConfig> peers{{2, "127.0.0.1", 5001}};
    NodeState state = NodeState::Starting;
    if (!config.is_valid()) {
        std::cerr << "Invalid node configuration\n";
        return 1;
    }
    std::unordered_set<int> peer_ids;
    for (const NodeConfig& peer : peers) {
        peers_valid = peers_valid && peer.is_valid() && peer.node_id != config.node_id;
        peers_valid = peers_valid && peer_ids.insert(peer.node_id).second;
    }
    if (!peers_valid) {
        std::cerr << "Invalid peer configuration\n";
        return 1;
    }
    state = NodeState::Ready;
    const std::size_t cluster_size = peers.size() + 1;
    const std::size_t quorum_size = cluster_size / 2 + 1;
    std::cout << "Node " << config.node_id << " listening on "
              << config.endpoint() << " (" << state_name(state) << ")\n";
    std::cout << "Configured peers: " << peers.size() << '\n';
    std::cout << "Cluster size: " << cluster_size
              << ", quorum: " << quorum_size << '\n';
    if (const NodeConfig* peer = find_peer(peers, 2)) {
        std::cout << "Peer 2 endpoint: " << peer->endpoint() << '\n';
    }
    return 0;
}
