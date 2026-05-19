#pragma once

#include <array>
#include <cstdint>

#include "chess_engine/board.hpp"

namespace chess_engine::zobrist {

// Deterministic zobrist constants generated from a fixed-seed std::mt19937_64.
// Layout: 12 pieces * 64 squares + 16 castling-mask states + 1 black-to-move
// + 8 en-passant files = 793 keys (we store 781 individual keys plus a 16-entry
// castling table). The seed is committed to the codebase to make a future change
// to the seed an intentional, reviewable act.
constexpr std::uint64_t kSeed = 0xC0FFEEC0FFEEDEADull;

struct Table {
    std::array<std::array<std::uint64_t, kNumSquares>, kNumPieces> piece_square;
    std::array<std::uint64_t, 16> castling;
    std::array<std::uint64_t, 8> en_passant_file;
    std::uint64_t black_to_move;
};

const Table& table();

}  // namespace chess_engine::zobrist
