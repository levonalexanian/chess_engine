#include "chess_server/game_session.hpp"

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include <crow/json.h>

#include "chess_engine/engine.hpp"
#include "chess_engine/move.hpp"
#include "chess_engine/position.hpp"

namespace chess_server {

namespace {

std::string format_move(chess_engine::Move const& move) {
    if (move.raw() == 0) {
        return "0000";
    }
    auto square_to_string = [](std::uint8_t sq) {
        std::string out;
        out.push_back(static_cast<char>('a' + (sq & 7)));
        out.push_back(static_cast<char>('1' + ((sq >> 3) & 7)));
        return out;
    };
    std::string uci_str = square_to_string(move.from()) + square_to_string(move.to());
    std::uint8_t const promo = move.promotion();
    if (promo != 0) {
        constexpr const char* kPromos = "nbrq";
        if (promo < 4) {
            uci_str.push_back(kPromos[promo]);
        }
    }
    return uci_str;
}

}  // namespace

OutboundMessage make_state_message(std::string fen) {
    OutboundMessage msg;
    msg.type = "state";
    msg.fen = std::move(fen);
    return msg;
}

OutboundMessage make_engine_move_message(std::string uci) {
    OutboundMessage msg;
    msg.type = "engine_move";
    msg.uci = std::move(uci);
    return msg;
}

OutboundMessage make_error_message(std::string message) {
    OutboundMessage msg;
    msg.type = "error";
    msg.message = std::move(message);
    return msg;
}

std::string serialize_outbound(OutboundMessage const& msg) {
    crow::json::wvalue payload;
    payload["type"] = msg.type;
    if (msg.type == "state") {
        payload["fen"] = msg.fen;
    } else if (msg.type == "engine_move") {
        payload["uci"] = msg.uci;
    } else if (msg.type == "error") {
        payload["message"] = msg.message;
    }
    return payload.dump();
}

GameSession::GameSession(EngineRegistry const& registry) : registry_(registry) {}

std::vector<OutboundMessage> GameSession::on_new_game(std::string_view engine_name) {
    auto engine = registry_.create(engine_name);
    if (engine == nullptr) {
        return {make_error_message("unknown engine: " + std::string(engine_name))};
    }

    engine_ = std::move(engine);
    position_ = chess_engine::Position{};
    last_user_move_.reset();
    state_ = SessionState::InGame;
    engine_->set_position(position_.fen());

    return {make_state_message(position_.fen())};
}

std::vector<OutboundMessage> GameSession::on_user_move(std::string_view uci) {
    if (state_ != SessionState::InGame) {
        return {make_error_message("no active game")};
    }
    if (uci.empty()) {
        return {make_error_message("missing uci")};
    }

    last_user_move_ = std::string(uci);
    return {make_state_message(position_.fen())};
}

std::vector<OutboundMessage> GameSession::on_request_engine_move() {
    if (state_ != SessionState::InGame) {
        return {make_error_message("no active game")};
    }
    if (engine_ == nullptr) {
        return {make_error_message("no engine selected")};
    }

    auto const move = engine_->best_move(std::chrono::milliseconds{100});
    auto const uci = format_move(move);

    std::vector<OutboundMessage> out;
    out.push_back(make_engine_move_message(uci));
    out.push_back(make_state_message(position_.fen()));
    return out;
}

SessionState GameSession::state() const {
    return state_;
}

std::string GameSession::current_fen() const {
    return position_.fen();
}

std::optional<std::string> GameSession::last_user_move() const {
    return last_user_move_;
}

chess_engine::Engine const* GameSession::engine() const {
    return engine_.get();
}

}  // namespace chess_server
