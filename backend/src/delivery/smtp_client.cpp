#include "delivery/smtp_client.h"
#include "core/logger.h"
#include "dns/dns_resolver.h"
#include "core/tls_context.h"
#include "core/platform_socket.h"
#include <sstream>
#include <chrono>
#include <vector>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <stdexcept>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

SmtpDeliveryClient& SmtpDeliveryClient::instance() {
    static SmtpDeliveryClient inst;
    return inst;
}

// Helper functions for cross-platform socket operations
static bool sendData(socket_t sock, SSL* ssl, const std::string& data) {
    if (ssl) {
        return SSL_write(ssl, data.c_str(), (int)data.length()) > 0;
    } else {
        return send(sock, data.c_str(), (int)data.length(), 0) != SOCKET_ERROR;
    }
}

static bool receiveLine(socket_t sock, SSL* ssl, std::string& line) {
    line.clear();
    char c;
    while (true) {
        int n;
        if (ssl) {
            n = SSL_read(ssl, &c, 1);
        } else {
            n = recv(sock, &c, 1, 0);
        }
        
        if (n <= 0) return false;
        if (c == '\n') break;
        if (c != '\r') line += c;
    }
    return true;
}

static bool readSmtpResponse(socket_t sock, SSL* ssl, std::vector<std::string>& response) {
    response.clear();
    std::string line;
    
    // Read first line
    if (!receiveLine(sock, ssl, line)) return false;
    response.push_back(line);
    
    // Check if it's a multi-line response (starts with 3-digit code followed by '-')
    if (line.length() >= 4 && line[3] == '-') {
        std::string expectedCode = line.substr(0, 3);
        while (true) {
            if (!receiveLine(sock, ssl, line)) return false;
            response.push_back(line);
            // Last line has space after code
            if (line.length() >= 4 && line.substr(0, 3) == expectedCode && line[3] == ' ') {
                break;
            }
        }
    }
    
    return true;
}

static void closeSocket(socket_t sock) {
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}

std::vector<std::string> SmtpDeliveryClient::lookupMX(const std::string& domain) {
    std::vector<std::string> mxHosts;
    
    try {
        // Use existing DNS resolver to get MX records
        // TODO: Implement proper MX record lookup using DNSResolver
        // For now, return the domain itself as fallback
        Logger::instance().log(LogLevel::Info,
            "Delivery: Looking up MX records for " + domain);
        
        // Placeholder: In production, use DNSResolver to get MX records
        // For now, assume domain itself is the mail server
        mxHosts.push_back(domain);
        
    } catch (const std::exception& ex) {
        Logger::instance().log(LogLevel::Error,
            "Delivery: MX lookup failed for " + domain + ": " + ex.what());
    }
    
    return mxHosts;
}

