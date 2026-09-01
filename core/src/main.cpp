#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>
#include <unordered_map>
#include <ctime>

enum class NodeState { Starting, Ready };
enum class NodeDomain { Users, Products, Orders };

const char* state_name(NodeState state) {
    switch (state) {
        case NodeState::Starting:
            return "starting";
        case NodeState::Ready:
            return "ready";
    }
    return "unknown";
}

const char* domain_name(NodeDomain domain) {
    switch (domain) {
        case NodeDomain::Users:
            return "Users";
        case NodeDomain::Products:
            return "Products";
        case NodeDomain::Orders:
            return "Orders";
    }
    return "Unknown";
}

struct NodeConfig {
    int node_id;
    std::string host;
    int port;
    NodeDomain domain;

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

    std::string summary() const {
        std::string result = "Node " + std::to_string(config.node_id) +
                             " (" + config.endpoint() + ") with " +
                             std::to_string(peers.size()) + " peer(s)";
        return result;
    }
};

struct ClusterMetadata {
    std::vector<NodeConfig> nodes;

    const NodeConfig* find_by_domain(NodeDomain domain) const {
        for (const auto& candidate : nodes) {
            if (candidate.domain == domain) {
                return &candidate;
            }
        }
        return nullptr;
    }
};

struct ReplicaMap {
    std::unordered_map<NodeDomain, NodeDomain> replication;

    void add(NodeDomain owner, NodeDomain replica) {
        replication[owner] = replica;
    }
};

struct Store {
    std::unordered_map<std::string, std::string> data;

    void set(const std::string& key, const std::string& value) {
        data[key] = value;
    }

    std::string get(const std::string& key) const {
        auto it = data.find(key);
        return (it != data.end()) ? it->second : "";
    }

    std::size_t size() const {
        return data.size();
    }
};

struct LogEntry {
    std::string key;
    std::string value;
    long timestamp;
};

struct TransactionLog {
    std::vector<LogEntry> entries;

    void record(const std::string& key, const std::string& value) {
        entries.push_back({key, value, (long)std::time(nullptr)});
    }

    std::size_t size() const {
        return entries.size();
    }
};

bool can_failover(const ReplicaMap& replicas, NodeDomain domain) {
    return replicas.replication.find(domain) != replicas.replication.end();
}

int main() {
    ClusterMetadata metadata{{
        {1, "127.0.0.1", 5001, NodeDomain::Users},
        {2, "127.0.0.1", 5002, NodeDomain::Products},
        {3, "127.0.0.1", 5003, NodeDomain::Orders}
    }};

    Node node{{1, "127.0.0.1", 5001, NodeDomain::Users},
              {{2, "127.0.0.1", 5002, NodeDomain::Products},
               {3, "127.0.0.1", 5003, NodeDomain::Orders}}};
    Store store;
    TransactionLog log;
    ReplicaMap replicas;
    replicas.add(NodeDomain::Users, NodeDomain::Products);
    replicas.add(NodeDomain::Products, NodeDomain::Orders);
    replicas.add(NodeDomain::Orders, NodeDomain::Users);
    store.set("db_version", "1.0");
    log.record("db_version", "1.0");
    store.set("replication_factor", "2");
    log.record("replication_factor", "2");
    NodeState state = NodeState::Starting;
    if (!node.is_valid()) {
        std::cerr << "Invalid node configuration\n";
        return 1;
    }
    state = NodeState::Ready;
    const std::size_t cluster_size = node.peers.size() + 1;
    const std::size_t quorum_size = cluster_size / 2 + 1;
    std::cout << node.summary() << '\n';
    std::cout << "State: " << state_name(state) << '\n';
    std::cout << "Cluster: " << cluster_size << " nodes, quorum: " << quorum_size << '\n';
    std::cout << "Domain: " << domain_name(node.config.domain) << '\n';
    std::cout << "Store: " << store.size() << " items\n";
    std::cout << "  db_version: " << store.get("db_version") << '\n';
    std::cout << "  replication_factor: " << store.get("replication_factor") << '\n';
    std::cout << "Transaction log: " << log.size() << " entries\n";
    for (const auto& endpoint : node.peer_endpoints()) {
        std::cout << "  Peer endpoint: " << endpoint << '\n';
    }
    if (const NodeConfig* products = metadata.find_by_domain(NodeDomain::Products)) {
        std::cout << "Products node: " << products->endpoint() << '\n';
    }
    if (can_failover(replicas, NodeDomain::Users)) {
        std::cout << "Failover route: " << domain_name(replicas.replication[NodeDomain::Users]) << '\n';
    }
    return 0;
}
