#include "chess_server/game_session.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include <crow/json.h>

#include "chess_engine/engine.h"
#include "chess_engine/move.h"
#include "chess_engine/position.h"

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
    return on_new_game(engine_name, std::nullopt, {});
}

std::vector<OutboundMessage> GameSession::on_new_game(
    std::string_view engine_name, std::optional<std::string> starting_fen,
    std::vector<std::string> const& moves) {
    auto engine = registry_.create(engine_name);
    if (engine == nullptr) {
        return {make_error_message("unknown engine: " + std::string(engine_name))};
    }

    chess_engine::Position new_position;
    if (starting_fen.has_value()) {
        auto parsed = chess_engine::Position::from_fen(*starting_fen);
        if (!parsed.has_value()) {
            return {make_error_message("malformed starting_fen: " + *starting_fen)};
        }
        new_position = std::move(*parsed);
    }

    for (auto const& move_uci : moves) {
        auto const parsed_move = chess_engine::Move::from_uci(move_uci);
        if (!parsed_move.has_value()) {
            return {make_error_message("malformed move: " + move_uci)};
        }
        auto const legal = new_position.generate_legal_moves();
        bool applied = false;
        for (auto const& candidate : legal) {
            if (candidate.from() == parsed_move->from() &&
                candidate.to() == parsed_move->to() &&
                candidate.is_promotion() == parsed_move->is_promotion() &&
                (!parsed_move->is_promotion() ||
                 candidate.promotion() == parsed_move->promotion())) {
                new_position.make_move(candidate);
                applied = true;
                break;
            }
        }
        if (!applied) {
            return {make_error_message("illegal move in history: " + move_uci)};
        }
    }

    engine_ = std::move(engine);
    position_ = std::move(new_position);
    last_user_move_ = moves.empty() ? std::optional<std::string>{}
                                    : std::optional<std::string>{moves.back()};
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

    auto const parsed = chess_engine::Move::from_uci(uci);
    if (!parsed.has_value()) {
        return {make_error_message("malformed uci: " + std::string(uci))};
    }

    auto const legal = position_.generate_legal_moves();
    auto match = legal.end();
    for (auto it = legal.begin(); it != legal.end(); ++it) {
        if (it->from() == parsed->from() && it->to() == parsed->to() &&
            (!parsed->is_promotion() || it->promotion() == parsed->promotion())) {
            if (parsed->is_promotion() && !it->is_promotion()) {
                continue;
            }
            if (!parsed->is_promotion() && it->is_promotion()) {
                continue;
            }
            match = it;
            break;
        }
    }
    if (match == legal.end()) {
        return {make_error_message("illegal move: " + std::string(uci))};
    }

    position_.make_move(*match);
    last_user_move_ = std::string(uci);
    engine_->set_position(position_.fen());

    return {make_state_message(position_.fen())};
}

std::vector<OutboundMessage> GameSession::on_request_engine_move() {
    if (state_ != SessionState::InGame) {
        return {make_error_message("no active game")};
    }
    if (engine_ == nullptr) {
        return {make_error_message("no engine selected")};
    }

    engine_->set_position(position_.fen());
    auto const move = engine_->best_move(std::chrono::milliseconds{100});
    auto const uci = format_move(move);

    if (move.raw() != 0) {
        position_.make_move(move);
    }

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
