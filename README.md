# ShardCore

ShardCore is a fault-tolerant, sharded distributed database system built as a learning-first project for understanding distributed systems, database internals, replication, sharding, and cluster coordination in real code.

The project is built in stages, with a strong emphasis on learning the underlying principles instead of copy-pasting a finished system. It is structured as a practical distributed database design with:

- a C++ core cluster and storage engine
- a Node.js/Express API layer
- a React frontend
- an e-commerce demonstration domain
- incremental implementation of real distributed database behaviors

## Project goal

The system aims to model a distributed database that can:

- split data across shards
- replicate writes across nodes
- keep the system available during partial failures
- coordinate cluster state and majorities
- expose a clean API for client access
- demonstrate these ideas through an e-commerce application

## Architecture overview

### 1. C++ distributed database core

This is the system kernel. It contains the node model, cluster topology, sharding logic, replication logic, quorum checks, and low-level distributed coordination primitives.

Current implementation status:

- node identity configuration
- peer validation
- node lifecycle states
- cluster size and quorum calculation
- peer endpoint lookup
- in-memory store
- transaction log tracking

### 2. Node.js/Express API layer

This layer exposes the database to clients through HTTP endpoints. It will mediate requests for:

- product reads and writes
- inventory management
- order creation and fulfillment
- cluster health checks
- database metadata and routing information

### 3. React frontend

The frontend is a user-facing e-commerce interface that demonstrates how the database behaves in production-like workflows. It should provide screens for:

- catalog browsing
- product detail views
- shopping cart flows
- order placement
- inventory and stock status
- cluster status dashboards for observing node health and replication

### 4. E-commerce demo domain

The application is built around an online retail workflow to make the distributed database concepts concrete:

- users browse products
- product data is sharded by category or keyspace
- inventory is tracked per shard
- orders are written with consistency checks
- stock movements are coordinated across replicas
- node failures can be simulated and recovered from

## Core distributed systems concepts being implemented

This project is designed to learn and implement the following ideas in a practical way:

- sharding and partitioning
- replication and leader-follower design
- quorum-based consistency decisions
- fault detection and failover logic
- read/write coordination across nodes
- cluster membership and peer validation
- persistence and transaction logging
- API-to-storage separation
- client-visible application behavior over a distributed backend

## Current repository state

The repository currently contains the early C++ foundation for the cluster layer. The core prototype validates local node configuration, cluster topology, peer constraints, quorum sizing, in-memory store behavior, and transaction logging.

This is intentionally small and incremental, and it is meant to evolve toward the full distributed system architecture described above.

## Repository structure

```text
ShardCore/
├── README.md
├── core/
│   └── src/
│       └── main.cpp
├── api/                  # planned Node.js/Express service
├── frontend/             # planned React app
├── docs/                 # planned architecture and design notes
└── scripts/              # planned setup and test scripts
```

## Current local build and run

The current C++ prototype can be compiled and run with MinGW:

```powershell
g++ -Wall -Wextra core/src/main.cpp -o core/src/main.exe
.\core\src\main.exe
```

Example output currently shows a node on `127.0.0.1:5000` with a peer at `127.0.0.1:5001`, cluster quorum calculation, and a basic in-memory data store.

## Development direction

The roadmap is intentionally incremental:

1. define node and cluster topology
2. implement sharding model
3. add replication and commit behavior
4. add failure detection and recovery
5. build API and client interfaces
6. add frontend e-commerce flows
7. connect database logic to real application requests

## License

No license has been selected yet.

## Contributing

Contributions are welcome as the project grows. For now, the work is focused on learning by building the system incrementally and validating each step with small code changes.
