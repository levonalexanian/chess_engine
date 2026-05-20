#ifndef CHESS_SERVER_REGISTRY_H
#define CHESS_SERVER_REGISTRY_H

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "chess_engine/engine.hpp"

namespace chess_server {

std::string_view default_engine_name();

class EngineRegistry {
public:
    using Factory = std::function<std::unique_ptr<chess_engine::Engine>()>;

    EngineRegistry();

    void register_engine(std::string name, Factory factory);

    std::unique_ptr<chess_engine::Engine> create(std::string_view name) const;

    bool has(std::string_view name) const;

    std::vector<std::string> names() const;

    static EngineRegistry with_defaults();

private:
    std::unordered_map<std::string, Factory> factories_;
};

}  // namespace chess_server

#endif  // CHESS_SERVER_REGISTRY_H
