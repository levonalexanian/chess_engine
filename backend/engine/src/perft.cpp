#include "chess_engine/perft.hpp"

#include <cstdint>

#include "chess_engine/move.hpp"
#include "chess_engine/position.hpp"

namespace chess_engine {

std::uint64_t perft(Position& pos, int depth) {
    if (depth == 0) return 1;
    auto moves = pos.generate_legal_moves();
    if (depth == 1) {
        return static_cast<std::uint64_t>(moves.size());
    }
    std::uint64_t nodes = 0;
    for (const auto& m : moves) {
        auto undo = pos.make_move(m);
        nodes += perft(pos, depth - 1);
        pos.unmake_move(undo);
    }
    return nodes;
}

}  // namespace chess_engine
