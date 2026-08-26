#include <iostream>
#include <string>

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
    if (!config.is_valid()) {
        std::cerr << "Invalid node configuration\n";
        return 1;
    }
    std::cout << "Node " << config.node_id << " listening on "
              << config.host << ':' << config.port << '\n';
    return 0;
}
