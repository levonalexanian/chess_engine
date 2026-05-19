#include <chrono>
#include <memory>
#include <string>
#include <string_view>

#include <fmt/core.h>

#include "chess_engine/engine.hpp"
#include "chess_engine/move.hpp"

namespace {

class PlaceholderEngine : public chess_engine::Engine {
public:
    void set_position(std::string_view) override {}

    chess_engine::Move best_move(std::chrono::milliseconds) override {
        return {};
    }

    std::string name() const override {
        return "placeholder";
    }
};

}  // namespace

int main() {
    std::unique_ptr<chess_engine::Engine> engine = std::make_unique<PlaceholderEngine>();
    fmt::print("{}\n", engine->name());
    return 0;
}
