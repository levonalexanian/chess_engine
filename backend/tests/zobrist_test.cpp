#include <cstdint>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>

#include "chess_engine/position.hpp"
#include "chess_engine/zobrist.hpp"

namespace ce = chess_engine;

namespace {

constexpr const char* kStartingFen =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
constexpr const char* kKiwipete =
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";

}  // namespace

TEST(Zobrist, TableIsDeterministic) {
    const auto& a = ce::zobrist::table();
    const auto& b = ce::zobrist::table();
    EXPECT_EQ(&a, &b);
    EXPECT_EQ(a.black_to_move, b.black_to_move);
    EXPECT_NE(a.black_to_move, 0u);
    EXPECT_EQ(a.piece_square[0][0], b.piece_square[0][0]);
}

TEST(Zobrist, ConstantsAreLargelyUnique) {
    const auto& t = ce::zobrist::table();
    std::unordered_set<std::uint64_t> seen;
    for (const auto& row : t.piece_square) {
        for (auto v : row) {
            seen.insert(v);
        }
    }
    for (auto v : t.castling) seen.insert(v);
    for (auto v : t.en_passant_file) seen.insert(v);
    seen.insert(t.black_to_move);
    // Collisions are astronomically unlikely from a 64-bit PRNG; we expect all unique.
    const std::size_t expected = 12 * 64 + 16 + 8 + 1;
    EXPECT_EQ(seen.size(), expected);
}

TEST(Zobrist, PositionFromFenHasComputedHash) {
    auto pos = ce::Position::from_fen(kStartingFen);
    ASSERT_TRUE(pos.has_value());
    EXPECT_EQ(pos->zobrist_hash(), pos->compute_zobrist_hash());
    EXPECT_NE(pos->zobrist_hash(), 0u);
}

TEST(Zobrist, DifferentPositionsHaveDifferentHashes) {
    auto start = ce::Position::from_fen(kStartingFen);
    auto kiwi = ce::Position::from_fen(kKiwipete);
    ASSERT_TRUE(start.has_value());
    ASSERT_TRUE(kiwi.has_value());
    EXPECT_NE(start->zobrist_hash(), kiwi->zobrist_hash());
}

TEST(Zobrist, HashIsInvariantUnderFenRoundTrip) {
    for (const char* fen : {
             kStartingFen,
             kKiwipete,
             "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
             "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
             "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
             "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
         }) {
        auto p1 = ce::Position::from_fen(fen);
        ASSERT_TRUE(p1.has_value()) << fen;
        auto p2 = ce::Position::from_fen(p1->to_fen());
        ASSERT_TRUE(p2.has_value()) << fen;
        EXPECT_EQ(p1->zobrist_hash(), p2->zobrist_hash()) << fen;
    }
}

TEST(Zobrist, SideToMoveAffectsHash) {
    auto white = ce::Position::from_fen(kStartingFen);
    auto black = ce::Position::from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
    ASSERT_TRUE(white.has_value());
    ASSERT_TRUE(black.has_value());
    EXPECT_NE(white->zobrist_hash(), black->zobrist_hash());
}

TEST(Zobrist, CastlingMaskAffectsHash) {
    auto full = ce::Position::from_fen(kStartingFen);
    auto less = ce::Position::from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w Kkq - 0 1");
    ASSERT_TRUE(full.has_value());
    ASSERT_TRUE(less.has_value());
    EXPECT_NE(full->zobrist_hash(), less->zobrist_hash());
}

TEST(Zobrist, EnPassantSquareAffectsHash) {
    auto a = ce::Position::from_fen("rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2");
    auto b = ce::Position::from_fen("rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2");
    ASSERT_TRUE(a.has_value());
    ASSERT_TRUE(b.has_value());
    EXPECT_NE(a->zobrist_hash(), b->zobrist_hash());
}
