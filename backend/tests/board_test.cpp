#include <cstdint>

#include <gtest/gtest.h>

#include "chess_engine/board.hpp"

namespace ce = chess_engine;

TEST(Board, DefaultConstructedIsEmpty) {
    ce::Board board;
    EXPECT_EQ(board.occupancy(), 0u);
    EXPECT_EQ(board.occupancy_white(), 0u);
    EXPECT_EQ(board.occupancy_black(), 0u);
    for (int sq = 0; sq < ce::kNumSquares; ++sq) {
        EXPECT_FALSE(board.piece_at(sq).has_value());
    }
}

TEST(Board, SetPiecePlacesPieceAtSquare) {
    ce::Board board;
    board.set_piece(0, ce::Piece::WhiteRook);
    auto p = board.piece_at(0);
    ASSERT_TRUE(p.has_value());
    EXPECT_EQ(*p, ce::Piece::WhiteRook);
    EXPECT_EQ(board.occupancy(), 1ULL);
    EXPECT_EQ(board.occupancy_white(), 1ULL);
    EXPECT_EQ(board.occupancy_black(), 0ULL);
    EXPECT_EQ(board.pieces(ce::Piece::WhiteRook), 1ULL);
}

TEST(Board, RemovePieceClearsSquare) {
    ce::Board board;
    board.set_piece(7, ce::Piece::BlackKing);
    EXPECT_TRUE(board.piece_at(7).has_value());
    board.remove_piece(7);
    EXPECT_FALSE(board.piece_at(7).has_value());
    EXPECT_EQ(board.occupancy(), 0u);
}

TEST(Board, SetPieceOverwritesPreviousOccupant) {
    ce::Board board;
    board.set_piece(28, ce::Piece::WhiteKnight);
    board.set_piece(28, ce::Piece::BlackQueen);
    auto p = board.piece_at(28);
    ASSERT_TRUE(p.has_value());
    EXPECT_EQ(*p, ce::Piece::BlackQueen);
    EXPECT_EQ(board.pieces(ce::Piece::WhiteKnight), 0u);
    EXPECT_EQ(board.occupancy_white(), 0u);
    EXPECT_EQ(board.occupancy_black(), 1ULL << 28);
}

TEST(Board, OccupancyAggregatesAcrossColors) {
    ce::Board board;
    board.set_piece(0, ce::Piece::WhiteRook);
    board.set_piece(63, ce::Piece::BlackRook);
    board.set_piece(4, ce::Piece::WhiteKing);
    board.set_piece(60, ce::Piece::BlackKing);
    EXPECT_EQ(board.occupancy_white(), (1ULL << 0) | (1ULL << 4));
    EXPECT_EQ(board.occupancy_black(), (1ULL << 60) | (1ULL << 63));
    EXPECT_EQ(board.occupancy(),
              (1ULL << 0) | (1ULL << 4) | (1ULL << 60) | (1ULL << 63));
}

TEST(Board, ClearResetsAllState) {
    ce::Board board;
    board.set_piece(10, ce::Piece::WhitePawn);
    board.set_piece(30, ce::Piece::BlackBishop);
    board.clear();
    EXPECT_EQ(board.occupancy(), 0u);
    EXPECT_FALSE(board.piece_at(10).has_value());
    EXPECT_FALSE(board.piece_at(30).has_value());
}

TEST(BoardHelpers, ColorOfAndPieceTypeOfRoundTrip) {
    for (int i = 0; i < ce::kNumPieces; ++i) {
        auto piece = static_cast<ce::Piece>(i);
        auto color = ce::color_of(piece);
        auto type = ce::piece_type_of(piece);
        EXPECT_EQ(ce::make_piece(color, type), piece);
    }
}
