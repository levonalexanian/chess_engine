#include <algorithm>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>

#include "chess_engine/move.h"
#include "chess_engine/position.h"

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

TEST(PseudoLegal, StartingPositionHasTwentyMoves) {
    auto pos = pos_from("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    auto moves = pos.generate_pseudo_legal_moves();
    EXPECT_EQ(moves.size(), 20u);

    auto uci = uci_set(moves);
    // 16 pawn moves (8 single + 8 double pushes).
    for (char file = 'a'; file <= 'h'; ++file) {
        EXPECT_TRUE(uci.contains(std::string{file, '2', file, '3'})) << file;
        EXPECT_TRUE(uci.contains(std::string{file, '2', file, '4'})) << file;
    }
    // 4 knight moves.
    EXPECT_TRUE(uci.contains("b1a3"));
    EXPECT_TRUE(uci.contains("b1c3"));
    EXPECT_TRUE(uci.contains("g1f3"));
    EXPECT_TRUE(uci.contains("g1h3"));
}

TEST(PseudoLegal, BlockedDoublePushDropsToSinglePush) {
    // White e2 pawn with a black knight on e3 — single and double pushes blocked.
    auto pos = pos_from("rnbqkbnr/pppppppp/8/8/8/4n3/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    auto uci = uci_set(pos.generate_pseudo_legal_moves());
    EXPECT_FALSE(uci.contains("e2e3"));
    EXPECT_FALSE(uci.contains("e2e4"));
}

TEST(PseudoLegal, PromotionExpandsToFourMoves) {
    auto pos = pos_from("8/P7/8/8/8/8/8/4k2K w - - 0 1");
    auto uci = uci_set(pos.generate_pseudo_legal_moves());
    EXPECT_TRUE(uci.contains("a7a8q"));
    EXPECT_TRUE(uci.contains("a7a8r"));
    EXPECT_TRUE(uci.contains("a7a8b"));
    EXPECT_TRUE(uci.contains("a7a8n"));
}

TEST(PseudoLegal, EnPassantCaptureEmitted) {
    auto pos = pos_from("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3");
    auto moves = pos.generate_pseudo_legal_moves();
    auto uci = uci_set(moves);
    EXPECT_TRUE(uci.contains("e5f6"));
    // confirm the move carries the en-passant flag.
    auto it = std::find_if(moves.begin(), moves.end(), [](const ce::Move& m) {
        return m.to_uci() == "e5f6";
    });
    ASSERT_NE(it, moves.end());
    EXPECT_TRUE(it->is_en_passant());
}

TEST(PseudoLegal, EnPassantOnAFileNoWrap) {
    // White can en-passant capture into a6.
    auto pos = pos_from("8/8/8/Pp6/8/8/8/4k2K w - b6 0 1");
    auto uci = uci_set(pos.generate_pseudo_legal_moves());
    EXPECT_TRUE(uci.contains("a5b6"));
    // White pawn at a5 cannot reach a phantom square that wrapped from h.
    EXPECT_FALSE(uci.contains("h5b6"));
}

TEST(PseudoLegal, EnPassantOnHFileNoWrap) {
    auto pos = pos_from("8/8/8/6pP/8/8/8/4k2K w - g6 0 1");
    auto uci = uci_set(pos.generate_pseudo_legal_moves());
    EXPECT_TRUE(uci.contains("h5g6"));
}

TEST(PseudoLegal, BlackEnPassantOnHFile) {
    auto pos = pos_from("4k2K/8/8/8/6Pp/8/8/8 b - g3 0 1");
    auto uci = uci_set(pos.generate_pseudo_legal_moves());
    EXPECT_TRUE(uci.contains("h4g3"));
}

TEST(PseudoLegal, KnightOnB1HasTwoTargetsInStartPos) {
    auto pos = pos_from("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    auto uci = uci_set(pos.generate_pseudo_legal_moves());
    EXPECT_TRUE(uci.contains("b1a3"));
    EXPECT_TRUE(uci.contains("b1c3"));
    EXPECT_FALSE(uci.contains("b1d2"));  // d2 is own pawn
}

TEST(PseudoLegal, BishopBlockedByOwnPiecesGivesNoMoves) {
    // In starting pos white bishops have no moves.
    auto pos = pos_from("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    auto uci = uci_set(pos.generate_pseudo_legal_moves());
    EXPECT_FALSE(uci.contains("c1d2"));
    EXPECT_FALSE(uci.contains("f1e2"));
}

TEST(PseudoLegal, RookOnEmptyRankReachesAllSquares) {
    auto pos = pos_from("4k3/8/8/8/8/8/8/R3K3 w Q - 0 1");
    auto uci = uci_set(pos.generate_pseudo_legal_moves());
    // a1 rook moves: rank 1 -> b1/c1/d1 (own king on e1 blocks), file a -> a2..a8. 3 + 7 = 10.
    int rook_moves = 0;
    for (const auto& u : uci) {
        if (u.substr(0, 2) == "a1") ++rook_moves;
    }
    EXPECT_EQ(rook_moves, 10);
}

TEST(PseudoLegal, KingMoves_KingOnE1HasFiveTargets) {
    auto pos = pos_from("4k3/8/8/8/8/8/8/4K3 w - - 0 1");
    auto uci = uci_set(pos.generate_pseudo_legal_moves());
    int king_moves = 0;
    for (const auto& u : uci) {
        if (u.substr(0, 2) == "e1") ++king_moves;
    }
    EXPECT_EQ(king_moves, 5);  // d1, d2, e2, f2, f1
}

TEST(PseudoLegal, BlackPromotionExpandsToFourMoves) {
    auto pos = pos_from("4K2k/8/8/8/8/8/p7/8 b - - 0 1");
    auto uci = uci_set(pos.generate_pseudo_legal_moves());
    EXPECT_TRUE(uci.contains("a2a1q"));
    EXPECT_TRUE(uci.contains("a2a1r"));
    EXPECT_TRUE(uci.contains("a2a1b"));
    EXPECT_TRUE(uci.contains("a2a1n"));
}

TEST(PseudoLegal, PawnCapturePromotion) {
    auto pos = pos_from("rn5k/PP6/8/8/8/8/8/7K w - - 0 1");
    auto uci = uci_set(pos.generate_pseudo_legal_moves());
    // a7 can capture b8 and promote (b7 cannot push since b8 has own piece... wait actually b8 has a knight).
    EXPECT_TRUE(uci.contains("a7b8q"));
    EXPECT_TRUE(uci.contains("a7b8n"));
}

TEST(Castling, WhiteBothSidesAvailableWhenClear) {
    auto pos = pos_from("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    auto uci = uci_set(pos.generate_pseudo_legal_moves());
    EXPECT_TRUE(uci.contains("e1g1"));
    EXPECT_TRUE(uci.contains("e1c1"));
}

TEST(Castling, BlackBothSidesAvailableWhenClear) {
    auto pos = pos_from("r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1");
    auto uci = uci_set(pos.generate_pseudo_legal_moves());
    EXPECT_TRUE(uci.contains("e8g8"));
    EXPECT_TRUE(uci.contains("e8c8"));
}

TEST(Castling, NoRightsMeansNoCastlingMoves) {
    auto pos = pos_from("r3k2r/8/8/8/8/8/8/R3K2R w - - 0 1");
    auto uci = uci_set(pos.generate_pseudo_legal_moves());
    EXPECT_FALSE(uci.contains("e1g1"));
    EXPECT_FALSE(uci.contains("e1c1"));
}

TEST(Castling, BlockedPathSuppressesMove) {
    // f1 occupied by white knight blocks kingside.
    auto pos = pos_from("r3k2r/8/8/8/8/8/8/R3KN1R w KQkq - 0 1");
    auto uci = uci_set(pos.generate_pseudo_legal_moves());
    EXPECT_FALSE(uci.contains("e1g1"));
    EXPECT_TRUE(uci.contains("e1c1"));
}

TEST(Castling, KingInCheckSuppressesAllCastling) {
    // Black rook on e2 puts white king in check.
    auto pos = pos_from("4k3/8/8/8/8/8/4r3/R3K2R w KQ - 0 1");
    auto uci = uci_set(pos.generate_pseudo_legal_moves());
    EXPECT_FALSE(uci.contains("e1g1"));
    EXPECT_FALSE(uci.contains("e1c1"));
}

TEST(Castling, KingTransitSquareUnderAttackSuppresses) {
    // Black rook on f2 attacks f1 — king-side transit attacked.
    auto pos = pos_from("4k3/8/8/8/8/8/5r2/R3K2R w KQ - 0 1");
    auto uci = uci_set(pos.generate_pseudo_legal_moves());
    EXPECT_FALSE(uci.contains("e1g1"));
    EXPECT_TRUE(uci.contains("e1c1"));
}

TEST(Castling, QueensideAttackedB1AllowedB1NotKingTransit) {
    // Black rook on b2 attacks b1 — but b1 is NOT a king-transit square; only c1 and d1 are.
    auto pos = pos_from("4k3/8/8/8/8/8/1r6/R3K2R w KQ - 0 1");
    auto uci = uci_set(pos.generate_pseudo_legal_moves());
    EXPECT_TRUE(uci.contains("e1c1"));
}

TEST(Castling, FlagSetOnCastlingMove) {
    auto pos = pos_from("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    auto moves = pos.generate_pseudo_legal_moves();
    auto it = std::find_if(moves.begin(), moves.end(), [](const ce::Move& m) {
        return m.to_uci() == "e1g1";
    });
    ASSERT_NE(it, moves.end());
    EXPECT_TRUE(it->is_castling());
}

TEST(AttackedBy, BasicSliderAndKnight) {
    auto pos = pos_from("8/8/8/8/3k4/8/8/4R2K w - - 0 1");
    // White rook on e1 attacks e-file and rank 1.
    EXPECT_TRUE(pos.is_square_attacked(/*e4*/ 28, ce::Color::White));
    // h1 king attacks g1/g2/h2 etc; not e4.
    EXPECT_FALSE(pos.is_square_attacked(/*a8*/ 56, ce::Color::White));
}
