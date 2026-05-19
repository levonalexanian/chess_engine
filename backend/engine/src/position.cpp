#include "chess_engine/position.hpp"

#include <array>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace chess_engine {

namespace {

constexpr std::string_view kStartingFen =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

std::optional<Piece> piece_from_fen_char(char c) {
    switch (c) {
        case 'P': return Piece::WhitePawn;
        case 'N': return Piece::WhiteKnight;
        case 'B': return Piece::WhiteBishop;
        case 'R': return Piece::WhiteRook;
        case 'Q': return Piece::WhiteQueen;
        case 'K': return Piece::WhiteKing;
        case 'p': return Piece::BlackPawn;
        case 'n': return Piece::BlackKnight;
        case 'b': return Piece::BlackBishop;
        case 'r': return Piece::BlackRook;
        case 'q': return Piece::BlackQueen;
        case 'k': return Piece::BlackKing;
        default: return std::nullopt;
    }
}

char fen_char_for(Piece p) {
    static constexpr char kChars[] = "PNBRQKpnbrqk";
    return kChars[p];
}

std::vector<std::string_view> split_fields(std::string_view fen) {
    std::vector<std::string_view> fields;
    std::size_t i = 0;
    while (i < fen.size()) {
        while (i < fen.size() && (fen[i] == ' ' || fen[i] == '\t')) {
            ++i;
        }
        const std::size_t start = i;
        while (i < fen.size() && fen[i] != ' ' && fen[i] != '\t') {
            ++i;
        }
        if (start < i) {
            fields.emplace_back(fen.substr(start, i - start));
        }
    }
    return fields;
}

std::optional<int> parse_int_field(std::string_view field) {
    int value = 0;
    const auto* first = field.data();
    const auto* last = field.data() + field.size();
    auto res = std::from_chars(first, last, value);
    if (res.ec != std::errc{} || res.ptr != last) {
        return std::nullopt;
    }
    return value;
}

std::optional<int> parse_en_passant(std::string_view field) {
    if (field == "-") {
        return std::optional<int>{std::nullopt};
    }
    if (field.size() != 2) {
        return std::nullopt;
    }
    const char f = field[0];
    const char r = field[1];
    if (f < 'a' || f > 'h' || r < '1' || r > '8') {
        return std::nullopt;
    }
    const int square = (r - '1') * 8 + (f - 'a');
    return square;
}

std::optional<CastlingRights> parse_castling(std::string_view field) {
    CastlingRights c;
    if (field == "-") {
        return c;
    }
    for (char ch : field) {
        switch (ch) {
            case 'K': c.set(CastlingRights::WhiteKingSide); break;
            case 'Q': c.set(CastlingRights::WhiteQueenSide); break;
            case 'k': c.set(CastlingRights::BlackKingSide); break;
            case 'q': c.set(CastlingRights::BlackQueenSide); break;
            default: return std::nullopt;
        }
    }
    return c;
}

}  // namespace

Position::Position() {
    auto starting = Position::from_fen(kStartingFen);
    if (starting.has_value()) {
        *this = *starting;
    }
}

Position::Position(std::string_view fen) {
    auto parsed = Position::from_fen(fen);
    if (parsed.has_value()) {
        *this = *parsed;
    } else {
        auto starting = Position::from_fen(kStartingFen);
        if (starting.has_value()) {
            *this = *starting;
        }
    }
}

