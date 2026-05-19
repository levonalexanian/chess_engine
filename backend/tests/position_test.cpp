#include <optional>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include "chess_engine/board.hpp"
#include "chess_engine/position.hpp"

namespace ce = chess_engine;

namespace {

constexpr const char* kStartingFen =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
constexpr const char* kKiwipete =
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
constexpr const char* kPosition3 =
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1";
constexpr const char* kPosition4 =
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
constexpr const char* kPosition5 =
    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
constexpr const char* kPosition6 =
    "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10";

}  // namespace

TEST(Position, DefaultIsStartingPosition) {
    ce::Position pos;
    EXPECT_EQ(pos.to_fen(), kStartingFen);
    EXPECT_TRUE(pos.white_to_move());
    EXPECT_EQ(pos.halfmove_clock(), 0);
    EXPECT_EQ(pos.fullmove_number(), 1);
    EXPECT_FALSE(pos.en_passant_square().has_value());
    EXPECT_TRUE(pos.castling().has(ce::CastlingRights::WhiteKingSide));
    EXPECT_TRUE(pos.castling().has(ce::CastlingRights::WhiteQueenSide));
    EXPECT_TRUE(pos.castling().has(ce::CastlingRights::BlackKingSide));
    EXPECT_TRUE(pos.castling().has(ce::CastlingRights::BlackQueenSide));
}

TEST(Position, FromFenStartingPositionPlacement) {
    auto pos = ce::Position::from_fen(kStartingFen);
    ASSERT_TRUE(pos.has_value());
    EXPECT_EQ(*pos->board().piece_at(0), ce::Piece::WhiteRook);
    EXPECT_EQ(*pos->board().piece_at(4), ce::Piece::WhiteKing);
    EXPECT_EQ(*pos->board().piece_at(7), ce::Piece::WhiteRook);
    EXPECT_EQ(*pos->board().piece_at(56), ce::Piece::BlackRook);
    EXPECT_EQ(*pos->board().piece_at(60), ce::Piece::BlackKing);
    EXPECT_EQ(*pos->board().piece_at(63), ce::Piece::BlackRook);
    EXPECT_EQ(*pos->board().piece_at(8), ce::Piece::WhitePawn);
    EXPECT_EQ(*pos->board().piece_at(48), ce::Piece::BlackPawn);
    EXPECT_FALSE(pos->board().piece_at(24).has_value());
}

TEST(Position, FenRoundTripPerftPositions) {
    for (const char* fen : {kStartingFen, kKiwipete, kPosition3, kPosition4, kPosition5, kPosition6}) {
        auto pos = ce::Position::from_fen(fen);
        ASSERT_TRUE(pos.has_value()) << fen;
        EXPECT_EQ(pos->to_fen(), fen) << fen;
    }
}

TEST(Position, ParsesBlackToMoveAndCounters) {
    const std::string fen = "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 2";
    auto pos = ce::Position::from_fen(fen);
    ASSERT_TRUE(pos.has_value());
    EXPECT_FALSE(pos->white_to_move());
    EXPECT_EQ(pos->halfmove_clock(), 0);
    EXPECT_EQ(pos->fullmove_number(), 2);
    ASSERT_TRUE(pos->en_passant_square().has_value());
    EXPECT_EQ(*pos->en_passant_square(), 2 * 8 + 4);  // e3
    EXPECT_EQ(pos->to_fen(), fen);
}

TEST(Position, NoCastlingRightsSerializesDash) {
    const std::string fen = "8/8/8/4k3/4K3/8/8/8 w - - 0 1";
    auto pos = ce::Position::from_fen(fen);
    ASSERT_TRUE(pos.has_value());
    EXPECT_EQ(pos->castling().mask, 0u);
    EXPECT_EQ(pos->to_fen(), fen);
}

TEST(Position, FromFenRejectsMalformed) {
    EXPECT_FALSE(ce::Position::from_fen("").has_value());
    EXPECT_FALSE(ce::Position::from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq").has_value());
    EXPECT_FALSE(ce::Position::from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR x KQkq - 0 1").has_value());
    EXPECT_FALSE(ce::Position::from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w XYZ - 0 1").has_value());
    EXPECT_FALSE(ce::Position::from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq z9 0 1").has_value());
    EXPECT_FALSE(ce::Position::from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBN w KQkq - 0 1").has_value());
}

TEST(Position, EqualityOperator) {
    auto a = ce::Position::from_fen(kStartingFen);
    auto b = ce::Position::from_fen(kStartingFen);
    auto c = ce::Position::from_fen(kKiwipete);
    ASSERT_TRUE(a.has_value());
    ASSERT_TRUE(b.has_value());
    ASSERT_TRUE(c.has_value());
    EXPECT_EQ(*a, *b);
    EXPECT_NE(*a, *c);
}