DeliveryResult SmtpDeliveryClient::connectAndDeliver(
    const std::string& mxHost,
    int port,
    const std::string& from,
    const std::string& to,
    const std::string& rawMessage
) {
    DeliveryResult result;
    
    socket_t sock = INVALID_SOCKET;
    SSL* ssl = nullptr;
    bool tlsActive = false;
    std::vector<std::string> response;
    
    try {
        // Resolve hostname
        struct addrinfo hints{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        
        struct addrinfo* addrInfo = nullptr;
        int res = getaddrinfo(mxHost.c_str(), std::to_string(port).c_str(), &hints, &addrInfo);
        if (res != 0 || !addrInfo) {
            result.errorMessage = "DNS resolution failed for " + mxHost + ": " + gai_strerror(res);
            result.retryAfterSeconds = 300; // Retry in 5 minutes
            return result;
        }
        
        // Create socket
        sock = socket(addrInfo->ai_family, addrInfo->ai_socktype, addrInfo->ai_protocol);
        if (sock == INVALID_SOCKET) {
            result.errorMessage = "Failed to create socket";
            result.retryAfterSeconds = 60;
            freeaddrinfo(addrInfo);
            return result;
        }
        
        // Set timeout
        struct timeval timeout;
        timeout.tv_sec = CONNECTION_TIMEOUT_SEC;
        timeout.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
        
        // Connect
        if (connect(sock, addrInfo->ai_addr, (int)addrInfo->ai_addrlen) == SOCKET_ERROR) {
            result.errorMessage = "Connection failed to " + mxHost;
            result.retryAfterSeconds = 300;
            closeSocket(sock);
            freeaddrinfo(addrInfo);
            return result;
        }
        
        freeaddrinfo(addrInfo);
        
        // Read greeting
        if (!readSmtpResponse(sock, nullptr, response) || response.empty()) {
            result.errorMessage = "Failed to read SMTP greeting";
            result.retryAfterSeconds = 60;
            closeSocket(sock);
            return result;
        }
        
        // Check greeting (should be 220)
        if (response[0].length() < 3 || response[0].substr(0, 3) != "220") {
            result.errorMessage = "Invalid SMTP greeting: " + response[0];
            result.retryAfterSeconds = 60;
            closeSocket(sock);
            return result;
        }
        
        // Send EHLO
        std::string ehlo = "EHLO mailserver.local\r\n";
        if (!sendData(sock, nullptr, ehlo)) {
            result.errorMessage = "Failed to send EHLO";
            result.retryAfterSeconds = 60;
            closeSocket(sock);
            return result;
        }
        
        // Read EHLO response
        if (!readSmtpResponse(sock, nullptr, response) || response.empty()) {
            result.errorMessage = "Failed to read EHLO response";
            result.retryAfterSeconds = 60;
            closeSocket(sock);
            return result;
        }
        
        if (response[0].length() < 3 || response[0].substr(0, 3) != "250") {
            result.errorMessage = "EHLO failed: " + response[0];
            result.retryAfterSeconds = 60;
            closeSocket(sock);
            return result;
        }
        
        // Check for STARTTLS capability
        bool supportsStartTls = false;
        for (const auto& line : response) {
            if (line.find("STARTTLS") != std::string::npos) {
                supportsStartTls = true;
                break;
            }
        }
        
        // If STARTTLS is supported, upgrade to TLS
        if (supportsStartTls) {
            std::string starttls = "STARTTLS\r\n";
            if (!sendData(sock, nullptr, starttls)) {
                result.errorMessage = "Failed to send STARTTLS";
                result.retryAfterSeconds = 60;
                closeSocket(sock);
                return result;
            }
            
            if (!readSmtpResponse(sock, nullptr, response) || response.empty()) {
                result.errorMessage = "Failed to read STARTTLS response";
                result.retryAfterSeconds = 60;
                closeSocket(sock);
                return result;
            }
            
            if (response[0].length() < 3 || response[0].substr(0, 3) != "220") {
                result.errorMessage = "STARTTLS rejected: " + response[0];
                result.retryAfterSeconds = 60;
                closeSocket(sock);
                return result;
            }
            
            // Upgrade to TLS
            ssl = TlsContext::instance().createClientSSL((int)sock);
            if (!ssl) {
                result.errorMessage = "Failed to create SSL context";
                result.retryAfterSeconds = 60;
                closeSocket(sock);
                return result;
            }
            
            // Perform TLS handshake
            if (SSL_connect(ssl) != 1) {
                result.errorMessage = "TLS handshake failed";
                result.retryAfterSeconds = 60;
                SSL_free(ssl);
                closeSocket(sock);
                return result;
            }
            
            // Verify certificate
            if (SSL_get_verify_result(ssl) != X509_V_OK) {
                result.errorMessage = "Certificate verification failed";
                result.retryAfterSeconds = 300;
                SSL_free(ssl);
                closeSocket(sock);
                return result;
            }
            
            tlsActive = true;
            
            // Send EHLO again after STARTTLS
            if (!sendData(sock, ssl, ehlo)) {
                result.errorMessage = "Failed to send EHLO after STARTTLS";
                result.retryAfterSeconds = 60;
                SSL_free(ssl);
                closeSocket(sock);
                return result;
            }
            
            if (!readSmtpResponse(sock, ssl, response) || response.empty()) {
                result.errorMessage = "Failed to read EHLO response after STARTTLS";
                result.retryAfterSeconds = 60;
                SSL_free(ssl);
                closeSocket(sock);
                return result;
            }
            
            if (response[0].length() < 3 || response[0].substr(0, 3) != "250") {
                result.errorMessage = "EHLO failed after STARTTLS: " + response[0];
                result.retryAfterSeconds = 60;
                SSL_free(ssl);
                closeSocket(sock);
                return result;
            }
        }
        
        // Send MAIL FROM
        std::string mailFrom = "MAIL FROM:<" + from + ">\r\n";
        if (!sendData(sock, ssl, mailFrom)) {
            result.errorMessage = "Failed to send MAIL FROM";
            result.retryAfterSeconds = 60;
            if (ssl) SSL_free(ssl);
            closeSocket(sock);
            return result;
        }
        
        if (!readSmtpResponse(sock, ssl, response) || response.empty()) {
            result.errorMessage = "Failed to read MAIL FROM response";
            result.retryAfterSeconds = 60;
            if (ssl) SSL_free(ssl);
            closeSocket(sock);
            return result;
        }
        
        if (response[0].length() < 3 || response[0].substr(0, 3) != "250") {
            result.errorMessage = "MAIL FROM rejected: " + response[0];
            result.permanentFailure = (response[0].length() >= 3 && response[0][0] == '5');
            result.retryAfterSeconds = result.permanentFailure ? 0 : 300;
            if (ssl) SSL_free(ssl);
            closeSocket(sock);
            return result;
        }
        
        // Send RCPT TO
        std::string rcptTo = "RCPT TO:<" + to + ">\r\n";
        if (!sendData(sock, ssl, rcptTo)) {
            result.errorMessage = "Failed to send RCPT TO";
            result.retryAfterSeconds = 60;
            if (ssl) SSL_free(ssl);
            closeSocket(sock);
            return result;
        }
        
        if (!readSmtpResponse(sock, ssl, response) || response.empty()) {
            result.errorMessage = "Failed to read RCPT TO response";
            result.retryAfterSeconds = 60;
            if (ssl) SSL_free(ssl);
            closeSocket(sock);
            return result;
        }
        
        if (response[0].length() < 3 || response[0].substr(0, 3) != "250") {
            result.errorMessage = "RCPT TO rejected: " + response[0];
            result.permanentFailure = (response[0].length() >= 3 && response[0][0] == '5');
            result.retryAfterSeconds = result.permanentFailure ? 0 : 300;
            if (ssl) SSL_free(ssl);
            closeSocket(sock);
            return result;
        }
        
        // Send DATA
        std::string dataCmd = "DATA\r\n";
        if (!sendData(sock, ssl, dataCmd)) {
            result.errorMessage = "Failed to send DATA";
            result.retryAfterSeconds = 60;
            if (ssl) SSL_free(ssl);
            closeSocket(sock);
            return result;
        }
        
        if (!readSmtpResponse(sock, ssl, response) || response.empty()) {
            result.errorMessage = "Failed to read DATA response";
            result.retryAfterSeconds = 60;
            if (ssl) SSL_free(ssl);
            closeSocket(sock);
            return result;
        }
        
        if (response[0].length() < 3 || response[0].substr(0, 3) != "354") {
            result.errorMessage = "DATA command failed: " + response[0];
            result.retryAfterSeconds = 60;
            if (ssl) SSL_free(ssl);
            closeSocket(sock);
            return result;
        }
        
        // Send message
        if (!sendData(sock, ssl, rawMessage)) {
            result.errorMessage = "Failed to send message data";
            result.retryAfterSeconds = 60;
            if (ssl) SSL_free(ssl);
            closeSocket(sock);
            return result;
        }
        
        // Send end-of-data marker
        std::string endOfData = "\r\n.\r\n";
        if (!sendData(sock, ssl, endOfData)) {
            result.errorMessage = "Failed to send end-of-data marker";
            result.retryAfterSeconds = 60;
            if (ssl) SSL_free(ssl);
            closeSocket(sock);
            return result;
        }
        
        // Read delivery response
        if (!readSmtpResponse(sock, ssl, response) || response.empty()) {
            result.errorMessage = "Failed to read delivery response";
            result.retryAfterSeconds = 60;
            if (ssl) SSL_free(ssl);
            closeSocket(sock);
            return result;
        }
        
        if (response[0].length() < 3 || response[0].substr(0, 3) != "250") {
            result.errorMessage = "Message rejected: " + response[0];
            result.permanentFailure = (response[0].length() >= 3 && response[0][0] == '5');
            result.retryAfterSeconds = result.permanentFailure ? 0 : 300;
            if (ssl) SSL_free(ssl);
            closeSocket(sock);
            return result;
        }
        
        // Send QUIT
        std::string quit = "QUIT\r\n";
        sendData(sock, ssl, quit);  // Don't check result for QUIT
        
        result.success = true;
        
        // Clean shutdown
        if (ssl) {
            SSL_shutdown(ssl);
            SSL_free(ssl);
        }
        closeSocket(sock);
        
        Logger::instance().log(LogLevel::Info,
            "Delivery: Successfully delivered to " + to + " via " + mxHost + 
            (tlsActive ? " (TLS)" : " (plaintext)"));
        
    } catch (const std::exception& ex) {
        result.errorMessage = "Exception during delivery: " + std::string(ex.what());
        result.retryAfterSeconds = 300;
        if (ssl) SSL_free(ssl);
        if (sock != INVALID_SOCKET) closeSocket(sock);
    }
    
    return result;
}

DeliveryResult SmtpDeliveryClient::deliver(
    const std::string& from,
    const std::string& to,
    const std::string& rawMessage
) {
    // Extract domain from recipient
    size_t atPos = to.find('@');
    if (atPos == std::string::npos) {
        DeliveryResult result;
        result.permanentFailure = true;
        result.errorMessage = "Invalid recipient address: " + to;
        return result;
    }
    
    std::string domain = to.substr(atPos + 1);
    
    // Lookup MX records
    auto mxHosts = lookupMX(domain);
    if (mxHosts.empty()) {
        DeliveryResult result;
        result.permanentFailure = true;
        result.errorMessage = "No MX records found for " + domain;
        return result;
    }
    
    // Try each MX host
    for (const auto& mxHost : mxHosts) {
        DeliveryResult result = connectAndDeliver(mxHost, DEFAULT_SMTP_PORT, from, to, rawMessage);
        if (result.success) {
            return result;
        }
        if (result.permanentFailure) {
            return result; // Don't try other MX hosts for permanent failures
        }
        // Temporary failure - try next MX host
    }
    
    // All MX hosts failed
    DeliveryResult result;
    result.errorMessage = "All MX hosts failed for " + domain;
    result.retryAfterSeconds = 300;
    return result;
}

