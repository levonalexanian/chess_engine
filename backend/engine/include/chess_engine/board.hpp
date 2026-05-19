#pragma once

#include <cstdint>
#include <string_view>

namespace chess_engine {

// Bitboard-based board representation: 12 piece bitboards (6 piece types x 2 colors)
// plus per-color occupancy bitboards. Future commits will fill in the bitboard fields
// and the move/unmove logic; this declaration is the stable surface.
class Board {
public:
    Board();
    explicit Board(std::string_view fen);

    std::uint64_t occupancy() const;
    std::uint64_t occupancy_white() const;
    std::uint64_t occupancy_black() const;
};

}  // namespace chess_engine
