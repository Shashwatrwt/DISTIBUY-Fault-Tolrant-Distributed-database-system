#include <iostream>
#include <string>

enum class NodeState { Starting, Ready };

struct NodeConfig {
    int node_id;
    std::string host;
    int port;

    bool is_valid() const {
        return node_id > 0 && !host.empty() && port > 0 && port <= 65535;
    }
};

int main() {
    NodeConfig config{1, "127.0.0.1", 5000};
    NodeState state = NodeState::Starting;
    if (!config.is_valid()) {
        std::cerr << "Invalid node configuration\n";
        return 1;
    }
    state = NodeState::Ready;
    const char* state_label = state == NodeState::Ready ? "ready" : "starting";
    std::cout << "Node " << config.node_id << " listening on "
              << config.host << ':' << config.port << " (" << state_label << ")\n";
    return 0;
}
