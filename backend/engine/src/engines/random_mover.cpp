#include "chess_engine/engines/random_mover.hpp"

#include <chrono>
#include <cstdint>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include <spdlog/spdlog.h>

#include "chess_engine/move.hpp"
#include "chess_engine/position.hpp"

namespace chess_engine {

RandomMoverEngine::RandomMoverEngine() : RandomMoverEngine(std::random_device{}()) {}

RandomMoverEngine::RandomMoverEngine(std::uint64_t seed) : position_(), rng_(seed) {}

void RandomMoverEngine::set_position(std::string_view fen) {
    auto parsed = Position::from_fen(fen);
    if (!parsed.has_value()) {
        spdlog::warn("RandomMoverEngine::set_position: malformed FEN, keeping previous position: {}",
                     std::string(fen));
        return;
    }
    position_ = *parsed;
}

Move RandomMoverEngine::best_move(std::chrono::milliseconds) {
    auto const moves = position_.generate_legal_moves();
    if (moves.empty()) {
        return Move{};
    }
    std::uniform_int_distribution<std::size_t> dist(0, moves.size() - 1);
    return moves[dist(rng_)];
}

std::string RandomMoverEngine::name() const {
    return "random";
}

}  // namespace chess_engine
