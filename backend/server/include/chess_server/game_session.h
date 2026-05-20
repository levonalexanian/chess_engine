#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "chess_engine/engine.hpp"
#include "chess_engine/position.hpp"
#include "chess_server/registry.hpp"

namespace chess_server {

enum class SessionState {
    Idle,
    InGame,
    GameOver,
};

struct OutboundMessage {
    std::string type;
    std::string fen;
    std::string uci;
    std::string message;
};

OutboundMessage make_state_message(std::string fen);
OutboundMessage make_engine_move_message(std::string uci);
OutboundMessage make_error_message(std::string message);

std::string serialize_outbound(OutboundMessage const& msg);

class GameSession {
public:
    explicit GameSession(EngineRegistry const& registry);

    std::vector<OutboundMessage> on_new_game(std::string_view engine_name);
    std::vector<OutboundMessage> on_new_game(std::string_view engine_name,
                                             std::optional<std::string> starting_fen,
                                             std::vector<std::string> const& moves);
    std::vector<OutboundMessage> on_user_move(std::string_view uci);
    std::vector<OutboundMessage> on_request_engine_move();

    SessionState state() const;
    std::string current_fen() const;
    std::optional<std::string> last_user_move() const;
    chess_engine::Engine const* engine() const;

private:
    EngineRegistry const& registry_;
    chess_engine::Position position_;
    std::unique_ptr<chess_engine::Engine> engine_;
    SessionState state_{SessionState::Idle};
    std::optional<std::string> last_user_move_;
};

}  // namespace chess_server
