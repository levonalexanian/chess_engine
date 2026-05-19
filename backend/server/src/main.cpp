#include <cstdlib>
#include <exception>
#include <filesystem>

#include <spdlog/spdlog.h>

#include "chess_server/app.hpp"

int main() {
    spdlog::set_level(spdlog::level::info);

    try {
        auto options = chess_server::App::options_from_env();
        chess_server::App app(options);

        std::error_code ec;
        if (std::filesystem::exists(options.static_root, ec) && !ec) {
            spdlog::info("Serving static assets from {}", options.static_root.string());
        } else {
            spdlog::warn("Static asset directory {} not found; serving stub page",
                         options.static_root.string());
        }

        spdlog::info("chess-server listening on {}:{}", options.bind_address, options.port);
        app.crow_app().bindaddr(options.bind_address).port(options.port).multithreaded().run();
    } catch (std::exception const& ex) {
        spdlog::error("chess-server fatal error: {}", ex.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
