#include "core/config_loader.h"
#include "core/logger.h"
#include "core/tls_enforcement.h"
#include <vector>
#include <yaml-cpp/yaml.h>
#include <stdexcept>
#include <algorithm>

ServerConfig ConfigLoader::loadFromFile(const std::string& path) {
    ServerConfig cfg;

    try {
        YAML::Node root = YAML::LoadFile(path);

        /* =========================
           SERVER SECTION (MISSING BEFORE)
           ========================= */
        if (root["server"]) {
            auto s = root["server"];

            if (s["host"]) cfg.host = s["host"].as<std::string>();
            if (s["domain"]) cfg.domain = s["domain"].as<std::string>();
            if (s["smtp_port"]) cfg.smtpPort = s["smtp_port"].as<int>();
            if (s["imap_port"]) cfg.imapPort = s["imap_port"].as<int>();
            if (s["mail_root"]) cfg.mailRoot = s["mail_root"].as<std::string>();

            if (s["tls_required"]) cfg.tlsRequired = s["tls_required"].as<bool>();
            if (s["require_starttls"]) cfg.requireStartTls = s["require_starttls"].as<bool>();

            if (s["min_tls_version"]) {
                cfg.minTlsVersion = s["min_tls_version"].as<int>();
                cfg.hasMinTlsVersion = true;
            }
        }

        /* =========================
           TLS FILES
           ========================= */
        if (root["tls"]) {
            auto t = root["tls"];
            if (t["cert_file"]) cfg.tlsCertFile = t["cert_file"].as<std::string>();
            if (t["key_file"])  cfg.tlsKeyFile  = t["key_file"].as<std::string>();
        }

        /* =========================
           LOGGING
           ========================= */
        if (root["logging"]) {
            auto l = root["logging"];
            if (l["file"])  cfg.logFile  = l["file"].as<std::string>();
            if (l["level"]) cfg.logLevel = l["level"].as<std::string>();
        }

        /* =========================
           SMTP
           ========================= */
        if (root["smtp"]) {
            auto s = root["smtp"];
            if (s["max_message_size"]) cfg.maxMessageSize = s["max_message_size"].as<size_t>();
            if (s["timeout"]) cfg.smtpTimeout = s["timeout"].as<int>();
            if (s["data_timeout"]) cfg.dataTimeout = s["data_timeout"].as<int>();
        }

        /* =========================
           AUTH
           ========================= */
        if (root["auth"]) {
            auto a = root["auth"];
            if (a["users_file"]) cfg.usersFile = a["users_file"].as<std::string>();
        }

        /* =========================
           HA / CLUSTER
           ========================= */
        if (root["ha"]) {
            auto ha = root["ha"];
            if (ha["enabled"]) cfg.enableHA = ha["enabled"].as<bool>();
            if (ha["redis_host"]) cfg.redisHost = ha["redis_host"].as<std::string>();
            if (ha["redis_port"]) cfg.redisPort = ha["redis_port"].as<int>();
            if (ha["redis_password"]) cfg.redisPassword = ha["redis_password"].as<std::string>();
            if (ha["cluster_id"]) cfg.clusterId = ha["cluster_id"].as<std::string>();
            if (ha["node_id"]) cfg.nodeId = ha["node_id"].as<std::string>();
        }

        /* =========================
           ADMIN
           ========================= */
        if (root["admin"]) {
            auto a = root["admin"];
            if (a["token"]) cfg.adminToken = a["token"].as<std::string>();
        }

    } catch (const std::exception& ex) {
        Logger::instance().log(
            LogLevel::Error,
            std::string("Failed to load config: ") + ex.what());
        throw;
    }

    /* =========================
       VALIDATION + TLS ENFORCEMENT
       ========================= */
    validateConfig(cfg);

    TlsEnforcement::instance().setMinTlsVersion(cfg.minTlsVersion);
    TlsEnforcement::instance().setTlsRequired(cfg.tlsRequired);
    TlsEnforcement::instance().setRequireStartTls(cfg.requireStartTls);

    return cfg;
}

void ConfigLoader::validateConfig(const ServerConfig& cfg) {
    std::vector<std::string> errors;

    if (!cfg.hasMinTlsVersion) {
        errors.push_back("server.min_tls_version is required");
    }

    if (cfg.minTlsVersion < 1 || cfg.minTlsVersion > 3) {
        errors.push_back("server.min_tls_version must be 1, 2, or 3");
    }

    if (!errors.empty()) {
        std::string msg = "Configuration validation failed:\n";
        for (const auto& err : errors) {
            msg += "  - " + err + "\n";
        }
        Logger::instance().log(LogLevel::Error, msg);
        throw std::runtime_error(msg);
    }

    Logger::instance().log(LogLevel::Info, "Configuration validation passed");
}
