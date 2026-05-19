#include "chess_engine/position.hpp"

namespace chess_engine {

namespace {

constexpr const char* kStartingFen =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

}  // namespace

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
    return kStartingFen;
}

std::string Position::fen() const {
    return kStartingFen;
}

}  // namespace chess_engine
