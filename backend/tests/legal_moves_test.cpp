#include <algorithm>
#include <string>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>

#include "chess_engine/move.hpp"
#include "chess_engine/position.hpp"

namespace ce = chess_engine;

namespace {

ce::Position pos_from(const char* fen) {
    auto p = ce::Position::from_fen(fen);
    [&]() { ASSERT_TRUE(p.has_value()) << fen; }();
    return *p;
}

std::unordered_set<std::string> uci_set(const std::vector<ce::Move>& moves) {
    std::unordered_set<std::string> s;
    for (const auto& m : moves) {
        s.insert(m.to_uci());
    }
    return s;
}

}  // namespace

TEST(LegalMoves, StartingPositionHasTwentyMoves) {
    auto pos = pos_from("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_EQ(pos.generate_legal_moves().size(), 20u);
}

TEST(LegalMoves, KiwipeteHas48Moves) {
    auto pos = pos_from(
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    EXPECT_EQ(pos.generate_legal_moves().size(), 48u);
}

TEST(LegalMoves, Position3Has14Moves) {
    auto pos = pos_from("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
    EXPECT_EQ(pos.generate_legal_moves().size(), 14u);
}

TEST(LegalMoves, Position4Has6Moves) {
    auto pos = pos_from(
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    EXPECT_EQ(pos.generate_legal_moves().size(), 6u);
}

TEST(LegalMoves, Position5Has44Moves) {
    auto pos = pos_from("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
    EXPECT_EQ(pos.generate_legal_moves().size(), 44u);
}

TEST(LegalMoves, Position6Has46Moves) {
    auto pos = pos_from(
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
    EXPECT_EQ(pos.generate_legal_moves().size(), 46u);
}

TEST(LegalMoves, PinnedPieceCannotMoveAwayFromPin) {
    // White king on e1, white bishop on e2, black rook on e8. Bishop is pinned.
    auto pos = pos_from("4r3/8/8/8/8/8/4B3/4K3 w - - 0 1");
    auto uci = uci_set(pos.generate_legal_moves());
    // e2 bishop has no legal squares off the e-file except where it would still be on it.
    // Actually a bishop cannot stay on the e-file when moving — every move takes it off
    // the pin line, so it has zero legal moves.
    int bishop_moves = 0;
    for (const auto& u : uci) {
        if (u.substr(0, 2) == "e2") ++bishop_moves;
    }
    EXPECT_EQ(bishop_moves, 0);
}

TEST(LegalMoves, KingMustEscapeCheck) {
    // White king e1, black queen e8 — must escape check or block/capture.
    auto pos = pos_from("4q3/8/8/8/8/8/8/4K3 w - - 0 1");
    auto uci = uci_set(pos.generate_legal_moves());
    // King can move to d1 or f1 (and not d2 or f2? actually d2 and f2 are not on e-file so safe).
    // d2 and f2 are fine; e2 is on e-file under attack; only d1/d2/f1/f2 escape.
    EXPECT_TRUE(uci.contains("e1d1"));
    EXPECT_TRUE(uci.contains("e1d2"));
    EXPECT_TRUE(uci.contains("e1f1"));
    EXPECT_TRUE(uci.contains("e1f2"));
    EXPECT_FALSE(uci.contains("e1e2"));
}

TEST(LegalMoves, KingCannotMoveIntoCheck) {
    auto pos = pos_from("8/8/8/8/8/8/r7/4K3 w - - 0 1");
    auto uci = uci_set(pos.generate_legal_moves());
    // a2 rook attacks all of rank 2 — king cannot step to d2/e2/f2.
    EXPECT_FALSE(uci.contains("e1d2"));
    EXPECT_FALSE(uci.contains("e1e2"));
    EXPECT_FALSE(uci.contains("e1f2"));
}

TEST(LegalMoves, CheckmateProducesNoLegalMoves) {
    // Fool's mate (variation): white in checkmate.
    auto pos = pos_from("rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 0 3");
    EXPECT_EQ(pos.generate_legal_moves().size(), 0u);
}

TEST(LegalMoves, StalemateProducesNoLegalMoves) {
    // Classic stalemate position.
    auto pos = pos_from("k7/2Q5/8/8/8/8/8/4K3 b - - 0 1");
    // Black king on a8, white queen on c7. Black has no legal move and is NOT in check.
    EXPECT_EQ(pos.generate_legal_moves().size(), 0u);
    EXPECT_FALSE(pos.is_square_attacked(/*a8*/ 56, ce::Color::White));
}
