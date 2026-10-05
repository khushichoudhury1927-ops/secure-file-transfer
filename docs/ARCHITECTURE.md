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
| io.h | Helpers that send and read exact byte counts over TLS; the message layout is in docs/PROTOCOL.md |
| raii.h | SslContext and SslConnection classes that free TLS objects and close sockets automatically |

## 3. Data Structures
- Header fields, sent in this order: name length (uint64_t), file name (std::string), file size (uint64_t), SHA-256 (Checksum::Hash, a std::array<unsigned char, 32>).
- Resume offset (uint64_t) sent back by the server.
- Chunk buffer: std::vector<char> of 64 KB.
- SslContext and SslConnection (raii.h): own the OpenSSL objects and free them automatically.

## 4. Class Diagram
```mermaid
classDiagram
    class Server {
        -int port_
        -int listenFd_
        -SslContext ctx_
        +run() int
        -loadCertificate() bool
        -openSocket() bool
    }
    class ServerSession {
        -SslConnection conn_
        -string name_
        -uint64_t size_
        -uint64_t offset_
        +run()
        -receiveHeader() bool
        -receiveData() bool
        -verifyAndFinish()
    }
    class Client {
        -int port_
        -SslContext ctx_
        +sendFile(path) int
    }
    class Checksum {
        +sha256File(path, out)$ bool
        +toHex(hash)$ string
    }
    class SslContext
    class SslConnection
    Server --> ServerSession : creates one per client
    Server *-- SslContext
    ServerSession *-- SslConnection
    ServerSession ..> Checksum : verifies hash
    Client ..> Checksum : computes hash
    Client *-- SslContext
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
