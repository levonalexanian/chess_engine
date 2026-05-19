#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

#include "chess_engine/engine.hpp"
#include "chess_engine/move.hpp"
#include "chess_engine/uci.hpp"

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
    chess_engine::UciLoop::run(*engine, std::cin, std::cout);
    return 0;
}
