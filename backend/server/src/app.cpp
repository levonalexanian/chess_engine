#include "chess_server/app.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <utility>

#include <crow/json.h>
#include <spdlog/spdlog.h>

#include "chess_server/game_session.h"
#include "chess_server/registry.h"

namespace chess_server {

static constexpr std::string_view kFrontendMissingPage =
    "<!doctype html>\n"
    "<html lang=\"en\">\n"
    "<head><meta charset=\"utf-8\"><title>chess-server</title></head>\n"
    "<body>\n"
    "<h1>chess-server</h1>\n"
    "<p>Frontend bundle not built yet. Run <code>make web</code> after the\n"
    "frontend lands (initial commit 9).</p>\n"
    "<p>WebSocket JSON endpoint: <code>/ws</code></p>\n"
    "<p>Health check: <code>/healthz</code></p>\n"
    "</body></html>\n";

static std::string content_type_for(std::filesystem::path const& path) {
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

static bool path_is_safe(std::string_view path) {
    if (path.find("..") != std::string_view::npos) {
        return false;
    }
    if (!path.empty() && path.front() == '/') {
        return false;
    }
    return true;
}

static crow::response serve_static_file(std::filesystem::path const& root,
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

static std::string env_or(char const* name, std::string fallback) {
    char const* value = std::getenv(name);
    if (value == nullptr || *value == '\0') {
        return fallback;
    }
    return std::string(value);
}

App::App(AppOptions options) : App(std::move(options), EngineRegistry::with_defaults()) {}

App::App(AppOptions options, EngineRegistry registry)
    : options_(std::move(options)),
      registry_(std::move(registry)),
      app_(std::make_unique<crow::SimpleApp>()) {
    register_routes();
}

crow::SimpleApp& App::crow_app() { return *app_; }

AppOptions const& App::options() const { return options_; }

EngineRegistry const& App::registry() const { return registry_; }

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

std::vector<OutboundMessage> App::dispatch_message(GameSession& session,
                                                   std::string_view payload) {
    auto parsed = crow::json::load(payload.data(), payload.size());
    if (!parsed) {
        return {make_error_message("bad json")};
    }
    if (!parsed.has("type") || parsed["type"].t() != crow::json::type::String) {
        return {make_error_message("missing type")};
    }

    std::string const type = parsed["type"].s();
    if (type == "new_game") {
        std::string engine_name(default_engine_name());
        if (parsed.has("engine")) {
            if (parsed["engine"].t() != crow::json::type::String) {
                return {make_error_message("missing engine")};
            }
            std::string requested(parsed["engine"].s());
            if (!requested.empty()) {
                engine_name = std::move(requested);
            }
        }
        std::optional<std::string> starting_fen;
        if (parsed.has("starting_fen")) {
            if (parsed["starting_fen"].t() != crow::json::type::String) {
                return {make_error_message("starting_fen must be a string")};
            }
            std::string value(parsed["starting_fen"].s());
            if (!value.empty()) {
                starting_fen = std::move(value);
            }
        }
        std::vector<std::string> move_history;
        if (parsed.has("moves")) {
            if (parsed["moves"].t() != crow::json::type::List) {
                return {make_error_message("moves must be an array")};
            }
            for (auto const& entry : parsed["moves"]) {
                if (entry.t() != crow::json::type::String) {
                    return {make_error_message("moves entries must be strings")};
                }
                move_history.emplace_back(entry.s());
            }
        }
        return session.on_new_game(engine_name, std::move(starting_fen), move_history);
    }
    if (type == "user_move") {
        if (!parsed.has("uci") || parsed["uci"].t() != crow::json::type::String) {
            return {make_error_message("missing uci")};
        }
        return session.on_user_move(std::string(parsed["uci"].s()));
    }
    if (type == "request_engine_move") {
        return session.on_request_engine_move();
    }
    return {make_error_message("unknown message type: " + type)};
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
        .onopen([this](crow::websocket::connection& conn) {
            spdlog::info("ws open from {}", conn.get_remote_ip());
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            sessions_.emplace(std::piecewise_construct,
                              std::forward_as_tuple(&conn),
                              std::forward_as_tuple(registry_));
        })
        .onclose([this](crow::websocket::connection& conn, std::string const& reason) {
            spdlog::info("ws close from {} (reason={})", conn.get_remote_ip(), reason);
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            sessions_.erase(&conn);
        })
        .onmessage([this](crow::websocket::connection& conn, std::string const& data,
                          bool is_binary) {
            if (is_binary) {
                auto const error = serialize_outbound(make_error_message("binary not supported"));
                conn.send_text(error);
                return;
            }

            std::vector<OutboundMessage> outbox;
            {
                std::lock_guard<std::mutex> lock(sessions_mutex_);
                auto it = sessions_.find(&conn);
                if (it == sessions_.end()) {
                    auto [inserted, _] = sessions_.emplace(
                        std::piecewise_construct,
                        std::forward_as_tuple(&conn),
                        std::forward_as_tuple(registry_));
                    it = inserted;
                }
                outbox = dispatch_message(it->second, data);
            }
            for (auto const& msg : outbox) {
                conn.send_text(serialize_outbound(msg));
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
        if (!path_is_safe(path)) {
            return crow::response(crow::status::BAD_REQUEST, "invalid path");
        }
        std::filesystem::path target = static_root / std::filesystem::path(path);
        if (std::filesystem::exists(target, ec) && !ec) {
            return serve_static_file(static_root, path);
        }
        auto const extension = std::filesystem::path(path).extension().string();
        bool looks_like_asset = !extension.empty() && extension != ".html";
        if (looks_like_asset) {
            return crow::response(crow::status::NOT_FOUND, "not found");
        }
        if (std::filesystem::exists(static_root / "index.html", ec) && !ec) {
            return serve_static_file(static_root, "index.html");
        }
        return crow::response(crow::status::NOT_FOUND, "not found");
    });
}

}  // namespace chess_server
