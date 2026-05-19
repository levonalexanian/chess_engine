#pragma once

#include <array>
#include <cstdint>

#include "chess_engine/board.hpp"

namespace chess_engine::attacks {

constexpr int file_of(int square) { return square & 7; }
constexpr int rank_of(int square) { return square >> 3; }
constexpr int square_of(int file, int rank) { return rank * 8 + file; }
constexpr Bitboard bit(int square) { return Bitboard{1} << square; }

namespace detail {

constexpr bool on_board(int file, int rank) {
    return file >= 0 && file < 8 && rank >= 0 && rank < 8;
}

constexpr std::array<Bitboard, kNumSquares> build_knight_attacks() {
    std::array<Bitboard, kNumSquares> table{};
    constexpr int dfs[8] = {-2, -2, -1, -1, 1, 1, 2, 2};
    constexpr int drs[8] = {-1, 1, -2, 2, -2, 2, -1, 1};
    for (int sq = 0; sq < kNumSquares; ++sq) {
        const int f = sq & 7;
        const int r = sq >> 3;
        Bitboard attacks = 0;
        for (int i = 0; i < 8; ++i) {
            const int nf = f + dfs[i];
            const int nr = r + drs[i];
            if (on_board(nf, nr)) {
                attacks |= Bitboard{1} << (nr * 8 + nf);
            }
        }
        table[sq] = attacks;
    }
    return table;
}

constexpr std::array<Bitboard, kNumSquares> build_king_attacks() {
    std::array<Bitboard, kNumSquares> table{};
    for (int sq = 0; sq < kNumSquares; ++sq) {
        const int f = sq & 7;
        const int r = sq >> 3;
        Bitboard attacks = 0;
        for (int df = -1; df <= 1; ++df) {
            for (int dr = -1; dr <= 1; ++dr) {
                if (df == 0 && dr == 0) continue;
                const int nf = f + df;
                const int nr = r + dr;
                if (on_board(nf, nr)) {
                    attacks |= Bitboard{1} << (nr * 8 + nf);
                }
            }
        }
        table[sq] = attacks;
    }
    return table;
}

constexpr std::array<std::array<Bitboard, kNumSquares>, 2> build_pawn_attacks() {
    std::array<std::array<Bitboard, kNumSquares>, 2> table{};
    for (int sq = 0; sq < kNumSquares; ++sq) {
        const int f = sq & 7;
        const int r = sq >> 3;
        Bitboard white = 0;
        if (on_board(f - 1, r + 1)) white |= Bitboard{1} << ((r + 1) * 8 + (f - 1));
        if (on_board(f + 1, r + 1)) white |= Bitboard{1} << ((r + 1) * 8 + (f + 1));
        Bitboard black = 0;
        if (on_board(f - 1, r - 1)) black |= Bitboard{1} << ((r - 1) * 8 + (f - 1));
        if (on_board(f + 1, r - 1)) black |= Bitboard{1} << ((r - 1) * 8 + (f + 1));
        table[Color::White][sq] = white;
        table[Color::Black][sq] = black;
    }
    return table;
}

}  // namespace detail

inline constexpr std::array<Bitboard, kNumSquares> kKnightAttacks = detail::build_knight_attacks();
inline constexpr std::array<Bitboard, kNumSquares> kKingAttacks = detail::build_king_attacks();
inline constexpr std::array<std::array<Bitboard, kNumSquares>, 2> kPawnAttacks =
    detail::build_pawn_attacks();

constexpr Bitboard knight_attacks(int square) { return kKnightAttacks[square]; }
constexpr Bitboard king_attacks(int square) { return kKingAttacks[square]; }
constexpr Bitboard pawn_attacks(Color c, int square) { return kPawnAttacks[c][square]; }

}  // namespace chess_engine::attacks
