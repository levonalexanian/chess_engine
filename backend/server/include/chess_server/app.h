#ifndef CHESS_SERVER_APP_H
#define CHESS_SERVER_APP_H

#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <crow.h>

#include "chess_server/game_session.h"
#include "chess_server/registry.h"

namespace chess_server {

struct AppOptions {
    std::string bind_address = "0.0.0.0";
    std::uint16_t port = 8080;
    std::filesystem::path static_root = "frontend/dist";
};

class App {
public:
    explicit App(AppOptions options);
    App(AppOptions options, EngineRegistry registry);

    crow::SimpleApp& crow_app();

    AppOptions const& options() const;
    EngineRegistry const& registry() const;

    static AppOptions options_from_env();

    static std::vector<OutboundMessage> dispatch_message(GameSession& session,
                                                         std::string_view payload);

private:
    void register_routes();

    AppOptions options_;
    EngineRegistry registry_;
    std::unique_ptr<crow::SimpleApp> app_;

    std::mutex sessions_mutex_;
    std::unordered_map<crow::websocket::connection*, GameSession> sessions_;
};

}  // namespace chess_server

#endif  // CHESS_SERVER_APP_H
