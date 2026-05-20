#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "chess_engine/engines/random_mover.h"
#include "chess_engine/move.h"
#include "chess_engine/position.h"

namespace {

constexpr char const* kStartingFen =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

constexpr std::chrono::milliseconds kBudget{10};

}  // namespace

TEST(RandomMoverEngine, NameIsRandom) {
    chess_engine::RandomMoverEngine engine(1);
    EXPECT_EQ(engine.name(), "random");
}

TEST(RandomMoverEngine, BestMoveFromStartposIsLegalOpeningMove) {
    chess_engine::RandomMoverEngine engine(42);
    engine.set_position(kStartingFen);

    auto const move = engine.best_move(kBudget);
    ASSERT_NE(move.raw(), 0u);

    chess_engine::Position pos(kStartingFen);
    auto const legal = pos.generate_legal_moves();
    ASSERT_EQ(legal.size(), 20u);

    bool found = false;
    for (auto const& m : legal) {
        if (m == move) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "engine returned move " << move.to_uci()
                       << " which is not in the legal move list";
}

TEST(RandomMoverEngine, SameSeedSamePositionProducesSameMove) {
    chess_engine::RandomMoverEngine engine_a(12345);
    chess_engine::RandomMoverEngine engine_b(12345);
    engine_a.set_position(kStartingFen);
    engine_b.set_position(kStartingFen);

    auto const move_a = engine_a.best_move(kBudget);
    auto const move_b = engine_b.best_move(kBudget);

    EXPECT_EQ(move_a.raw(), move_b.raw());
}

TEST(RandomMoverEngine, DifferentSeedsEventuallyProduceDifferentMoves) {
    std::set<std::uint16_t> distinct;
    for (std::uint64_t seed = 1; seed <= 200; ++seed) {
        chess_engine::RandomMoverEngine engine(seed);
        engine.set_position(kStartingFen);
        distinct.insert(engine.best_move(kBudget).raw());
    }
    EXPECT_GT(distinct.size(), 1u);
}

TEST(RandomMoverEngine, DistributionRoughlyUniformAcrossSeeds) {
    chess_engine::Position pos(kStartingFen);
    auto const legal = pos.generate_legal_moves();
    ASSERT_EQ(legal.size(), 20u);

    std::array<int, 20> bucket{};
    constexpr int kTrials = 4000;
    for (std::uint64_t seed = 1; seed <= kTrials; ++seed) {
        chess_engine::RandomMoverEngine engine(seed);
        engine.set_position(kStartingFen);
        auto const move = engine.best_move(kBudget);
        for (std::size_t i = 0; i < legal.size(); ++i) {
            if (legal[i] == move) {
                bucket[i] += 1;
                break;
            }
        }
    }

    constexpr int kExpected = kTrials / 20;
    for (auto count : bucket) {
        EXPECT_GT(count, kExpected / 2)
            << "bucket count " << count << " too far below expected " << kExpected;
        EXPECT_LT(count, kExpected * 2)
            << "bucket count " << count << " too far above expected " << kExpected;
    }
}

TEST(RandomMoverEngine, ReturnsDefaultMoveOnStalemate) {
    chess_engine::RandomMoverEngine engine(7);
    engine.set_position("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");

    auto const move = engine.best_move(kBudget);
    EXPECT_EQ(move.raw(), 0u);
    EXPECT_EQ(move.to_uci(), "0000");
}

TEST(RandomMoverEngine, ReturnsDefaultMoveOnCheckmate) {
    chess_engine::RandomMoverEngine engine(11);
    engine.set_position("6k1/5ppp/8/8/8/8/5PPP/4r1K1 w - - 0 1");

    auto const move = engine.best_move(kBudget);
    EXPECT_EQ(move.raw(), 0u);
    EXPECT_EQ(move.to_uci(), "0000");
}

TEST(RandomMoverEngine, ReturnsSingleLegalMoveWhenOnlyOneExists) {
    constexpr char const* kFen = "7k/8/8/8/8/7q/8/7K w - - 0 1";
    chess_engine::RandomMoverEngine engine(99);
    engine.set_position(kFen);

    chess_engine::Position pos(kFen);
    auto const legal = pos.generate_legal_moves();
    ASSERT_EQ(legal.size(), 1u);

    auto const move = engine.best_move(kBudget);
    EXPECT_EQ(move, legal[0]);
}

TEST(RandomMoverEngine, MalformedFenKeepsPreviousPosition) {
    chess_engine::RandomMoverEngine engine(3);
    engine.set_position(kStartingFen);
    engine.set_position("not a valid fen");

    auto const move = engine.best_move(kBudget);
    ASSERT_NE(move.raw(), 0u);

    chess_engine::Position pos(kStartingFen);
    auto const legal = pos.generate_legal_moves();
    bool found = false;
    for (auto const& m : legal) {
        if (m == move) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}
