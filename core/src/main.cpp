#include <iostream>
#include <string>

struct NodeConfig {
    int node_id;
    std::string host;
    int port;
};

int main() {
    NodeConfig config{1, "127.0.0.1", 5000};
    std::cout << "Node " << config.node_id << " listening on "
              << config.host << ':' << config.port << '\n';
    return 0;
}
