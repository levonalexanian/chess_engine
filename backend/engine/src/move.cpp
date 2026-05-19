#include "chess_engine/move.hpp"

#include <cctype>
#include <cstdint>

namespace chess_engine {

namespace {

constexpr const char* kPromoChars = "nbrq";

std::optional<int> parse_square(char file, char rank) {
    if (file < 'a' || file > 'h') {
        return std::nullopt;
    }
    if (rank < '1' || rank > '8') {
        return std::nullopt;
    }
    const int f = file - 'a';
    const int r = rank - '1';
    return r * 8 + f;
}

}  // namespace

Move::Move(std::uint8_t from_sq, std::uint8_t to_sq, Flag flag, Promo promo) {
    data_ = static_cast<std::uint16_t>(
        (from_sq & 0x3F) |
        ((to_sq & 0x3F) << 6) |
        ((static_cast<std::uint16_t>(promo) & 0x3) << 12) |
        ((static_cast<std::uint16_t>(flag) & 0x3) << 14));
}

std::string Move::to_uci() const {
    if (data_ == 0) {
        return "0000";
    }
    auto square_to_string = [](std::uint8_t sq) {
        std::string s;
        s.push_back(static_cast<char>('a' + (sq & 7)));
        s.push_back(static_cast<char>('1' + ((sq >> 3) & 7)));
        return s;
    };
    std::string out = square_to_string(from()) + square_to_string(to());
    if (is_promotion()) {
        out.push_back(kPromoChars[promotion() & 0x3]);
    }
    return out;
}

std::optional<Move> Move::from_uci(std::string_view uci) {
    if (uci == "0000") {
        return Move{};
    }
    if (uci.size() != 4 && uci.size() != 5) {
        return std::nullopt;
    }
    auto from_sq = parse_square(uci[0], uci[1]);
    auto to_sq = parse_square(uci[2], uci[3]);
    if (!from_sq.has_value() || !to_sq.has_value()) {
        return std::nullopt;
    }
    if (uci.size() == 4) {
        return Move{static_cast<std::uint8_t>(*from_sq),
                    static_cast<std::uint8_t>(*to_sq),
                    FlagNone,
                    PromoKnight};
    }
    const char promo = static_cast<char>(std::tolower(static_cast<unsigned char>(uci[4])));
    Promo promo_val;
    switch (promo) {
        case 'n':
            promo_val = PromoKnight;
            break;
        case 'b':
            promo_val = PromoBishop;
            break;
        case 'r':
            promo_val = PromoRook;
            break;
        case 'q':
            promo_val = PromoQueen;
            break;
        default:
            return std::nullopt;
    }
    return Move{static_cast<std::uint8_t>(*from_sq),
                static_cast<std::uint8_t>(*to_sq),
                FlagPromotion,
                promo_val};
}

}  // namespace chess_engine
