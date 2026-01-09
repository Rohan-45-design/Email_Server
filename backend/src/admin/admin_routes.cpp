#include "admin/admin_routes.h"
#include "admin/admin_auth.h"
#include "monitoring/metrics.h"
#include "monitoring/health.h"
#include "queue/mail_queue.h"
#include "core/logger.h"

#include <nlohmann/json.hpp>

#include "core/platform_socket.h"

#include <sstream>
#include <chrono>
#include <ctime>
#include <vector>
#include <string>
#include <algorithm>

/* ============================================================
   Helpers
   ============================================================ */

static std::string generateAuthToken() {
    auto now = std::chrono::system_clock::now();
    auto ts = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    return "admin_token_" + std::to_string(ts);
}

static std::string httpOK(const std::string& body,
                          const std::string& contentType = "text/plain") {
    return
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: " + contentType + "\r\n"
        "Access-Control-Allow-Origin: http://localhost:5175\r\n"
        "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
        "\r\n" + body;
}

static std::string httpCreated(const std::string& body) {
    return
        "HTTP/1.1 201 Created\r\n"
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: http://localhost:5175\r\n"
        "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
        "\r\n" + body;
}

static std::string httpUnauthorized() {
    return
        "HTTP/1.1 401 Unauthorized\r\n"
        "Access-Control-Allow-Origin: http://localhost:5175\r\n"
        "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
        "\r\n";
}

static std::string httpNotFound() {
    return
        "HTTP/1.1 404 Not Found\r\n"
        "Access-Control-Allow-Origin: http://localhost:5175\r\n"
        "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
        "\r\n";
}

static std::string httpMethodNotAllowed() {
    return
        "HTTP/1.1 405 Method Not Allowed\r\n"
        "Access-Control-Allow-Origin: http://localhost:5175\r\n"
        "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
        "\r\n";
}

static std::string httpOptions() {
    return
        "HTTP/1.1 200 OK\r\n"
        "Access-Control-Allow-Origin: http://localhost:5175\r\n"
        "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
        "\r\n";
}

/* ============================================================
   In-memory demo storage
   ============================================================ */

static std::vector<nlohmann::json> users = {
    {
        {"id", 1},
        {"username", "admin"},
        {"email", "admin@example.com"},
        {"createdAt", "2024-01-01T00:00:00Z"}
    }
};

static int nextUserId = 2;

/* ============================================================
   Request parsing
   ============================================================ */

static nlohmann::json parseJsonBody(const std::string& request) {
    const auto pos = request.find("\r\n\r\n");
    if (pos == std::string::npos)
        return nullptr;

    try {
        return nlohmann::json::parse(request.substr(pos + 4));
    } catch (...) {
        return nullptr;
    }
}

static std::pair<std::string, std::string>
parseRequest(const std::string& req) {
    const auto s1 = req.find(' ');
    if (s1 == std::string::npos)
        return {"", ""};

    const auto s2 = req.find(' ', s1 + 1);
    if (s2 == std::string::npos)
        return {"", ""};

    return {
        req.substr(0, s1),
        req.substr(s1 + 1, s2 - s1 - 1)
    };
}

/* ============================================================
   Main handler
   ============================================================ */

std::string AdminRoutes::handleRequest(socket_t sock) {
    char buf[8192]{};

    int n = recv(sock, buf, sizeof(buf) - 1, 0);
    if (n <= 0)
        return httpNotFound();

    std::string request(buf, n);
    auto [method, path] = parseRequest(request);

    if (method == "OPTIONS")
        return httpOptions();

    /* ---------- LOGIN ---------- */
    if (path == "/api/auth/login" && method == "POST") {
        auto body = parseJsonBody(request);
        if (!body || !body.contains("username") || !body.contains("password"))
            return httpUnauthorized();

        if (body["username"] == "admin" &&
            body["password"] == "admin123") {

            nlohmann::json resp = {
                {"token", generateAuthToken()},
                {"user", {
                    {"id", 1},
                    {"username", "admin"},
                    {"email", "admin@example.com"}
                }}
            };
            return httpOK(resp.dump(), "application/json");
        }
        return httpUnauthorized();
    }

    /* ---------- AUTH ---------- */
    if (!AdminAuth::authorize(request))
        return httpUnauthorized();

    /* ---------- ME ---------- */
    if (path == "/api/auth/me" && method == "GET") {
        nlohmann::json resp = {
            {"id", 1},
            {"username", "admin"},
            {"email", "admin@example.com"}
        };
        return httpOK(resp.dump(), "application/json");
    }

    /* ---------- USERS ---------- */
    if (path == "/api/users" && method == "GET") {
        return httpOK(nlohmann::json(users).dump(), "application/json");
    }

    if (path == "/api/users" && method == "POST") {
        auto body = parseJsonBody(request);
        if (!body || !body.contains("username")
                  || !body.contains("email")
                  || !body.contains("password"))
            return httpMethodNotAllowed();

        nlohmann::json u = {
            {"id", nextUserId++},
            {"username", body["username"]},
            {"email", body["email"]},
            {"createdAt", "2024-01-01T00:00:00Z"}
        };
        users.push_back(u);
        return httpCreated(u.dump());
    }

    if (path.rfind("/api/users/", 0) == 0 && method == "DELETE") {
        int id = std::stoi(path.substr(11));
        auto it = std::find_if(users.begin(), users.end(),
            [&](const nlohmann::json& u) {
                return u["id"] == id;
            });

        if (it != users.end()) {
            users.erase(it);
            return httpOK("{\"message\":\"User deleted\"}", "application/json");
        }
        return httpNotFound();
    }

    /* ---------- STATUS ---------- */
    if (path == "/api/server/status" && method == "GET") {
        auto health = Health::check();

        static auto startTime = std::chrono::steady_clock::now();
        auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - startTime).count();

        nlohmann::json response = {
            {"running", health.ok},
            {"uptime", uptime},
            {"metrics", Metrics::instance().renderPrometheus()}
        };

        return httpOK(response.dump(), "application/json");
    }

    /* ---------- METRICS ---------- */
    if (path == "/api/server/metrics" && method == "GET") {
        return httpOK(Metrics::instance().renderPrometheus(), "text/plain");
    }

    /* ---------- LOGS ---------- */
    if (path == "/api/logs" && method == "GET") {
        nlohmann::json logs = nlohmann::json::array({
            {{"level","info"},{"message","Server started"}},
            {{"level","info"},{"message","SMTP listening"}}
        });
        return httpOK(nlohmann::json{{"logs", logs}}.dump(),
                      "application/json");
    }

    /* ---------- LEGACY ---------- */
    if (path == "/admin/health" && method == "GET") {
        return httpOK(Health::check().ok ? "OK" : "FAIL");
    }

    if (path == "/admin/metrics" && method == "GET") {
        return httpOK(Metrics::instance().renderPrometheus());
    }

    if (path == "/admin/queue" && method == "GET") {
        std::ostringstream oss;
        for (auto& m : MailQueue::instance().list())
            oss << m.id << "\n";
        return httpOK(oss.str());
    }

    return httpNotFound();
}
