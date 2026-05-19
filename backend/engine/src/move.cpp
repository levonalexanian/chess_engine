#include "chess_engine/move.hpp"

namespace chess_engine {

Move::Move() = default;

std::uint8_t Move::from() const {
    return 0;
}

std::uint8_t Move::to() const {
    return 0;
}

std::uint8_t Move::promotion() const {
    return 0;
}

std::uint8_t Move::flags() const {
    return 0;
}

std::uint16_t Move::raw() const {
    return data_;
}

}  // namespace chess_engine
