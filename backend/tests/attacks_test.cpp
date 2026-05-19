#include <bit>
#include <cstdint>

#include <gtest/gtest.h>

#include "chess_engine/attacks.hpp"
#include "chess_engine/board.hpp"

namespace ce = chess_engine;
namespace ca = chess_engine::attacks;

namespace {

int square_of(const char* algebraic) {
    return (algebraic[1] - '1') * 8 + (algebraic[0] - 'a');
}

ce::Bitboard bits(std::initializer_list<const char*> squares) {
    ce::Bitboard b = 0;
    for (auto s : squares) {
        b |= ce::Bitboard{1} << square_of(s);
    }
    return b;
}

}  // namespace

TEST(KnightAttacks, CenterSquareHasEightTargets) {
    const auto attacks = ca::knight_attacks(square_of("d4"));
    EXPECT_EQ(std::popcount(attacks), 8);
    const auto expected = bits({"b3", "b5", "c2", "c6", "e2", "e6", "f3", "f5"});
    EXPECT_EQ(attacks, expected);
}

TEST(KnightAttacks, CornerSquareHasTwoTargets) {
    const auto a1 = ca::knight_attacks(square_of("a1"));
    EXPECT_EQ(std::popcount(a1), 2);
    EXPECT_EQ(a1, bits({"b3", "c2"}));

    const auto h8 = ca::knight_attacks(square_of("h8"));
    EXPECT_EQ(std::popcount(h8), 2);
    EXPECT_EQ(h8, bits({"f7", "g6"}));
}

TEST(KnightAttacks, EdgeSquaresClipCorrectly) {
    // a-file square — only files a/b/c reachable
    const auto a4 = ca::knight_attacks(square_of("a4"));
    EXPECT_EQ(std::popcount(a4), 4);
    EXPECT_EQ(a4, bits({"b2", "b6", "c3", "c5"}));

    // h-file square
    const auto h4 = ca::knight_attacks(square_of("h4"));
    EXPECT_EQ(std::popcount(h4), 4);
    EXPECT_EQ(h4, bits({"f3", "f5", "g2", "g6"}));
}

TEST(KingAttacks, CenterSquareHasEightTargets) {
    const auto attacks = ca::king_attacks(square_of("d4"));
    EXPECT_EQ(std::popcount(attacks), 8);
    const auto expected =
        bits({"c3", "c4", "c5", "d3", "d5", "e3", "e4", "e5"});
    EXPECT_EQ(attacks, expected);
}

TEST(KingAttacks, CornerSquareHasThreeTargets) {
    const auto a1 = ca::king_attacks(square_of("a1"));
    EXPECT_EQ(std::popcount(a1), 3);
    EXPECT_EQ(a1, bits({"a2", "b1", "b2"}));

    const auto h8 = ca::king_attacks(square_of("h8"));
    EXPECT_EQ(std::popcount(h8), 3);
    EXPECT_EQ(h8, bits({"g7", "g8", "h7"}));
}

TEST(KingAttacks, EdgeSquaresClipCorrectly) {
    const auto a4 = ca::king_attacks(square_of("a4"));
    EXPECT_EQ(std::popcount(a4), 5);
    EXPECT_EQ(a4, bits({"a3", "a5", "b3", "b4", "b5"}));
}

TEST(PawnAttacks, WhiteCenterCovers_NW_NE) {
    const auto attacks = ca::pawn_attacks(ce::Color::White, square_of("e4"));
    EXPECT_EQ(attacks, bits({"d5", "f5"}));
}

TEST(PawnAttacks, BlackCenterCovers_SW_SE) {
    const auto attacks = ca::pawn_attacks(ce::Color::Black, square_of("e5"));
    EXPECT_EQ(attacks, bits({"d4", "f4"}));
}

TEST(PawnAttacks, WhiteAFileOnlyCoversNE) {
    EXPECT_EQ(ca::pawn_attacks(ce::Color::White, square_of("a2")), bits({"b3"}));
}

TEST(PawnAttacks, WhiteHFileOnlyCoversNW) {
    EXPECT_EQ(ca::pawn_attacks(ce::Color::White, square_of("h2")), bits({"g3"}));
}

TEST(PawnAttacks, BlackAFileOnlyCoversSE) {
    EXPECT_EQ(ca::pawn_attacks(ce::Color::Black, square_of("a7")), bits({"b6"}));
}

TEST(PawnAttacks, BlackHFileOnlyCoversSW) {
    EXPECT_EQ(ca::pawn_attacks(ce::Color::Black, square_of("h7")), bits({"g6"}));
}

TEST(PawnAttacks, EighthRankWhiteHasNone) {
    EXPECT_EQ(ca::pawn_attacks(ce::Color::White, square_of("e8")), 0u);
}

TEST(PawnAttacks, FirstRankBlackHasNone) {
    EXPECT_EQ(ca::pawn_attacks(ce::Color::Black, square_of("e1")), 0u);
}
