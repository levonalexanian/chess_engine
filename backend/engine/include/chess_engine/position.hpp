#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "chess_engine/board.hpp"
#include "chess_engine/move.hpp"

namespace chess_engine {

struct CastlingRights {
    static constexpr std::uint8_t WhiteKingSide = 1 << 0;
    static constexpr std::uint8_t WhiteQueenSide = 1 << 1;
    static constexpr std::uint8_t BlackKingSide = 1 << 2;
    static constexpr std::uint8_t BlackQueenSide = 1 << 3;

    std::uint8_t mask{0};

    bool has(std::uint8_t bit) const { return (mask & bit) != 0; }
    void set(std::uint8_t bit) { mask |= bit; }
    void clear(std::uint8_t bit) { mask &= static_cast<std::uint8_t>(~bit); }

    friend bool operator==(CastlingRights a, CastlingRights b) { return a.mask == b.mask; }
};

class Position {
public:
    struct EmptyTag {};

    Position();
    explicit Position(std::string_view fen);
    explicit Position(EmptyTag) {}

    static std::optional<Position> from_fen(std::string_view fen);

    const Board& board() const { return board_; }
    Board& board() { return board_; }

    Color side_to_move() const { return side_to_move_; }
    bool white_to_move() const { return side_to_move_ == Color::White; }
    CastlingRights castling() const { return castling_; }
    std::optional<int> en_passant_square() const { return en_passant_square_; }
    int halfmove_clock() const { return halfmove_clock_; }
    int fullmove_number() const { return fullmove_number_; }
    std::uint64_t zobrist_hash() const { return zobrist_hash_; }

    std::uint64_t compute_zobrist_hash() const;

    // Apply a UCI move to the position. Does not validate legality (move
    // generation lives on the next branch); use it for now to advance state
    // through a known-legal move list. Returns false only when the UCI string
    // is malformed or names an empty source square.
    bool make_move(std::string_view uci);

    // Generate every pseudo-legal move for the side to move. Pseudo-legal
    // means: produced by the standard movement rules of each piece (including
    // captures, double pushes, promotions, en passant, and castling), but the
    // moving side may still be left in check. Use generate_legal_moves to get
    // the filtered list.
    std::vector<Move> generate_pseudo_legal_moves() const;

    // True iff `square` is attacked by any piece of `attacker` color in the
    // current position. Used by the castling generator and the legality filter.
    bool is_square_attacked(int square, Color attacker) const;

    std::string to_fen() const;
    std::string fen() const;

    friend bool operator==(const Position& a, const Position& b);
    friend bool operator!=(const Position& a, const Position& b) { return !(a == b); }

private:
    Board board_;
    Color side_to_move_{Color::White};
    CastlingRights castling_{};
    std::optional<int> en_passant_square_{};
    int halfmove_clock_{0};
    int fullmove_number_{1};
    std::uint64_t zobrist_hash_{0};
};

}  // namespace chess_engine
