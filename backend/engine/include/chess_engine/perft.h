#ifndef CHESS_ENGINE_PERFT_H
#define CHESS_ENGINE_PERFT_H

#include <cstdint>

#include "chess_engine/position.h"

namespace chess_engine {

// Count the number of leaf nodes at depth `depth` from `pos`, using legal
// move generation and make/unmake to traverse. Standard chess-programming
// perft used to validate move generation correctness against known-good counts.
std::uint64_t perft(Position& pos, int depth);

}  // namespace chess_engine

#endif  // CHESS_ENGINE_PERFT_H
