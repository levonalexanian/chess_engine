#include "chess_engine/position.hpp"

namespace chess_engine {

Position::Position() = default;

Position::Position(std::string_view) {}

const Board& Position::board() const {
    return board_;
}

bool Position::white_to_move() const {
    return true;
}

int Position::halfmove_clock() const {
    return 0;
}

int Position::fullmove_number() const {
    return 1;
}

std::string Position::to_fen() const {
    return {};
}

}  // namespace chess_engine
