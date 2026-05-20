#ifndef CHESS_ENGINE_BOARD_H
#define CHESS_ENGINE_BOARD_H

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace chess_engine {

using Bitboard = std::uint64_t;

enum Color : std::uint8_t {
    White = 0,
    Black = 1,
};

enum PieceType : std::uint8_t {
    Pawn = 0,
    Knight = 1,
    Bishop = 2,
    Rook = 3,
    Queen = 4,
    King = 5,
};

enum Piece : std::uint8_t {
    WhitePawn = 0,
    WhiteKnight = 1,
    WhiteBishop = 2,
    WhiteRook = 3,
    WhiteQueen = 4,
    WhiteKing = 5,
    BlackPawn = 6,
    BlackKnight = 7,
    BlackBishop = 8,
    BlackRook = 9,
    BlackQueen = 10,
    BlackKing = 11,
    NoPiece = 12,
};

constexpr int kNumPieces = 12;
constexpr int kNumSquares = 64;

constexpr Color color_of(Piece p) {
    return p < BlackPawn ? Color::White : Color::Black;
}

constexpr PieceType piece_type_of(Piece p) {
    return static_cast<PieceType>(p % 6);
}

constexpr Piece make_piece(Color c, PieceType t) {
    return static_cast<Piece>(static_cast<std::uint8_t>(c) * 6 + static_cast<std::uint8_t>(t));
}

class Board {
public:
    Board();
    explicit Board(std::string_view fen);

    void clear();
    void set_piece(int square, Piece piece);
    void remove_piece(int square);

    std::optional<Piece> piece_at(int square) const;
    Bitboard pieces(Piece piece) const { return pieces_[piece]; }

    Bitboard occupancy() const { return occupancy_all_; }
    Bitboard occupancy_white() const { return occupancy_white_; }
    Bitboard occupancy_black() const { return occupancy_black_; }
    Bitboard occupancy(Color c) const { return c == Color::White ? occupancy_white_ : occupancy_black_; }

private:
    void recompute_occupancy();

    std::array<Bitboard, kNumPieces> pieces_{};
    Bitboard occupancy_white_{0};
    Bitboard occupancy_black_{0};
    Bitboard occupancy_all_{0};
};

}  // namespace chess_engine

#endif  // CHESS_ENGINE_BOARD_H
