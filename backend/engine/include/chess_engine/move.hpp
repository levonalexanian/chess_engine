#pragma once

#include <cstdint>

namespace chess_engine {

// 16-bit packed move: 6 bits from-square, 6 bits to-square, 2 bits promotion piece,
// 2 bits flags (capture, castle, en passant, double-push, ...). Future commits will
// settle the exact bit layout and add encode/decode helpers.
class Move {
public:
    Move();

    std::uint8_t from() const;
    std::uint8_t to() const;
    std::uint8_t promotion() const;
    std::uint8_t flags() const;

    std::uint16_t raw() const;

private:
    std::uint16_t data_{0};
};

}  // namespace chess_engine
