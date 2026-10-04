# Project Requirements Document (PRD)

## 1. Overview
A C++ command-line client and server on Linux that transfers a file over TLS, verifies it with SHA-256, and can resume an interrupted transfer.

## 2. Functional Requirements
- FR1: The server listens on a user-given port.
- FR2: The client connects to the server over TLS.
- FR3: The client verifies the server certificate.
- FR4: The client sends a file in fixed-size chunks.
- FR5: The server saves the received file.
- FR6: The client computes a SHA-256 hash and sends it.
- FR7: The server compares hashes and reports success or failure.
- FR8: If a transfer stops, the client resumes from the last received byte.
- FR9: Both programs print clear progress and error messages.

## 3. Non-Functional Requirements
- NFR1 Security: all data is encrypted in transit.
- NFR2 Reliability: a corrupted file is always detected.
- NFR3 Performance: large files are sent in chunks, not loaded fully into memory.
- NFR4 Portability: builds on Ubuntu with CMake.
- NFR5 Maintainability: code split into clear modules, documented.

## 4. Modules
- Server (listening, TLS, session handling)
- Client (connect, send file)
- Checksum (SHA-256)
- Common (protocol messages, helpers)

## 5. Deliverables
Source code, PRD, architecture and UML diagrams, test plan, final report, GitHub repository.

## 6. Timeline
- Stage 2: PRD and plan
- Stage 3: design and diagrams
- Stage 4: TLS connection and basic transfer
- Stage 5: checksum, resume, testing
- Stage 6: final demo and report
