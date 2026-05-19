#include <chrono>
#include <memory>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include "chess_engine/board.hpp"
#include "chess_engine/engine.hpp"
#include "chess_engine/move.hpp"
#include "chess_engine/position.hpp"

namespace {

class PlaceholderEngine : public chess_engine::Engine {
public:
    void set_position(std::string_view) override {}
    chess_engine::Move best_move(std::chrono::milliseconds) override { return {}; }
    std::string name() const override { return "placeholder"; }
};

}  // namespace

TEST(EngineSkeleton, ConstructsBoardAndPosition) {
    chess_engine::Board board;
    chess_engine::Position position;
    EXPECT_EQ(board.occupancy(), 0u);
    EXPECT_TRUE(position.white_to_move());
}

TEST(EngineSkeleton, MoveDefaultsToZero) {
    chess_engine::Move move;
    EXPECT_EQ(move.raw(), 0u);
    EXPECT_EQ(move.from(), 0u);
    EXPECT_EQ(move.to(), 0u);
}

TEST(EngineSkeleton, EngineInterfaceIsAbstractAndImplementable) {
    std::unique_ptr<chess_engine::Engine> engine = std::make_unique<PlaceholderEngine>();
    EXPECT_EQ(engine->name(), "placeholder");
    engine->set_position("startpos");
    auto move = engine->best_move(std::chrono::milliseconds{1});
    EXPECT_EQ(move.raw(), 0u);
}
