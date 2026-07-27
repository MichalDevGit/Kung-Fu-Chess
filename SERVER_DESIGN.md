# Cloud Server Architecture Design

## System Overview

This cloud server architecture is designed to support a massive-scale real-time gaming
environment capable of handling 100 million registered users, 10 million concurrent
active players globally, and high-frequency move updates (every 2 seconds per player).
The system utilizes a distributed, containerized cloud infrastructure to ensure high
availability, fault tolerance, and seamless scalability.

## Key Architectural Components

### Global Load Balancer

- **Function:** Acts as the primary entry point for all incoming client connections
  from around the world. It distributes incoming WebSocket traffic evenly across
  multiple regional server instances and routes players to the nearest available
  server to minimize network latency.
- **Problems Solved:** Prevents single-point bottlenecks at the network entry layer,
  distributes global traffic efficiently, and ensures high availability through
  redundant routing.

### Hybrid Application Server Instances (Docker Containers & Multi-threading)

- **Function:** The core game server code is containerized using Docker, allowing it
  to run as identical, scalable replicas across multiple physical cloud nodes. Within
  each Docker container, an asynchronous multi-threading model handles concurrent
  client connections and local room logic.
- **Problems Solved:** Isolates faults so that a crash in one container does not
  affect players on other servers; optimizes CPU core utilization via multi-threading;
  and avoids the high networking latency and architectural complexity of
  fine-grained microservices.

### Kubernetes (Orchestration & Auto-Scaling)

- **Function:** Acts as the automated manager for all Docker containers, monitoring
  resource usage and container health in real-time.
- **Problems Solved:** Automates auto-scaling by spinning up new server instances
  during traffic spikes and scaling them down when load decreases; provides
  self-healing capabilities by instantly replacing crashed containers; and handles
  zero-downtime rolling updates.

### Redis (In-Memory Data Store & Matchmaking Queue)

- **Function:** A high-speed, in-memory database acting as a centralized
  synchronization layer for all server instances. It manages the matchmaking queue,
  active room states, and temporary player data.
- **Problems Solved:** Overcomes the isolation barrier between different Docker
  containers, enabling all server instances to share a unified, real-time view of
  player queues and active game sessions without hitting slower disk storage.

### PostgreSQL with Write Queuing (Primary Database)

- **Function:** The long-term relational data store for registered user profiles and
  historical scores, paired with a fast message queue for write operations.
- **Problems Solved:** Prevents database lockups and mutex bottlenecks under massive
  write loads. By offloading non-critical updates to a temporary queue, the system
  maintains high availability and smooth gameplay performance while eventual
  consistency ensures data is safely written to the main database.

## Resiliency and Fault Tolerance

### Graceful Disconnections and Reconnection Windows

If a player's internet connection drops momentarily, the server grants a grace period
(e.g., 10 seconds). The player can reconnect through the load balancer to a new
container, and their game session is restored seamlessly using the centralized Redis
state.

### Master-Replica Database and Container Redundancy

Redis operates in a Master-Replica configuration to ensure that if a primary memory
node fails, a backup node takes over instantly. Similarly, container failures are
automatically mitigated by Kubernetes spinning up fresh instances within seconds.
