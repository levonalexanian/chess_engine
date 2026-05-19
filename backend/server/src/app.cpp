#include "chess_server/app.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>

#include <spdlog/spdlog.h>

namespace chess_server {

namespace {

constexpr std::string_view kFrontendMissingPage =
    "<!doctype html>\n"
    "<html lang=\"en\">\n"
    "<head><meta charset=\"utf-8\"><title>chess-server</title></head>\n"
    "<body>\n"
    "<h1>chess-server</h1>\n"
    "<p>Frontend bundle not built yet. Run <code>make web</code> after the\n"
    "frontend lands (initial commit 9).</p>\n"
    "<p>WebSocket echo endpoint: <code>/ws</code></p>\n"
    "<p>Health check: <code>/healthz</code></p>\n"
    "</body></html>\n";

std::string content_type_for(std::filesystem::path const& path) {
    static const std::unordered_map<std::string, std::string> kTypes = {
        {".html", "text/html; charset=utf-8"},
        {".htm", "text/html; charset=utf-8"},
        {".js", "application/javascript; charset=utf-8"},
        {".mjs", "application/javascript; charset=utf-8"},
        {".css", "text/css; charset=utf-8"},
        {".json", "application/json; charset=utf-8"},
        {".map", "application/json; charset=utf-8"},
        {".svg", "image/svg+xml"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".ico", "image/x-icon"},
        {".woff", "font/woff"},
        {".woff2", "font/woff2"},
        {".ttf", "font/ttf"},
        {".txt", "text/plain; charset=utf-8"},
    };
    auto it = kTypes.find(path.extension().string());
    if (it == kTypes.end()) {
        return "application/octet-stream";
    }
    return it->second;
}

bool path_is_safe(std::string_view path) {
    if (path.find("..") != std::string_view::npos) {
        return false;
    }
    if (!path.empty() && path.front() == '/') {
        return false;
    }
    return true;
}

crow::response serve_static_file(std::filesystem::path const& root,
                                 std::string_view relative_path) {
    if (!path_is_safe(relative_path)) {
        return crow::response(crow::status::BAD_REQUEST, "invalid path");
    }

    std::filesystem::path target = root / std::filesystem::path(std::string(relative_path));

    std::error_code ec;
    if (!std::filesystem::exists(target, ec) || ec) {
        return crow::response(crow::status::NOT_FOUND, "not found");
    }
    if (std::filesystem::is_directory(target, ec)) {
        target /= "index.html";
        if (!std::filesystem::exists(target, ec) || ec) {
            return crow::response(crow::status::NOT_FOUND, "not found");
        }
    }

    std::ifstream input(target, std::ios::binary);
    if (!input) {
        return crow::response(crow::status::NOT_FOUND, "not found");
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();

    crow::response response{buffer.str()};
    response.set_header("Content-Type", content_type_for(target));
    return response;
}

std::string env_or(char const* name, std::string fallback) {
    char const* value = std::getenv(name);
    if (value == nullptr || *value == '\0') {
        return fallback;
    }
    return std::string(value);
}

}  // namespace

App::App(AppOptions options)
    : options_(std::move(options)), app_(std::make_unique<crow::SimpleApp>()) {
    register_routes();
}

crow::SimpleApp& App::crow_app() { return *app_; }

AppOptions const& App::options() const { return options_; }

AppOptions App::options_from_env() {
    AppOptions options;
    options.bind_address = env_or("CHESS_SERVER_BIND", options.bind_address);
    std::string port_value = env_or("CHESS_SERVER_PORT", std::to_string(options.port));
    try {
        int parsed = std::stoi(port_value);
        if (parsed >= 0 && parsed <= 65535) {
            options.port = static_cast<std::uint16_t>(parsed);
        } else {
            spdlog::warn("CHESS_SERVER_PORT={} out of range, using default {}", port_value,
                         options.port);
        }
    } catch (std::exception const& ex) {
        spdlog::warn("CHESS_SERVER_PORT={} unparsable ({}), using default {}", port_value,
                     ex.what(), options.port);
    }

    char const* static_root = std::getenv("CHESS_SERVER_STATIC_ROOT");
    if (static_root != nullptr && *static_root != '\0') {
        options.static_root = static_root;
    }

    return options;
}

void App::register_routes() {
    auto& app = *app_;

    CROW_ROUTE(app, "/healthz")
    ([] {
        crow::response response{R"({"status":"ok"})"};
        response.set_header("Content-Type", "application/json");
        return response;
    });

    CROW_WEBSOCKET_ROUTE(app, "/ws")
        .onopen([](crow::websocket::connection& conn) {
            spdlog::info("ws open from {}", conn.get_remote_ip());
        })
        .onclose([](crow::websocket::connection& conn, std::string const& reason) {
            spdlog::info("ws close from {} (reason={})", conn.get_remote_ip(), reason);
        })
        .onmessage([](crow::websocket::connection& conn, std::string const& data, bool is_binary) {
            if (is_binary) {
                conn.send_binary(data);
            } else {
                conn.send_text(data);
            }
        });

    auto static_root = options_.static_root;

    CROW_ROUTE(app, "/")
    ([static_root] {
        std::error_code ec;
        if (std::filesystem::exists(static_root / "index.html", ec) && !ec) {
            return serve_static_file(static_root, "index.html");
        }
        crow::response response{std::string(kFrontendMissingPage)};
        response.set_header("Content-Type", "text/html; charset=utf-8");
        return response;
    });

    CROW_ROUTE(app, "/<path>")
    ([static_root](std::string const& path) {
        std::error_code ec;
        if (!std::filesystem::exists(static_root, ec) || ec) {
            return crow::response(crow::status::NOT_FOUND, "not found");
        }
        return serve_static_file(static_root, path);
    });
}

}  // namespace chess_server
