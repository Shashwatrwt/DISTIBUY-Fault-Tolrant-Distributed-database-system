# OS + DBMS Project: Distributed, Fault-Tolerant E-Commerce Database System (with AI Shopping Assistant)

A distributed, fault-tolerant database system built from scratch in C++, not using MySQL/PostgreSQL, that powers a real e-commerce website and demonstrates how large-scale systems stay online, consistent, and fast even when individual servers crash.

## The core problem you're solving

A normal e-commerce app looks like this:

```text
Users → One Server → One Database
```

This works for a college demo with 10 users, but it has one fatal flaw: if that database crashes, the entire website goes down. This is a single point of failure.

The answer is to avoid one database and instead split data across multiple independent servers or nodes, and make copies of important data so if one node dies, another can immediately take over.

## The three layers of the system

The original two-layer model is expanded with an AI layer inserted between the application and database layers.

### 1. Application Layer

This is what users see and use:

- browse products, search, and filter
- add to cart and checkout
- login and register
- view order history
- admin dashboard to manage products, orders, and system health

### 2. AI Layer

This layer converts natural-language shopping requests into structured database queries.

Examples:

- "find me a wireless gaming mouse under ₹2000"
- "show me laptops with good battery life"

The AI is fully offline using Ollama, and it talks to the database only through the existing REST API and coordinator path. It is never a second entry point into the data.

### 3. Distributed Database Layer

This is the actual engineering core of the project:

- multiple database nodes running independently
- data split across nodes
- copies of data for safety
- a coordinator routing every request to the correct place
- transaction handling so operations do not leave data half-broken
- automatic detection of dead nodes and recovery when they return

## Core concepts

### Nodes

A node is an independent process that owns a slice of data. In the finalized design, each node owns a full business domain, not a numeric key range:

| Node | Owns |
|---|---|
| Node 1 | Users |
| Node 2 | Products & Inventory |
| Node 3 | Orders & Payments |

All three can run on one laptop as separate processes on localhost ports such as 5001, 5002, and 5003.

### Partitioning

Instead of one giant table spanning one machine, each table lives on one node. This spreads storage and request load, and a problem in one domain does not bring down the entire system.

### Replication

Partitioning alone is risky. If one node fails, data becomes unreachable. So each node's data is replicated to another node in a ring:

```text
Node 1 (Users) → replicated to → Node 2
Node 2 (Products) → replicated to → Node 3
Node 3 (Orders) → replicated to → Node 1
```

If Node 2 fails, Node 3 already has a copy of Products data and can take over.

### Query Coordinator

The frontend and application layer never talk directly to nodes. They talk only to the Coordinator.

The coordinator:

- knows which node owns which data
- routes requests to the right node
- runs distributed transactions across multiple nodes
- sends heartbeat checks
- triggers failover when a node dies

This hides the complexity from the application layer.

## Concurrency control

A key issue is the last-item-in-stock problem.

Without protection:

```text
Both read stock = 1
Both think it is available
Both buy
Stock becomes -1
```

The solution is Two-Phase Locking (2PL): whoever touches the row first acquires a lock; others wait until it is released.

## Transactions

Placing an order is not one operation. It is multiple steps, such as:

- create order
- reduce stock
- process payment

These must succeed together or fail together. The implementation uses BEGIN → ... → COMMIT and ROLLBACK when necessary.

## Distributed transactions and 2PC

Because an order may span multiple nodes, a single purchase can involve multiple nodes. Two-Phase Commit (2PC) ensures both nodes agree before finalization:

### Phase 1: Prepare

The coordinator asks each node if it is ready to commit.

### Phase 2: Commit or Abort

If all say yes, the coordinator tells them to commit. If any says no, everyone rolls back.

This prevents half-finished updates.

## Failure detection, failover, and recovery

- detection: the coordinator pings nodes using heartbeats
- failover: traffic is redirected to the replica of a failed node
- recovery: when a node restarts, it replays its write-ahead log and syncs missed updates before returning to service

