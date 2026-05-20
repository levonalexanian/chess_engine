#include <cstdlib>
#include <exception>
#include <filesystem>

#include <spdlog/spdlog.h>

#include "chess_server/app.h"

int main() {
    spdlog::set_level(spdlog::level::info);

    try {
        auto options = chess_server::App::options_from_env();

        std::error_code ec;
        auto absolute_root = std::filesystem::weakly_canonical(options.static_root, ec);
        if (ec) {
            absolute_root = std::filesystem::absolute(options.static_root);
        }
        options.static_root = absolute_root;

        chess_server::App app(options);

        if (std::filesystem::exists(absolute_root, ec) && !ec) {
            spdlog::info("Serving static assets from {}", absolute_root.string());
        } else {
            spdlog::warn("Static asset directory {} not found; serving stub page",
                         absolute_root.string());
        }

        spdlog::info("chess-server listening on {}:{}", options.bind_address, options.port);
        app.crow_app().bindaddr(options.bind_address).port(options.port).multithreaded().run();
    } catch (std::exception const& ex) {
        spdlog::error("chess-server fatal error: {}", ex.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
