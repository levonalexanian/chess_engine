#include "chess_engine/board.hpp"

namespace chess_engine {

Board::Board() = default;

Board::Board(std::string_view) {}

std::uint64_t Board::occupancy() const {
    return 0;
}

std::uint64_t Board::occupancy_white() const {
    return 0;
}

std::uint64_t Board::occupancy_black() const {
    return 0;
}

}  // namespace chess_engine
