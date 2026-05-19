#pragma once

#include <string>
#include <string_view>

#include "chess_engine/board.hpp"

namespace chess_engine {

// Full FEN-loadable game position: a Board plus side-to-move, castling rights,
// en-passant target square, halfmove clock, and fullmove number. Future commits
// will implement FEN parsing/serialization and make/unmake on top of this state.
class Position {
public:
    Position();
    explicit Position(std::string_view fen);

    const Board& board() const;
    bool white_to_move() const;
    int halfmove_clock() const;
    int fullmove_number() const;
    std::string to_fen() const;

private:
    Board board_;
};

}  // namespace chess_engine