std::optional<Position> Position::from_fen(std::string_view fen) {
    auto fields = split_fields(fen);
    if (fields.size() < 4 || fields.size() > 6) {
        return std::nullopt;
    }
    Position pos{Position::EmptyTag{}};

    const auto& placement = fields[0];
    int rank = 7;
    int file = 0;
    for (char c : placement) {
        if (c == '/') {
            if (file != 8) {
                return std::nullopt;
            }
            --rank;
            file = 0;
            if (rank < 0) {
                return std::nullopt;
            }
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            const int empty = c - '0';
            if (empty < 1 || empty > 8) {
                return std::nullopt;
            }
            file += empty;
            if (file > 8) {
                return std::nullopt;
            }
        } else {
            auto piece = piece_from_fen_char(c);
            if (!piece.has_value()) {
                return std::nullopt;
            }
            if (file >= 8) {
                return std::nullopt;
            }
            const int square = rank * 8 + file;
            pos.board_.set_piece(square, *piece);
            ++file;
        }
    }
    if (rank != 0 || file != 8) {
        return std::nullopt;
    }

    if (fields[1] == "w") {
        pos.side_to_move_ = Color::White;
    } else if (fields[1] == "b") {
        pos.side_to_move_ = Color::Black;
    } else {
        return std::nullopt;
    }

    auto castling = parse_castling(fields[2]);
    if (!castling.has_value()) {
        return std::nullopt;
    }
    pos.castling_ = *castling;

    auto ep = parse_en_passant(fields[3]);
    if (!ep.has_value() && fields[3] != "-") {
        return std::nullopt;
    }
    if (fields[3] == "-") {
        pos.en_passant_square_.reset();
    } else {
        pos.en_passant_square_ = ep;
    }

    if (fields.size() >= 5) {
        auto halfmove = parse_int_field(fields[4]);
        if (!halfmove.has_value() || *halfmove < 0) {
            return std::nullopt;
        }
        pos.halfmove_clock_ = *halfmove;
    } else {
        pos.halfmove_clock_ = 0;
    }

    if (fields.size() == 6) {
        auto fullmove = parse_int_field(fields[5]);
        if (!fullmove.has_value() || *fullmove < 1) {
            return std::nullopt;
        }
        pos.fullmove_number_ = *fullmove;
    } else {
        pos.fullmove_number_ = 1;
    }

    return pos;
}

std::string Position::to_fen() const {
    std::ostringstream out;
    for (int rank = 7; rank >= 0; --rank) {
        int empty_run = 0;
        for (int file = 0; file < 8; ++file) {
            const int square = rank * 8 + file;
            auto piece = board_.piece_at(square);
            if (!piece.has_value()) {
                ++empty_run;
            } else {
                if (empty_run > 0) {
                    out << empty_run;
                    empty_run = 0;
                }
                out << fen_char_for(*piece);
            }
        }
        if (empty_run > 0) {
            out << empty_run;
        }
        if (rank > 0) {
            out << '/';
        }
    }

    out << ' ' << (side_to_move_ == Color::White ? 'w' : 'b') << ' ';

    if (castling_.mask == 0) {
        out << '-';
    } else {
        if (castling_.has(CastlingRights::WhiteKingSide)) out << 'K';
        if (castling_.has(CastlingRights::WhiteQueenSide)) out << 'Q';
        if (castling_.has(CastlingRights::BlackKingSide)) out << 'k';
        if (castling_.has(CastlingRights::BlackQueenSide)) out << 'q';
    }
    out << ' ';

    if (en_passant_square_.has_value()) {
        const int sq = *en_passant_square_;
        out << static_cast<char>('a' + (sq & 7));
        out << static_cast<char>('1' + (sq >> 3));
    } else {
        out << '-';
    }

    out << ' ' << halfmove_clock_ << ' ' << fullmove_number_;
    return out.str();
}

std::string Position::fen() const {
    return to_fen();
}

bool operator==(const Position& a, const Position& b) {
    if (a.side_to_move_ != b.side_to_move_) return false;
    if (a.castling_ != b.castling_) return false;
    if (a.en_passant_square_ != b.en_passant_square_) return false;
    if (a.halfmove_clock_ != b.halfmove_clock_) return false;
    if (a.fullmove_number_ != b.fullmove_number_) return false;
    for (int i = 0; i < kNumPieces; ++i) {
        if (a.board_.pieces(static_cast<Piece>(i)) != b.board_.pieces(static_cast<Piece>(i))) {
            return false;
        }
    }
    return true;
}

}  // namespace chess_engine
