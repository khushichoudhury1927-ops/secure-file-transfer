# Secure File Transfer System (C++)

## 1. Introduction
This project is a command-line client and server, written in C++ on Linux, that transfers files securely over a network. The server listens on a port, and the client connects and sends a file.

## 2. Problem Statement
Plain file transfer sends data without protection, so it can be read or changed on the way. A transfer that fails halfway also has to start again from the beginning. This wastes time and makes it hard to trust that the file arrived correctly.

## 3. Objective
- Encrypt all data in transit using TLS (OpenSSL).
- Check that the received file is identical to the original using a SHA-256 hash.
- Resume an interrupted transfer from where it stopped.

## 4. Scope
*In scope:* one client sending one file to one server, TLS encryption, chunked transfer, integrity check, resume, and Linux command-line usage.
*Out of scope:* user login, a graphical interface, and downloading files from the server.

## 5. Expected Outcome and Applications
A working client and server that send a file over an encrypted connection and confirm it arrived intact. It can be used for safe file sharing between two machines, such as backups or sending logs.

## 6. Technologies and Concepts Used
- C++ (object-oriented design, standard library)
- Linux (terminal, sockets, file handling, build tools)
- OpenSSL (TLS encryption and SHA-256)
- CMake and Git (build and version control)
