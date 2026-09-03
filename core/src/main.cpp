#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>
#include <unordered_map>
#include <ctime>
#include <cstdlib>

enum class NodeState { Starting, Ready };
enum class NodeDomain { Users, Products, Orders };
enum class TxState { Begin, Commit, Abort };

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

const char* tx_state_name(TxState state) {
    switch (state) {
        case TxState::Begin:
            return "begin";
        case TxState::Commit:
            return "commit";
        case TxState::Abort:
            return "abort";
    }
    return "unknown";
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

// --- NEW: real liveness tracking, separate from static config validity ---
// This models an actual heartbeat mechanism. In the current single-process
// prototype there is no real network heartbeat yet, so we track "last seen"
// as a timestamp that a future heartbeat thread/loop would update whenever
// a peer responds. For now it is initialized to "now" at startup so a freshly
// started node is considered alive, and a node counts as unhealthy once too
// much time has passed since its last heartbeat.
struct LivenessInfo {
    long last_heartbeat_time = 0;
    bool has_heartbeat = false;

    void mark_alive(long now) {
        last_heartbeat_time = now;
        has_heartbeat = true;
    }
};

const long HEARTBEAT_TIMEOUT_SECONDS = 10;

bool is_alive(const LivenessInfo& liveness, long now) {
    if (!liveness.has_heartbeat) {
        return false;
    }
    return (now - liveness.last_heartbeat_time) <= HEARTBEAT_TIMEOUT_SECONDS;
}
// --- END NEW ---

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

    bool contains(const std::string& key) const {
        return data.find(key) != data.end();
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

    std::string last_key() const {
        return entries.empty() ? "" : entries.back().key;
    }
};

bool can_failover(const ReplicaMap& replicas, NodeDomain domain) {
    return replicas.replication.find(domain) != replicas.replication.end();
}

// --- CHANGED: node_healthy now checks real liveness, not just config validity ---
bool node_healthy(const Node& node, const LivenessInfo& liveness, long now) {
    return node.is_valid() && is_alive(liveness, now);
}
// --- END CHANGED ---

NodeDomain route_for(const ReplicaMap& replicas, NodeDomain request) {
    auto it = replicas.replication.find(request);
    return (it != replicas.replication.end()) ? it->second : request;
}

bool owns_domain(const ClusterMetadata& metadata, NodeDomain domain, int node_id) {
    const NodeConfig* candidate = metadata.find_by_domain(domain);
    return candidate != nullptr && candidate->node_id == node_id;
}

bool quorum_available(std::size_t responsive_nodes, std::size_t quorum_size) {
    return responsive_nodes >= quorum_size;
}

TxState finish_transaction(TxState state, bool quorum_ready) {
    return state == TxState::Begin && quorum_ready ? TxState::Commit : state;
}

bool log_ready_for_recovery(const TransactionLog& log) {
    return log.size() > 0;
}

// --- NEW: build the full cluster topology once, then derive "this node" + its peers from an id ---
ClusterMetadata build_cluster_metadata() {
    return ClusterMetadata{{
        {1, "127.0.0.1", 5433, NodeDomain::Users},
        {2, "127.0.0.1", 5434, NodeDomain::Products},
        {3, "127.0.0.1", 5435, NodeDomain::Orders}
    }};
}

Node build_node(const ClusterMetadata& metadata, int node_id) {
    NodeConfig local{};
    std::vector<NodeConfig> peers;

    for (const auto& candidate : metadata.nodes) {
        if (candidate.node_id == node_id) {
            local = candidate;
        }
    }
    for (const auto& candidate : metadata.nodes) {
        if (candidate.node_id != node_id) {
            peers.push_back(candidate);
        }
    }
    return Node{local, peers};
}
// --- END NEW ---

int main(int argc, char* argv[]) {
    // --- NEW: node id now comes from the command line instead of being hardcoded ---
    // Usage: ./main <node_id>   e.g. ./main 1   ./main 2   ./main 3
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <node_id>\n";
        std::cerr << "  node_id must be 1 (Users), 2 (Products), or 3 (Orders)\n";
        return 1;
    }

    int node_id = std::atoi(argv[1]);
    if (node_id < 1 || node_id > 3) {
        std::cerr << "Invalid node_id: " << argv[1] << " (must be 1, 2, or 3)\n";
        return 1;
    }

    ClusterMetadata metadata = build_cluster_metadata();
    Node node = build_node(metadata, node_id);
    // --- END NEW ---

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
    TxState tx = TxState::Begin;

    if (!node.is_valid()) {
        std::cerr << "Invalid node configuration\n";
        return 1;
    }
    state = NodeState::Ready;

    // --- NEW: mark this node alive now that startup succeeded ---
    long now = (long)std::time(nullptr);
    LivenessInfo liveness;
    liveness.mark_alive(now);
    // --- END NEW ---

    const std::size_t cluster_size = node.peers.size() + 1;
    const std::size_t quorum_size = cluster_size / 2 + 1;
    const std::size_t nodes_after_failure = cluster_size - 1;
    tx = finish_transaction(tx, quorum_available(cluster_size, quorum_size));

    std::cout << node.summary() << '\n';
    std::cout << "State: " << state_name(state) << '\n';
    std::cout << "Cluster: " << cluster_size << " nodes, quorum: " << quorum_size << '\n';
    std::cout << "Quorum status: " << (quorum_available(cluster_size, quorum_size) ? "available" : "unavailable") << '\n';
    std::cout << "After one failure: " << (quorum_available(nodes_after_failure, quorum_size) ? "available" : "unavailable") << '\n';
    std::cout << "Domain: " << domain_name(node.config.domain) << '\n';
    std::cout << "Store: " << store.size() << " items\n";
    std::cout << "  db_version: " << store.get("db_version") << '\n';
    std::cout << "  replication_factor: " << store.get("replication_factor") << '\n';
    std::cout << "Store integrity: " << (store.contains("db_version") ? "valid" : "missing") << '\n';
    std::cout << "Transaction log: " << log.size() << " entries\n";
    std::cout << "Recovery log model: " << (log_ready_for_recovery(log) ? "ready" : "empty") << '\n';
    std::cout << "Latest log key: " << log.last_key() << '\n';

    for (const auto& endpoint : node.peer_endpoints()) {
        std::cout << "  Peer endpoint: " << endpoint << '\n';
    }
    if (const NodeConfig* products = metadata.find_by_domain(NodeDomain::Products)) {
        std::cout << "Products node: " << products->endpoint() << '\n';
    }
    if (can_failover(replicas, NodeDomain::Users)) {
        std::cout << "Modeled failover route: " << domain_name(replicas.replication[NodeDomain::Users]) << '\n';
    }

    // --- CHANGED: heartbeat model now reflects real liveness check, not just config validity ---
    std::cout << "Heartbeat model: " << (node_healthy(node, liveness, now) ? "healthy" : "unhealthy") << '\n';
    // --- END CHANGED ---

    std::cout << "Modeled coordinator failover route: " << domain_name(route_for(replicas, NodeDomain::Users)) << '\n';
    std::cout << "Transaction state model: " << tx_state_name(tx) << '\n';
    std::cout << "Domain ownership: " << (owns_domain(metadata, node.config.domain, node.config.node_id) ? "owned" : "not-owned") << '\n';

    return 0;
}