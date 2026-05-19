#include "chess_server/registry.hpp"

#include <chrono>
#include <string>
#include <string_view>
#include <utility>

#include "chess_engine/engine.hpp"
#include "chess_engine/move.hpp"

namespace chess_server {

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

EngineRegistry::EngineRegistry() = default;

void EngineRegistry::register_engine(std::string name, Factory factory) {
    factories_[std::move(name)] = std::move(factory);
}

std::unique_ptr<chess_engine::Engine> EngineRegistry::create(std::string_view name) const {
    auto it = factories_.find(std::string(name));
    if (it == factories_.end()) {
        return nullptr;
    }
    return it->second();
}

bool EngineRegistry::has(std::string_view name) const {
    return factories_.find(std::string(name)) != factories_.end();
}

std::vector<std::string> EngineRegistry::names() const {
    std::vector<std::string> out;
    out.reserve(factories_.size());
    for (auto const& entry : factories_) {
        out.push_back(entry.first);
    }
    return out;
}

EngineRegistry EngineRegistry::with_defaults() {
    EngineRegistry registry;
    registry.register_engine("placeholder", []() -> std::unique_ptr<chess_engine::Engine> {
        return std::make_unique<PlaceholderEngine>();
    });
    return registry;
}

}  // namespace chess_server
