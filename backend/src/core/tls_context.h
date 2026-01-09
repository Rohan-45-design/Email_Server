#pragma once
#include <string>
#include <openssl/ssl.h>
#include <openssl/err.h>

class TlsContext {
public:
    static TlsContext& instance();
    bool init(const std::string& certFile, const std::string& keyFile);
    SSL* createSSL(int fd);
    SSL* createClientSSL(int fd); // For outbound connections

private:
    TlsContext() = default;
    ~TlsContext();
    SSL_CTX* ctx_ = nullptr;
    SSL_CTX* clientCtx_ = nullptr;
};
