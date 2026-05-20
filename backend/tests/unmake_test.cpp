#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "chess_engine/move.h"
#include "chess_engine/position.h"

namespace ce = chess_engine;

static ce::Position pos_from(const char* fen) {
    auto p = ce::Position::from_fen(fen);
    [&]() { ASSERT_TRUE(p.has_value()) << fen; }();
    return *p;
}

TEST(Unmake, SinglePushRoundTrip) {
    auto before = pos_from("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    auto pos = before;
    auto undo = pos.make_move(*ce::Move::from_uci("e2e4"));
    pos.unmake_move(undo);
    EXPECT_EQ(pos, before);
    EXPECT_EQ(pos.zobrist_hash(), before.zobrist_hash());
    EXPECT_EQ(pos.to_fen(), before.to_fen());
}

TEST(Unmake, CaptureRoundTrip) {
    auto before = pos_from("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2");
    auto pos = before;
    // pawn-on-pawn capture? actually e4 and e5 face off — let's pick d4-e5 capture: but no d4 pawn.
    // Use a simpler: knight g1f3, then knight f3e5 capturing.
    auto undo1 = pos.make_move(*ce::Move::from_uci("g1f3"));
    auto mid = pos;
    auto undo2 = pos.make_move(*ce::Move::from_uci("d7d5"));
    pos.unmake_move(undo2);
    EXPECT_EQ(pos, mid);
    pos.unmake_move(undo1);
    EXPECT_EQ(pos, before);
    EXPECT_EQ(pos.zobrist_hash(), before.zobrist_hash());
}

TEST(Unmake, PromotionRoundTrip) {
    auto before = pos_from("8/P7/8/8/8/8/8/4k2K w - - 0 1");
    auto pos = before;
    auto m = ce::Move(/*from a7*/ 48, /*to a8*/ 56, ce::Move::FlagPromotion, ce::Move::PromoQueen);
    auto undo = pos.make_move(m);
    EXPECT_EQ(*pos.board().piece_at(56), ce::Piece::WhiteQueen);
    pos.unmake_move(undo);
    EXPECT_EQ(pos, before);
    EXPECT_EQ(pos.zobrist_hash(), before.zobrist_hash());
}

TEST(Unmake, PromotionWithCaptureRoundTrip) {
    auto before = pos_from("rn5k/PP6/8/8/8/8/8/7K w - - 0 1");
    auto pos = before;
    auto m = ce::Move(/*a7*/ 48, /*b8*/ 57, ce::Move::FlagPromotion, ce::Move::PromoKnight);
    auto undo = pos.make_move(m);
    EXPECT_EQ(*pos.board().piece_at(57), ce::Piece::WhiteKnight);
    pos.unmake_move(undo);
    EXPECT_EQ(pos, before);
    EXPECT_EQ(pos.zobrist_hash(), before.zobrist_hash());
}

TEST(Unmake, EnPassantRoundTrip) {
    auto before = pos_from("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3");
    auto pos = before;
    auto m = ce::Move(/*e5*/ 36, /*f6*/ 45, ce::Move::FlagEnPassant);
    auto undo = pos.make_move(m);
    // f5 should be empty after en-passant capture.
    EXPECT_FALSE(pos.board().piece_at(/*f5*/ 37).has_value());
    pos.unmake_move(undo);
    EXPECT_EQ(pos, before);
    EXPECT_EQ(pos.zobrist_hash(), before.zobrist_hash());
}

TEST(Unmake, CastlingKingsideRoundTrip) {
    auto before = pos_from("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    auto pos = before;
    auto m = ce::Move(/*e1*/ 4, /*g1*/ 6, ce::Move::FlagCastling);
    auto undo = pos.make_move(m);
    EXPECT_EQ(*pos.board().piece_at(/*g1*/ 6), ce::Piece::WhiteKing);
    EXPECT_EQ(*pos.board().piece_at(/*f1*/ 5), ce::Piece::WhiteRook);
    pos.unmake_move(undo);
    EXPECT_EQ(pos, before);
    EXPECT_EQ(pos.zobrist_hash(), before.zobrist_hash());
}

TEST(Unmake, CastlingQueensideRoundTrip) {
    auto before = pos_from("r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1");
    auto pos = before;
    auto m = ce::Move(/*e8*/ 60, /*c8*/ 58, ce::Move::FlagCastling);
    auto undo = pos.make_move(m);
    EXPECT_EQ(*pos.board().piece_at(/*c8*/ 58), ce::Piece::BlackKing);
    EXPECT_EQ(*pos.board().piece_at(/*d8*/ 59), ce::Piece::BlackRook);
    pos.unmake_move(undo);
    EXPECT_EQ(pos, before);
    EXPECT_EQ(pos.zobrist_hash(), before.zobrist_hash());
}

TEST(Unmake, LongSequenceRoundTrips) {
    auto before = pos_from("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    auto pos = before;
    const std::vector<std::string> moves = {
        "e2e4", "e7e5", "g1f3", "b8c6", "f1c4", "f8c5",
        "e1g1", "g8f6", "d2d3", "d7d6", "b1c3", "c8e6",
    };
    std::vector<ce::UndoInfo> undos;
    for (const auto& u : moves) {
        auto parsed = ce::Move::from_uci(u);
        ASSERT_TRUE(parsed.has_value());
        // We need flag inference for castling. Use make_move(string) which delegates.
        // To keep stack of UndoInfos we'd need to expose, so use a custom approach:
        // construct a Move with castling flag where appropriate.
        ce::Move m = *parsed;
        const int from = m.from();
        const int to = m.to();
        if (auto pc = pos.board().piece_at(from); pc.has_value() &&
            ce::piece_type_of(*pc) == ce::PieceType::King &&
            (from & 7) == 4 && ((to & 7) == 6 || (to & 7) == 2)) {
            m = ce::Move(static_cast<std::uint8_t>(from), static_cast<std::uint8_t>(to),
                         ce::Move::FlagCastling);
        }
        undos.push_back(pos.make_move(m));
        EXPECT_EQ(pos.zobrist_hash(), pos.compute_zobrist_hash()) << u;
    }
    for (auto it = undos.rbegin(); it != undos.rend(); ++it) {
        pos.unmake_move(*it);
        EXPECT_EQ(pos.zobrist_hash(), pos.compute_zobrist_hash());
    }
    EXPECT_EQ(pos, before);
    EXPECT_EQ(pos.zobrist_hash(), before.zobrist_hash());
}

TEST(MakeMoveUciDelegation, EnPassantFromUci) {
    auto before = pos_from("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3");
    auto pos = before;
    ASSERT_TRUE(pos.make_move("e5f6"));
    // After ep capture, f5 (black pawn) should be removed.
    EXPECT_FALSE(pos.board().piece_at(/*f5*/ 37).has_value());
    EXPECT_EQ(*pos.board().piece_at(/*f6*/ 45), ce::Piece::WhitePawn);
}

TEST(MakeMoveUciDelegation, CastlingFromUci) {
    auto pos = pos_from(
        "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 4");
    ASSERT_TRUE(pos.make_move("e1g1"));
    EXPECT_EQ(*pos.board().piece_at(/*g1*/ 6), ce::Piece::WhiteKing);
    EXPECT_EQ(*pos.board().piece_at(/*f1*/ 5), ce::Piece::WhiteRook);
}
