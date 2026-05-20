#include "chess_engine/zobrist.h"

#include <random>

namespace chess_engine::zobrist {

namespace {

Table build_table() {
    Table t{};
    std::mt19937_64 prng(kSeed);
    for (int piece = 0; piece < kNumPieces; ++piece) {
        for (int sq = 0; sq < kNumSquares; ++sq) {
            t.piece_square[piece][sq] = prng();
        }
    }
    for (int i = 0; i < 16; ++i) {
        t.castling[i] = prng();
    }
    for (int i = 0; i < 8; ++i) {
        t.en_passant_file[i] = prng();
    }
    t.black_to_move = prng();
    return t;
}

}  // namespace

const Table& table() {
    static const Table kTable = build_table();
    return kTable;
}

}  // namespace chess_engine::zobrist
