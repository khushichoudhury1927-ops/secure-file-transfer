# Wire Protocol

All numbers are 8-byte big-endian unsigned integers. Everything travels inside one TLS connection.

## Client to server (header)
| Field | Size | Meaning |
|-------|------|---------|
| name length | 8 bytes | Length of the file name (1 to 255) |
| file name | name length | Name only, any path is stripped by the server |
| file size | 8 bytes | Total size of the file in bytes |
| SHA-256 | 32 bytes | Hash of the whole file |

## Server to client
| Field | Size | Meaning |
|-------|------|---------|
| resume offset | 8 bytes | Bytes already stored in received/NAME.part (0 for a new file) |

## Client to server (data)
The client skips the first offset bytes and sends the rest in chunks of up to 64 KB.

## Server to client (result)
| Reply | Meaning |
|-------|---------|
| OK | Hash matches, .part file renamed to the final name |
| NO | Hash mismatch, .part file deleted |

## Notes
- If the connection drops, the server keeps the .part file so the next attempt can resume.
- The server uses only the file name part of the path, so a name like ../x cannot escape the received/ folder.