This complete lifecycle is a major demo point: kill a node, watch failover, restart it, watch recovery.

## AI Shopping Assistant

### User flow

The website can include an Ask AI input:

```text
"Find me a wireless gaming mouse under ₹2000 with good battery life"
```

The AI converts it into a structured request like:

```python
class ProductSearchQuery(BaseModel):
    category: str | None = None
    wireless: bool | None = None
    max_price: float | None = None
    min_battery_life: str | None = None
    confidence: float = 1.0
    clarifying_question: str | None = None
```

The AI does not touch the database directly. It calls the existing REST API and the coordinator path, preserving the architecture.

### Ambiguity handling

If the request is vague, the AI should ask a clarifying question instead of guessing. Example:

```text
"Do you have a budget in mind? Any brand preference?"
```

The frontend shows this as a follow-up chat bubble. Only after the request is clear does the AI service call the product search endpoint.

### API pattern

```json
POST /api/products/search
{
  "category": "gaming mouse",
  "wireless": true,
  "max_price": 2000
}
```

This is functionally the same as filtering products manually in the UI. The coordinator and nodes do not know AI was involved.

### Why it fits the architecture

- reuses the same product search flow
- keeps the database as the source of truth
- avoids creating a second bypass path into the data layer
- works fully offline using local LLMs
- does not create a new single point of failure

## Ollama setup for local AI

Recommended models:

| RAM | Model | Notes |
|---|---|---|
| 8GB | llama3.2:3b or phi3:mini | fast and useful |
| 16GB | llama3.1:8b | strong balance |
| 16GB+ | qwen2.5:14b | stronger but slower |

```bash
ollama pull llama3.1:8b
ollama run llama3.1:8b "Hello, respond with just OK"
```

```python
import ollama

response = ollama.chat(
    model="llama3.1:8b",
    messages=[{"role": "user", "content": prompt}],
    format="json"
)
```

A strict validation layer is required because local models may return malformed JSON.

## OS concepts reflected in the project

| OS topic | Where it appears |
|---|---|
| Processes | each node, coordinator, and API server is a separate process |
| Threads | nodes handle multiple client requests concurrently |
| Synchronization | mutexes/locks prevent race conditions |
| IPC | nodes and coordinator communicate via TCP sockets |
| Deadlock | could arise when transactions wait for locks |
| File management | each node stores data in its own files |

## Tech stack

- C++ for the database engine
- TCP sockets for inter-process communication
- Node.js/Express for the REST API layer
- React for the frontend
- Python + Pydantic + Ollama for the AI layer
- file-based storage instead of MySQL/PostgreSQL

## Why this project is valuable

This project touches the same skills backend and infrastructure interviews test:

- distributed systems
- concurrency
- networking
- transaction management
- fault tolerance
- failover design

With the AI layer, it also adds applied AI engineering: structured output, ambiguity handling, and offline local LLM integration.

## What is intentionally not being built first

The advanced items are optional and only if time permits:

- automatic leader election
- dynamic sharding
- multi-machine deployment
- complex recovery optimizations

The MVP is already serious and defensible:

- 3 nodes
- coordinator
- TCP communication
- partitioning
- replication
- distributed transactions
- concurrency control
- failure detection and failover

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
├── scripts/              # planned setup/test scripts
└── .gitignore
```

## Current local build and run

The current C++ prototype can be built and run with MinGW:

```powershell
g++ -Wall -Wextra core/src/main.cpp -o core/src/main.exe
.\core\src\main.exe
```

The current prototype demonstrates a local node and peer setup, quorum logic, server metadata, and an in-memory store.

## Development roadmap

1. define node and cluster topology
2. implement sharding model
3. add replication and commit behavior
4. add failure detection and recovery
5. build the API layer
6. build the frontend and e-commerce flows
7. integrate the AI shopping assistant
8. connect the database logic to real user actions

## License

No license has been selected yet.

## Contributing

Contributions are welcome as the project grows. For now, the work is focused on learning by building the system incrementally and validating each step with small code changes.
