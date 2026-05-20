#include <cstdint>
#include <string>

#include <gtest/gtest.h>

#include "chess_engine/move.h"

namespace ce = chess_engine;

TEST(Move, DefaultConstructedIsNull) {
    ce::Move m;
    EXPECT_EQ(m.raw(), 0u);
    EXPECT_EQ(m.from(), 0u);
    EXPECT_EQ(m.to(), 0u);
    EXPECT_EQ(m.flags(), 0u);
    EXPECT_EQ(m.promotion(), 0u);
    EXPECT_EQ(m.to_uci(), "0000");
}

TEST(Move, FromUciAcceptsNullMove) {
    auto m = ce::Move::from_uci("0000");
    ASSERT_TRUE(m.has_value());
    EXPECT_EQ(m->raw(), 0u);
    EXPECT_EQ(m->to_uci(), "0000");
}

TEST(Move, FromUciSimpleMoveRoundTrip) {
    auto m = ce::Move::from_uci("e2e4");
    ASSERT_TRUE(m.has_value());
    EXPECT_EQ(m->from(), 12u);  // e2 = file 4, rank 1 -> 1*8+4 = 12
    EXPECT_EQ(m->to(), 28u);    // e4 = file 4, rank 3 -> 3*8+4 = 28
    EXPECT_EQ(m->flags(), ce::Move::FlagNone);
    EXPECT_EQ(m->to_uci(), "e2e4");
}

TEST(Move, FromUciCornerMoves) {
    auto m1 = ce::Move::from_uci("a1h8");
    ASSERT_TRUE(m1.has_value());
    EXPECT_EQ(m1->from(), 0u);
    EXPECT_EQ(m1->to(), 63u);
    EXPECT_EQ(m1->to_uci(), "a1h8");

    auto m2 = ce::Move::from_uci("h1a8");
    ASSERT_TRUE(m2.has_value());
    EXPECT_EQ(m2->from(), 7u);
    EXPECT_EQ(m2->to(), 56u);
    EXPECT_EQ(m2->to_uci(), "h1a8");
}

TEST(Move, PromotionRoundTripsAllFourPieces) {
    for (const char* uci : {"a7a8q", "a7a8r", "a7a8b", "a7a8n"}) {
        auto m = ce::Move::from_uci(uci);
        ASSERT_TRUE(m.has_value()) << uci;
        EXPECT_TRUE(m->is_promotion()) << uci;
        EXPECT_EQ(m->to_uci(), uci);
    }
}

TEST(Move, BlackPromotionRoundTrips) {
    auto m = ce::Move::from_uci("h2h1q");
    ASSERT_TRUE(m.has_value());
    EXPECT_TRUE(m->is_promotion());
    EXPECT_EQ(m->promotion(), ce::Move::PromoQueen);
    EXPECT_EQ(m->to_uci(), "h2h1q");
}

TEST(Move, FromUciRejectsMalformed) {
    EXPECT_FALSE(ce::Move::from_uci("").has_value());
    EXPECT_FALSE(ce::Move::from_uci("e2").has_value());
    EXPECT_FALSE(ce::Move::from_uci("e2e4e5").has_value());
    EXPECT_FALSE(ce::Move::from_uci("e9e4").has_value());
    EXPECT_FALSE(ce::Move::from_uci("z2z4").has_value());
    EXPECT_FALSE(ce::Move::from_uci("e2e4k").has_value());
}

TEST(Move, FromUciAcceptsUppercasePromotion) {
    auto m = ce::Move::from_uci("a7a8Q");
    ASSERT_TRUE(m.has_value());
    EXPECT_EQ(m->promotion(), ce::Move::PromoQueen);
}

TEST(Move, ConstructorPackingMatchesAccessors) {
    ce::Move m(12, 28, ce::Move::FlagNone);
    EXPECT_EQ(m.from(), 12u);
    EXPECT_EQ(m.to(), 28u);
    EXPECT_EQ(m.flags(), ce::Move::FlagNone);

    ce::Move castle(4, 6, ce::Move::FlagCastling);
    EXPECT_EQ(castle.flags(), ce::Move::FlagCastling);
    EXPECT_TRUE(castle.is_castling());

    ce::Move ep(35, 42, ce::Move::FlagEnPassant);
    EXPECT_TRUE(ep.is_en_passant());

    ce::Move promo(48, 56, ce::Move::FlagPromotion, ce::Move::PromoRook);
    EXPECT_TRUE(promo.is_promotion());
    EXPECT_EQ(promo.promotion(), ce::Move::PromoRook);
}

TEST(Move, EveryLegalUciSquarePairRoundTrips) {
    for (int from = 0; from < 64; ++from) {
        for (int to = 0; to < 64; ++to) {
            if (from == to) continue;
            std::string uci;
            uci.push_back(static_cast<char>('a' + (from & 7)));
            uci.push_back(static_cast<char>('1' + (from >> 3)));
            uci.push_back(static_cast<char>('a' + (to & 7)));
            uci.push_back(static_cast<char>('1' + (to >> 3)));
            auto m = ce::Move::from_uci(uci);
            ASSERT_TRUE(m.has_value()) << uci;
            EXPECT_EQ(m->to_uci(), uci) << uci;
        }
    }
}
