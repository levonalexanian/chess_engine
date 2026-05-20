#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "chess_engine/board.h"
#include "chess_engine/position.h"

namespace ce = chess_engine;

namespace {

constexpr const char* kStartingFen =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
constexpr const char* kKiwipete =
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";

ce::Position position_from(const char* fen) {
    auto p = ce::Position::from_fen(fen);
    [&]() { ASSERT_TRUE(p.has_value()) << fen; }();
    return *p;
}

void apply_sequence(ce::Position& p, std::vector<std::string> const& moves) {
    for (const auto& m : moves) {
        ASSERT_TRUE(p.make_move(m)) << m;
    }
}

}  // namespace

TEST(MakeMove, SinglePawnPushAdvancesState) {
    auto pos = position_from(kStartingFen);
    ASSERT_TRUE(pos.make_move("e2e4"));
    EXPECT_FALSE(pos.white_to_move());
    EXPECT_EQ(pos.halfmove_clock(), 0);
    EXPECT_EQ(pos.fullmove_number(), 1);
    ASSERT_TRUE(pos.en_passant_square().has_value());
    EXPECT_EQ(*pos.en_passant_square(), 2 * 8 + 4);  // e3
    EXPECT_EQ(pos.to_fen(),
              "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
}

TEST(MakeMove, BlackReplyIncrementsFullmove) {
    auto pos = position_from(kStartingFen);
    apply_sequence(pos, {"e2e4", "e7e5"});
    EXPECT_TRUE(pos.white_to_move());
    EXPECT_EQ(pos.fullmove_number(), 2);
    ASSERT_TRUE(pos.en_passant_square().has_value());
    EXPECT_EQ(*pos.en_passant_square(), 5 * 8 + 4);  // e6
}

TEST(MakeMove, NonPawnMoveIncrementsHalfmoveClock) {
    auto pos = position_from(kStartingFen);
    apply_sequence(pos, {"g1f3"});
    EXPECT_EQ(pos.halfmove_clock(), 1);
    EXPECT_FALSE(pos.en_passant_square().has_value());
}

TEST(MakeMove, CaptureResetsHalfmoveAndRemovesPiece) {
    auto pos = position_from("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2");
    ASSERT_TRUE(pos.make_move("e4e5"));  // not a capture — just a pawn push
    auto captured_pos = position_from("rnbqkbnr/pppp1ppp/8/4p3/3P4/8/PPP1PPPP/RNBQKBNR b KQkq d3 0 2");
    ASSERT_TRUE(captured_pos.make_move("e5d4"));
    EXPECT_EQ(captured_pos.halfmove_clock(), 0);
    EXPECT_EQ(*captured_pos.board().piece_at(3 * 8 + 3), ce::Piece::BlackPawn);
    EXPECT_FALSE(captured_pos.board().piece_at(2 * 8 + 4).has_value());
}

TEST(MakeMove, WhiteCastlesKingside) {
    auto pos = position_from(
        "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 4");
    ASSERT_TRUE(pos.make_move("e1g1"));
    EXPECT_EQ(*pos.board().piece_at(6), ce::Piece::WhiteKing);
    EXPECT_EQ(*pos.board().piece_at(5), ce::Piece::WhiteRook);
    EXPECT_FALSE(pos.board().piece_at(4).has_value());
    EXPECT_FALSE(pos.board().piece_at(7).has_value());
    EXPECT_FALSE(pos.castling().has(ce::CastlingRights::WhiteKingSide));
    EXPECT_FALSE(pos.castling().has(ce::CastlingRights::WhiteQueenSide));
    EXPECT_TRUE(pos.castling().has(ce::CastlingRights::BlackKingSide));
    EXPECT_TRUE(pos.castling().has(ce::CastlingRights::BlackQueenSide));
}

TEST(MakeMove, BlackCastlesQueenside) {
    auto pos = position_from(
        "r3kbnr/ppp1pppp/2nq4/3p4/3P4/2NQ4/PPP1PPPP/R1B1KBNR b KQkq - 4 4");
    ASSERT_TRUE(pos.make_move("e8c8"));
    EXPECT_EQ(*pos.board().piece_at(58), ce::Piece::BlackKing);
    EXPECT_EQ(*pos.board().piece_at(59), ce::Piece::BlackRook);
    EXPECT_FALSE(pos.castling().has(ce::CastlingRights::BlackKingSide));
    EXPECT_FALSE(pos.castling().has(ce::CastlingRights::BlackQueenSide));
}

TEST(MakeMove, EnPassantCaptureRemovesCapturedPawn) {
    auto pos = position_from(
        "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3");
    ASSERT_TRUE(pos.make_move("e5f6"));
    EXPECT_EQ(*pos.board().piece_at(5 * 8 + 5), ce::Piece::WhitePawn);   // f6 has white pawn
    EXPECT_FALSE(pos.board().piece_at(4 * 8 + 5).has_value());            // f5 cleared
    EXPECT_FALSE(pos.board().piece_at(4 * 8 + 4).has_value());            // e5 cleared
    EXPECT_EQ(pos.halfmove_clock(), 0);
    EXPECT_FALSE(pos.en_passant_square().has_value());
}

TEST(MakeMove, PromotionReplacesPawnWithChosenPiece) {
    auto pos = position_from("8/P7/8/8/8/8/8/4k2K w - - 0 1");
    ASSERT_TRUE(pos.make_move("a7a8q"));
    EXPECT_EQ(*pos.board().piece_at(56), ce::Piece::WhiteQueen);
    EXPECT_FALSE(pos.board().piece_at(48).has_value());
    EXPECT_EQ(pos.halfmove_clock(), 0);
}

TEST(MakeMove, PromotionWithCapture) {
    auto pos = position_from("rn5k/PP6/8/8/8/8/8/7K w - - 0 1");
    ASSERT_TRUE(pos.make_move("a7b8n"));
    EXPECT_EQ(*pos.board().piece_at(57), ce::Piece::WhiteKnight);
    EXPECT_FALSE(pos.board().piece_at(48).has_value());
    EXPECT_EQ(pos.halfmove_clock(), 0);
}

TEST(MakeMove, RookMoveRemovesMatchingCastlingRight) {
    auto pos = position_from(
        "rnbqk2r/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    ASSERT_TRUE(pos.make_move("h1g1"));
    EXPECT_FALSE(pos.castling().has(ce::CastlingRights::WhiteKingSide));
    EXPECT_TRUE(pos.castling().has(ce::CastlingRights::WhiteQueenSide));
}

TEST(MakeMove, CaptureOnCornerRemovesOpponentsRight) {
    auto pos = position_from(
        "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    ASSERT_TRUE(pos.make_move("a1a8"));
    EXPECT_FALSE(pos.castling().has(ce::CastlingRights::BlackQueenSide));
    EXPECT_TRUE(pos.castling().has(ce::CastlingRights::BlackKingSide));
    EXPECT_FALSE(pos.castling().has(ce::CastlingRights::WhiteQueenSide));
    EXPECT_TRUE(pos.castling().has(ce::CastlingRights::WhiteKingSide));
}

TEST(MakeMove, RejectsMalformedAndEmptySource) {
    auto pos = position_from(kStartingFen);
    EXPECT_FALSE(pos.make_move("e9e4"));
    EXPECT_FALSE(pos.make_move("garbage"));
    EXPECT_FALSE(pos.make_move("0000"));
    EXPECT_FALSE(pos.make_move("e4e5"));  // source empty in starting position
}

TEST(IncrementalZobrist, MatchesFromScratchAfterEachPly) {
    auto pos = position_from(kStartingFen);
    const std::vector<std::string> moves = {
        "e2e4", "e7e5", "g1f3", "b8c6", "f1c4", "f8c5",
        "e1g1", "g8f6", "d2d3", "d7d6", "b1c3", "c8e6",
    };
    for (const auto& m : moves) {
        ASSERT_TRUE(pos.make_move(m)) << m;
        EXPECT_EQ(pos.zobrist_hash(), pos.compute_zobrist_hash()) << m;
    }
}

TEST(IncrementalZobrist, KiwipeteCastlingAndCaptureScenarios) {
    auto pos = position_from(kKiwipete);
    const std::vector<std::string> moves = {
        "e1g1", "a8b8", "e5g6", "f7g6", "a1b1", "g7h8",
    };
    for (const auto& m : moves) {
        ASSERT_TRUE(pos.make_move(m)) << m;
        EXPECT_EQ(pos.zobrist_hash(), pos.compute_zobrist_hash()) << m;
    }
}

TEST(IncrementalZobrist, EnPassantCaptureMatchesFromScratch) {
    auto pos = position_from(
        "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3");
    ASSERT_TRUE(pos.make_move("e5f6"));
    EXPECT_EQ(pos.zobrist_hash(), pos.compute_zobrist_hash());
}

TEST(IncrementalZobrist, PromotionMatchesFromScratch) {
    auto pos = position_from("8/P7/8/8/8/8/8/4k2K w - - 0 1");
    ASSERT_TRUE(pos.make_move("a7a8q"));
    EXPECT_EQ(pos.zobrist_hash(), pos.compute_zobrist_hash());
}

TEST(IncrementalZobrist, CastlingMatchesFromScratch) {
    auto pos = position_from(
        "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 4");
    ASSERT_TRUE(pos.make_move("e1g1"));
    EXPECT_EQ(pos.zobrist_hash(), pos.compute_zobrist_hash());
}
