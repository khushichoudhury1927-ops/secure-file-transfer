# System Architecture

## 1. Overview
A client-server system. The client reads a file and sends it in chunks over a TLS connection. The server stores it and checks its SHA-256 hash.

```mermaid
flowchart LR
    A[Client] -- TLS connection --> B[Server]
    A --> C[(Source file)]
    B --> D[(Received file)]
    A --> E[Checksum module]
    B --> E
```


## 2. Components and Responsibilities
| Component | Responsibility |
|---|---|
| Server | Listens on a port, accepts clients, starts a session |
| ServerSession | Handles one client: TLS handshake, receives chunks, verifies hash |
| Client | Connects, verifies certificate, sends file in chunks |
| Checksum | Computes SHA-256 of a file |
| Protocol | Defines the messages exchanged (header, chunk, hash, result) |

## 3. Data Structures
- FileHeader { std::string name; uint64_t size; uint64_t resumeOffset; }
- Chunk as a std::vector<char> of fixed size (64 KB)
- Hash as a 64-character hex std::string

## 4. Class Diagram
```mermaid
classDiagram
    class Server {
        -int port
        -SSL_CTX* ctx
        +start()
        +acceptClient()
    }
    class ServerSession {
        -SSL* ssl
        +handshake()
        +receiveFile()
        +verifyHash()
    }
    class Client {
        -std::string host
        -int port
        +connectToServer()
        +sendFile(path)
    }
    class Checksum {
        +sha256(path) string
    }
    Server --> ServerSession : creates
    ServerSession --> Checksum : uses
    Client --> Checksum : uses
```


## 5. Sequence Diagram
```mermaid
sequenceDiagram
    participant C as Client
    participant S as Server
    C->>S: TCP connect
    C->>S: TLS handshake
    S-->>C: Certificate
    C->>S: File header (name, size, hash)
    S-->>C: Resume offset
    loop each 64 KB chunk
        C->>S: Chunk
    end
    S->>S: Compute SHA-256
    S-->>C: Hash match / mismatch
```


## 6. State Machine Diagram (Client)
```mermaid
stateDiagram-v2
    [*] --> Idle
    Idle --> Connecting: start
    Connecting --> Handshake: TCP connected
    Handshake --> Sending: certificate verified
    Sending --> Sending: next chunk
    Sending --> Verifying: all chunks sent
    Verifying --> Done: hash matches
    Verifying --> Failed: hash mismatch
    Connecting --> Failed: error
    Sending --> Connecting: connection lost, resume
    Done --> [*]
    Failed --> [*]
```


## 7. Implementation Plan
1. TLS server and client connect.
2. Send a file in chunks.
3. Add SHA-256 check.
4. Add resume.
5. Tests and documentation.

## 8. Development Environment
Ubuntu (WSL), g++ (C++20), CMake, OpenSSL, Git and GitHub.

## 9. Branching Strategy
main holds stable work. Each stage is developed on a branch (stage3-design, stage4-prototype, ...) and merged into main when complete.
