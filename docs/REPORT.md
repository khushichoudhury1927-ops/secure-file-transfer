# Project Report: Secure File Transfer System (C++)

## 1. Summary
A command-line client and server written in C++20 for Linux. The client sends a file over TLS, the server verifies it with SHA-256, and an interrupted transfer can resume from where it stopped.

## 2. Process Followed
| Stage | Work | Evidence |
|-------|------|----------|
| 1 | Introduction, scope | README.md |
| 2 | Requirements and plan | docs/PRD.md |
| 3 | Architecture and UML | docs/ARCHITECTURE.md |
| 4 | TLS prototype | branch stage4-prototype |
| 5 | Chunked transfer, SHA-256, resume, tests | branch stage5-testing, docs/TEST_PLAN.md |
| 6 | Protocol, report, demo | docs/PROTOCOL.md, docs/DEMO.md, this report |

## 3. Design Summary
- Server class: opens the listening socket, loads the certificate, accepts clients and creates one ServerSession per client.
- ServerSession class: runs the TLS handshake, reads the header, replies with the resume offset, receives the data with progress and speed output, verifies the hash and renames the file.
- Client class: loads the server certificate, verifies it, sends the header and then the data with progress and speed output.
- Checksum class: streams a file through SHA-256 using OpenSSL.
- raii.h: SslContext and SslConnection own the OpenSSL objects and the socket and release them automatically.
- io.h: helpers that send and receive an exact number of bytes over TLS.

## 4. Linux and C++ Concepts Used
- Sockets (socket, bind, listen, accept, connect) and TCP.
- Files and streams, std::filesystem, binary I/O.
- Environment variables (SFT_STOP_AFTER used for testing).
- Shell scripting for certificate generation and tests.
- CMake build system, Git branches and commits.
- RAII-style resource handling, std::vector buffers, fixed-size chunking.

## 5. Results
- 7 automated tests pass (small file, 5 MB file, empty file, interrupted transfer, resume, identical result, missing file).
- A 5,000,000 byte file interrupted at 2,031,616 bytes resumed and sent only the remaining 2,968,384 bytes.
- Received files matched the originals byte for byte.

## 6. Limitations
- One client at a time.
- Upload only (no download).
- Self-signed certificate, trusted from a local file.
- If the source file changes between attempts, the final hash fails and the partial file is discarded.
- Tested on localhost only.

## 7. Future Improvements
- Handle several clients at once with a thread pool.
- Add downloads and a file listing command.
- Show a progress bar and transfer speed.
- Support remote hosts, a configurable host address, and password or key authentication.
- Add unit tests with a C++ test framework.
