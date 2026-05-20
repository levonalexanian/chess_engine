#include "chess_engine/uci.h"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <iomanip>
#include <istream>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "chess_engine/board.h"
#include "chess_engine/engine.h"
#include "chess_engine/move.h"
#include "chess_engine/position.h"

namespace chess_engine {

namespace {

constexpr std::string_view kStartingFen =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

constexpr std::string_view kEngineAuthor = "chess_engine authors";

std::vector<std::string_view> tokenize(std::string_view line) {
    std::vector<std::string_view> tokens;
    std::size_t i = 0;
    while (i < line.size()) {
        while (i < line.size() && (line[i] == ' ' || line[i] == '\t' || line[i] == '\r' || line[i] == '\n')) {
            ++i;
        }
        const std::size_t start = i;
        while (i < line.size() && line[i] != ' ' && line[i] != '\t' && line[i] != '\r' && line[i] != '\n') {
            ++i;
        }
        if (start < i) {
            tokens.emplace_back(line.substr(start, i - start));
        }
    }
    return tokens;
}

std::optional<int> parse_int(std::string_view token) {
    int value = 0;
    const auto* first = token.data();
    const auto* last = token.data() + token.size();
    auto result = std::from_chars(first, last, value);
    if (result.ec != std::errc{} || result.ptr != last) {
        return std::nullopt;
    }
    return value;
}

std::optional<UciCommand> parse_position(const std::vector<std::string_view>& tokens) {
    if (tokens.size() < 2) {
        return std::nullopt;
    }
    UciCmdPosition cmd;
    std::size_t cursor = 1;
    if (tokens[cursor] == "startpos") {
        cmd.fen = std::string(kStartingFen);
        ++cursor;
    } else if (tokens[cursor] == "fen") {
        ++cursor;
        std::string fen;
        while (cursor < tokens.size() && tokens[cursor] != "moves") {
            if (!fen.empty()) {
                fen.push_back(' ');
            }
            fen.append(tokens[cursor].begin(), tokens[cursor].end());
            ++cursor;
        }
        if (fen.empty()) {
            return std::nullopt;
        }
        cmd.fen = std::move(fen);
    } else {
        return std::nullopt;
    }

    if (cursor < tokens.size() && tokens[cursor] == "moves") {
        ++cursor;
        while (cursor < tokens.size()) {
            cmd.moves.emplace_back(tokens[cursor]);
            ++cursor;
        }
    }
    return cmd;
}

std::optional<UciCommand> parse_go(const std::vector<std::string_view>& tokens) {
    UciCmdGo cmd;
    std::size_t cursor = 1;
    while (cursor < tokens.size()) {
        const auto& key = tokens[cursor];
        if (key == "infinite") {
            cmd.infinite = true;
            ++cursor;
            continue;
        }

        if (cursor + 1 >= tokens.size()) {
            break;
        }
        const auto value = parse_int(tokens[cursor + 1]);
        if (!value.has_value()) {
            cursor += 2;
            continue;
        }

        if (key == "wtime") {
            cmd.wtime = value;
        } else if (key == "btime") {
            cmd.btime = value;
        } else if (key == "winc") {
            cmd.winc = value;
        } else if (key == "binc") {
            cmd.binc = value;
        } else if (key == "movetime") {
            cmd.movetime = value;
        } else if (key == "depth") {
            cmd.depth = value;
        } else if (key == "nodes") {
            cmd.nodes = value;
        }
        cursor += 2;
    }
    return cmd;
}

std::string format_move(const Move& move) {
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
    const std::uint8_t promo = move.promotion();
    if (promo != 0) {
        constexpr const char* kPromos = "nbrq";
        if (promo < 4) {
            uci_str.push_back(kPromos[promo]);
        }
    }
    return uci_str;
}

std::chrono::milliseconds time_budget_for(const UciCmdGo& go) {
    if (go.movetime.has_value()) {
        return std::chrono::milliseconds{*go.movetime};
    }
    return std::chrono::milliseconds{1000};
}

char render_piece(Piece p) {
    static constexpr char kChars[] = "PNBRQKpnbrqk";
    return kChars[p];
}

std::string render_position(const Position& pos) {
    std::ostringstream out;
    for (int rank = 7; rank >= 0; --rank) {
        out << (rank + 1) << ' ';
        for (int file = 0; file < 8; ++file) {
            const int sq = rank * 8 + file;
            auto piece = pos.board().piece_at(sq);
            out << (piece.has_value() ? render_piece(*piece) : '.');
            if (file < 7) {
                out << ' ';
            }
        }
        out << '\n';
    }
    out << "  a b c d e f g h\n";

    out << "Side to move: " << (pos.white_to_move() ? "white" : "black") << '\n';
    out << "Castling: ";
    const auto castling = pos.castling();
    if (castling.mask == 0) {
        out << '-';
    } else {
        if (castling.has(CastlingRights::WhiteKingSide)) out << 'K';
        if (castling.has(CastlingRights::WhiteQueenSide)) out << 'Q';
        if (castling.has(CastlingRights::BlackKingSide)) out << 'k';
        if (castling.has(CastlingRights::BlackQueenSide)) out << 'q';
    }
    out << '\n';
    out << "En passant: ";
    if (pos.en_passant_square().has_value()) {
        const int sq = *pos.en_passant_square();
        out << static_cast<char>('a' + (sq & 7));
        out << static_cast<char>('1' + (sq >> 3));
    } else {
        out << '-';
    }
    out << '\n';
    out << "Halfmove clock: " << pos.halfmove_clock() << '\n';
    out << "Fullmove number: " << pos.fullmove_number() << '\n';
    out << "Zobrist hash: 0x" << std::hex << std::setw(16) << std::setfill('0')
        << pos.zobrist_hash() << std::dec << '\n';
    return out.str();
}

}  // namespace

std::string starting_fen() {
    return std::string(kStartingFen);
}

std::optional<UciCommand> parse_command(std::string_view line) {
    const auto tokens = tokenize(line);
    if (tokens.empty()) {
        return std::nullopt;
    }
    const auto& head = tokens.front();
    if (head == "uci") {
        return UciCommand{UciCmdUci{}};
    }
    if (head == "isready") {
        return UciCommand{UciCmdIsReady{}};
    }
    if (head == "ucinewgame") {
        return UciCommand{UciCmdUciNewGame{}};
    }
    if (head == "quit") {
        return UciCommand{UciCmdQuit{}};
    }
    if (head == "position") {
        return parse_position(tokens);
    }
    if (head == "go") {
        return parse_go(tokens);
    }
    if (head == "d" || head == "display") {
        return UciCommand{UciCmdDisplay{}};
    }
    return std::nullopt;
}

void UciLoop::run(Engine& engine, std::istream& in, std::ostream& out) {
    std::string line;
    Position current_position;
    while (std::getline(in, line)) {
        const auto parsed = parse_command(line);
        if (!parsed.has_value()) {
            continue;
        }
        const auto& cmd = *parsed;
        if (std::holds_alternative<UciCmdUci>(cmd)) {
            out << "id name chess_engine " << engine.name() << "\n";
            out << "id author " << kEngineAuthor << "\n";
            out << "uciok\n";
            out.flush();
        } else if (std::holds_alternative<UciCmdIsReady>(cmd)) {
            out << "readyok\n";
            out.flush();
        } else if (std::holds_alternative<UciCmdUciNewGame>(cmd)) {
            engine.set_position(std::string(kStartingFen));
            current_position = Position{std::string_view(kStartingFen)};
        } else if (const auto* pos = std::get_if<UciCmdPosition>(&cmd)) {
            auto parsed_pos = Position::from_fen(pos->fen);
            if (parsed_pos.has_value()) {
                current_position = *parsed_pos;
                for (const auto& uci : pos->moves) {
                    current_position.make_move(uci);
                }
            }
            engine.set_position(current_position.fen());
        } else if (const auto* go = std::get_if<UciCmdGo>(&cmd)) {
            const auto move = engine.best_move(time_budget_for(*go));
            out << "bestmove " << format_move(move) << "\n";
            out.flush();
        } else if (std::holds_alternative<UciCmdDisplay>(cmd)) {
            out << render_position(current_position);
            out.flush();
        } else if (std::holds_alternative<UciCmdQuit>(cmd)) {
            return;
        }
    }
}

}  // namespace chess_engine
