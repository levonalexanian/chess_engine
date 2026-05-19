#include "chess_engine/board.hpp"

namespace chess_engine {

Board::Board() = default;

Board::Board(std::string_view) : Board() {}

void Board::clear() {
    for (auto& bb : pieces_) {
        bb = 0;
    }
    occupancy_white_ = 0;
    occupancy_black_ = 0;
    occupancy_all_ = 0;
}

void Board::set_piece(int square, Piece piece) {
    const Bitboard mask = Bitboard{1} << square;
    if (auto existing = piece_at(square); existing.has_value()) {
        pieces_[*existing] &= ~mask;
    }
    pieces_[piece] |= mask;
    recompute_occupancy();
}

void Board::remove_piece(int square) {
    const Bitboard mask = Bitboard{1} << square;
    for (auto& bb : pieces_) {
        bb &= ~mask;
    }
    recompute_occupancy();
}

std::optional<Piece> Board::piece_at(int square) const {
    const Bitboard mask = Bitboard{1} << square;
    for (int i = 0; i < kNumPieces; ++i) {
        if (pieces_[i] & mask) {
            return static_cast<Piece>(i);
        }
    }
    return std::nullopt;
}

void Board::recompute_occupancy() {
    occupancy_white_ = 0;
    for (int i = WhitePawn; i <= WhiteKing; ++i) {
        occupancy_white_ |= pieces_[i];
    }
    occupancy_black_ = 0;
    for (int i = BlackPawn; i <= BlackKing; ++i) {
        occupancy_black_ |= pieces_[i];
    }
    occupancy_all_ = occupancy_white_ | occupancy_black_;
}

}  // namespace chess_engine
