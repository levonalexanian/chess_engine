#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include <crow.h>

namespace chess_server {

struct AppOptions {
    std::string bind_address = "0.0.0.0";
    std::uint16_t port = 8080;
    std::filesystem::path static_root = "frontend/dist";
};

class App {
public:
    explicit App(AppOptions options);

    crow::SimpleApp& crow_app();

    AppOptions const& options() const;

    static AppOptions options_from_env();

private:
    void register_routes();

    AppOptions options_;
    std::unique_ptr<crow::SimpleApp> app_;
};

}  // namespace chess_server
