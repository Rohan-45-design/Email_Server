cmake_minimum_required(VERSION 3.16)
project(mailserver CXX)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# -------------------- C++ STANDARD --------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------- FETCHCONTENT --------------------
include(FetchContent)

# =====================================================
# yaml-cpp (STABLE RELEASE TARBALL)
# =====================================================
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
set(YAML_CPP_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  yaml-cpp
  URL https://github.com/jbeder/yaml-cpp/archive/refs/tags/0.8.0.tar.gz
)

FetchContent_MakeAvailable(yaml-cpp)

# =====================================================
# nlohmann/json (STABLE RELEASE TARBALL)
# =====================================================
FetchContent_Declare(
  nlohmann_json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)

FetchContent_MakeAvailable(nlohmann_json)

# =====================================================
# MAIL SERVER EXECUTABLE
# =====================================================
add_executable(mailserver
    src/main.cpp

    src/core/server_context.cpp
    src/core/config_loader.cpp
    src/core/logger.cpp
    src/core/tls_context.cpp
    src/core/network_init.cpp
    src/core/password_hash.cpp
    src/core/audit_log.cpp
    src/core/input_validator.cpp
    src/core/app_lifecycle.cpp
    src/core/shutdown_coordinator.cpp
    src/core/crash_containment.cpp
    src/core/tls_enforcement.cpp
    src/core/connection_manager.cpp
    src/core/readiness_state.cpp
    src/core/auth_manager.cpp
    src/core/distributed_auth_manager.cpp
    src/core/rate_limiter.cpp
    src/core/base64.cpp

    src/smtp/smtp_server.cpp
    src/smtp/smtp_session.cpp

    src/imap/imap_server.cpp
    src/imap/imap_session.cpp
    src/imap/flags_index.cpp

    src/storage/mail_store.cpp

    src/queue/mail_queue.cpp
    src/queue/retry_worker.cpp

    src/delivery/smtp_client.cpp

    src/dns/dns_resolver.cpp
    src/dns/dns_packet.cpp

    src/antispam/dkim_canon.cpp
    src/antispam/dkim_signer.cpp
    src/antispam/dkim_verifier.cpp
    src/antispam/dmarc_evaluator.cpp
    src/antispam/spf_evaluator.cpp
    src/antispam/spf_checker.cpp
    src/antispam/spf_parser.cpp

    src/spam/spam_engine.cpp
    src/spam/spam_rules.cpp

    src/monitoring/metrics.cpp
    src/monitoring/health.cpp
    src/monitoring/http_metrics_server.cpp

    src/admin/admin_server.cpp
    src/admin/admin_routes.cpp
    src/admin/admin_auth.cpp

    src/ha/leader_election.cpp
    src/ha/ha_controller.cpp

    src/antivirus/virus_scanner.cpp

    src/virus/cloud_scanner.cpp
    src/virus/cloud_provider_virustotal.cpp
    src/virus/cloud_scan_worker.cpp
    src/virus/virus_verdict_store.cpp

    src/virus/sandbox_engine.cpp
    src/virus/sandbox_provider_anyrun.cpp
    src/virus/sandbox_verdict_store.cpp

    src/mime/mime_parser.cpp
    src/mime/mime_decoder.cpp

    src/policy/attachment_policy.cpp

    src/retro/retro_manager.cpp

    src/threat_intel/hash_reputation.cpp
    src/threat_intel/intel_feedback.cpp
    src/threat_intel/ioc_store.cpp
    src/threat_intel/sender_reputation.cpp
)

target_include_directories(mailserver PRIVATE src)

# =====================================================
# OpenSSL (TLS, STARTTLS, DKIM)
# =====================================================
find_package(OpenSSL REQUIRED)
target_link_libraries(mailserver PRIVATE OpenSSL::SSL OpenSSL::Crypto)

# =====================================================
# Third-party libraries
# =====================================================
target_link_libraries(mailserver PRIVATE
    yaml-cpp::yaml-cpp
    nlohmann_json::nlohmann_json
)

# =====================================================
# Windows networking
# =====================================================
if (WIN32)
    target_link_libraries(mailserver PRIVATE ws2_32)
endif()
