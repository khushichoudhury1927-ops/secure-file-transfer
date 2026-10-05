#pragma once
#include <openssl/ssl.h>
#include <unistd.h>

// Owns an SSL_CTX and frees it automatically.
class SslContext {
public:
    explicit SslContext(const SSL_METHOD* method) : ctx_(SSL_CTX_new(method)) {}
    ~SslContext() { if (ctx_) SSL_CTX_free(ctx_); }
    SslContext(const SslContext&) = delete;
    SslContext& operator=(const SslContext&) = delete;
    SSL_CTX* get() const { return ctx_; }
    explicit operator bool() const { return ctx_ != nullptr; }
private:
    SSL_CTX* ctx_;
};

// Owns one TLS connection: shuts it down, frees it and closes the socket.
class SslConnection {
public:
    SslConnection(SSL_CTX* ctx, int fd) : fd_(fd), ssl_(SSL_new(ctx)) {
        if (ssl_) SSL_set_fd(ssl_, fd_);
    }
    ~SslConnection() {
        if (ssl_) { SSL_shutdown(ssl_); SSL_free(ssl_); }
        if (fd_ >= 0) ::close(fd_);
    }
    SslConnection(const SslConnection&) = delete;
    SslConnection& operator=(const SslConnection&) = delete;
    SSL* ssl() const { return ssl_; }
private:
    int fd_;
    SSL* ssl_;
};
