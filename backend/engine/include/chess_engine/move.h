#ifndef CHESS_ENGINE_MOVE_H
#define CHESS_ENGINE_MOVE_H

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace chess_engine {

// 16-bit packed move layout:
//   bits 0-5   : from-square (0-63, A1=0..H8=63)
//   bits 6-11  : to-square   (0-63)
//   bits 12-13 : promotion piece (0=Knight, 1=Bishop, 2=Rook, 3=Queen)
//   bits 14-15 : flags       (0=None, 1=Promotion, 2=EnPassant, 3=Castling)
// A default-constructed Move has raw() == 0 and serializes to "0000".
class Move {
public:
    enum Flag : std::uint8_t {
        FlagNone = 0,
        FlagPromotion = 1,
        FlagEnPassant = 2,
        FlagCastling = 3,
    };

    enum Promo : std::uint8_t {
        PromoKnight = 0,
        PromoBishop = 1,
        PromoRook = 2,
        PromoQueen = 3,
    };

    Move() = default;
    Move(std::uint8_t from_sq, std::uint8_t to_sq, Flag flag = FlagNone, Promo promo = PromoKnight);

    std::uint8_t from() const { return static_cast<std::uint8_t>(data_ & 0x3F); }
    std::uint8_t to() const { return static_cast<std::uint8_t>((data_ >> 6) & 0x3F); }
    std::uint8_t promotion() const { return static_cast<std::uint8_t>((data_ >> 12) & 0x3); }
    std::uint8_t flags() const { return static_cast<std::uint8_t>((data_ >> 14) & 0x3); }
    std::uint16_t raw() const { return data_; }

    bool is_promotion() const { return flags() == FlagPromotion; }
    bool is_en_passant() const { return flags() == FlagEnPassant; }
    bool is_castling() const { return flags() == FlagCastling; }

    std::string to_uci() const;
    static std::optional<Move> from_uci(std::string_view uci);

    friend bool operator==(Move a, Move b) { return a.data_ == b.data_; }
    friend bool operator!=(Move a, Move b) { return a.data_ != b.data_; }

private:
    std::uint16_t data_{0};
};

}  // namespace chess_engine

#endif  // CHESS_ENGINE_MOVE_H
