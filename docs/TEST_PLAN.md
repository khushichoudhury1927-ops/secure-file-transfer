# Test Plan

## 1. Strategy
- Unit level: SHA-256 hashing is checked by comparing the client and server hashes and sha256sum.
- Integration level: client and server talk over a real TLS connection on localhost.
- System level: tests/run_tests.sh starts a server, runs the clients, and compares files byte by byte with cmp.

## 2. Test Cases
| ID | Requirement | Test | Expected result | Status |
|----|-------------|------|-----------------|--------|
| T1 | FR4, FR5 | Send a small text file | Received file is identical | Pass |
| T2 | FR4, NFR3 | Send a 5 MB random file | Received file is identical | Pass |
| T3 | FR4 | Send an empty file | Empty file is created | Pass |
| T4 | FR8 | Interrupt a transfer with SFT_STOP_AFTER | A .part file stays on the server | Pass |
| T5 | FR8 | Run the client again | Client resumes from a non-zero offset | Pass |
| T6 | FR6, FR7 | Check the resumed file | File is identical, hashes match | Pass |
| T7 | FR9 | Send a file that does not exist | Client reports an error and exits non-zero | Pass |
| T8 | FR2, FR3, NFR1 | Connect with a valid certificate | TLS handshake succeeds | Pass |

## 3. How to Run
    bash scripts/gen_cert.sh
    cmake -S . -B build
    cmake --build build -j"$(nproc)"
    bash tests/run_tests.sh

## 4. Results
All 7 automated tests pass. TLS handshake (T8) is verified by every transfer.

## 5. Defects Found and Fixed
- Early version replied OK without checking the hash. Fixed by adding the SHA-256 check on the server.
- Resume was missing, so a dropped connection restarted from zero. Fixed with .part files and an offset reply.

## 6. Known Limitations
- Handles one client at a time.
- Server certificate is self-signed and the client trusts it from a local file.
- Only uploads (client to server) are supported.
