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

#include "chess_engine/move.hpp"
#include "chess_engine/zobrist.hpp"

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

    pos.zobrist_hash_ = pos.compute_zobrist_hash();
    return pos;
}

namespace {

constexpr int square_index(int file, int rank) {
    return rank * 8 + file;
}

std::uint8_t castling_loss_for_square(int square) {
    // Whenever a piece moves to or from a corner, that side loses the
    // corresponding castling right (rook moved off its starting square, or
    // captured on a corner).
    switch (square) {
        case 0:  return CastlingRights::WhiteQueenSide;
        case 7:  return CastlingRights::WhiteKingSide;
        case 56: return CastlingRights::BlackQueenSide;
        case 63: return CastlingRights::BlackKingSide;
        default: return 0;
    }
}

}  // namespace

bool Position::make_move(std::string_view uci) {
    auto move = Move::from_uci(uci);
    if (!move.has_value() || move->raw() == 0) {
        return false;
    }
    const int from = move->from();
    const int to = move->to();
    auto moving = board_.piece_at(from);
    if (!moving.has_value()) {
        return false;
    }

    const auto& z = zobrist::table();
    const Color mover = color_of(*moving);
    const PieceType moving_type = piece_type_of(*moving);

    // Undo current state contributions to the hash; we'll XOR new contributions back in.
    zobrist_hash_ ^= z.castling[castling_.mask & 0xF];
    if (en_passant_square_.has_value()) {
        zobrist_hash_ ^= z.en_passant_file[*en_passant_square_ & 7];
    }
    if (side_to_move_ == Color::Black) {
        zobrist_hash_ ^= z.black_to_move;
    }

    bool is_capture = false;
    auto captured = board_.piece_at(to);
    if (captured.has_value()) {
        is_capture = true;
        zobrist_hash_ ^= z.piece_square[*captured][to];
        board_.remove_piece(to);
    }

    // Detect en-passant capture: a pawn moves diagonally onto the en-passant target.
    bool is_en_passant_capture = false;
    if (moving_type == PieceType::Pawn && en_passant_square_.has_value() &&
        to == *en_passant_square_ && !is_capture) {
        is_en_passant_capture = true;
        const int captured_square = mover == Color::White ? to - 8 : to + 8;
        auto ep_target = board_.piece_at(captured_square);
        if (ep_target.has_value()) {
            zobrist_hash_ ^= z.piece_square[*ep_target][captured_square];
            board_.remove_piece(captured_square);
            is_capture = true;
        }
    }

    // Move the piece from "from" to "to" (handling promotion later).
    zobrist_hash_ ^= z.piece_square[*moving][from];
    board_.remove_piece(from);

    Piece destination_piece = *moving;
    bool is_promotion = false;
    if (uci.size() == 5) {
        is_promotion = true;
        PieceType promo_type = PieceType::Queen;
        switch (uci[4]) {
            case 'q': case 'Q': promo_type = PieceType::Queen; break;
            case 'r': case 'R': promo_type = PieceType::Rook; break;
            case 'b': case 'B': promo_type = PieceType::Bishop; break;
            case 'n': case 'N': promo_type = PieceType::Knight; break;
            default: break;
        }
        destination_piece = make_piece(mover, promo_type);
    }
    board_.set_piece(to, destination_piece);
    zobrist_hash_ ^= z.piece_square[destination_piece][to];

    // Handle castling: the king has moved two squares horizontally on its home rank.
    bool is_castling = false;
    if (moving_type == PieceType::King) {
        const int from_file = from & 7;
        const int to_file = to & 7;
        if (from_file == 4 && (to_file == 6 || to_file == 2)) {
            is_castling = true;
            const int rank = from >> 3;
            int rook_from = 0;
            int rook_to = 0;
            if (to_file == 6) {
                rook_from = square_index(7, rank);
                rook_to = square_index(5, rank);
            } else {
                rook_from = square_index(0, rank);
                rook_to = square_index(3, rank);
            }
            auto rook = board_.piece_at(rook_from);
            if (rook.has_value()) {
                zobrist_hash_ ^= z.piece_square[*rook][rook_from];
                board_.remove_piece(rook_from);
                board_.set_piece(rook_to, *rook);
                zobrist_hash_ ^= z.piece_square[*rook][rook_to];
            }
        }
    }

    // Update castling rights: any king move removes both sides' rights for the mover;
    // any rook move from a starting corner removes that right; any move/capture onto a
    // corner removes the matching right for the side that owned it.
    if (moving_type == PieceType::King) {
        if (mover == Color::White) {
            castling_.clear(CastlingRights::WhiteKingSide | CastlingRights::WhiteQueenSide);
        } else {
            castling_.clear(CastlingRights::BlackKingSide | CastlingRights::BlackQueenSide);
        }
    }
    castling_.clear(castling_loss_for_square(from));
    castling_.clear(castling_loss_for_square(to));

    // Update en passant square: set only after a two-square pawn push.
    std::optional<int> new_ep;
    if (moving_type == PieceType::Pawn) {
        const int from_rank = from >> 3;
        const int to_rank = to >> 3;
        if (mover == Color::White && from_rank == 1 && to_rank == 3) {
            new_ep = from + 8;
        } else if (mover == Color::Black && from_rank == 6 && to_rank == 4) {
            new_ep = from - 8;
        }
    }
    en_passant_square_ = new_ep;

    // Halfmove clock: reset on pawn move or capture, otherwise increment.
    if (moving_type == PieceType::Pawn || is_capture) {
        halfmove_clock_ = 0;
    } else {
        halfmove_clock_ += 1;
    }

    // Fullmove number increments after black's move.
    if (side_to_move_ == Color::Black) {
        fullmove_number_ += 1;
    }

    // Flip side to move.
    side_to_move_ = side_to_move_ == Color::White ? Color::Black : Color::White;

    // Reapply state contributions to the hash.
    zobrist_hash_ ^= z.castling[castling_.mask & 0xF];
    if (en_passant_square_.has_value()) {
        zobrist_hash_ ^= z.en_passant_file[*en_passant_square_ & 7];
    }
    if (side_to_move_ == Color::Black) {
        zobrist_hash_ ^= z.black_to_move;
    }

    (void)is_castling;
    (void)is_en_passant_capture;
    (void)is_promotion;
    return true;
}

std::uint64_t Position::compute_zobrist_hash() const {
    const auto& z = zobrist::table();
    std::uint64_t h = 0;
    for (int sq = 0; sq < kNumSquares; ++sq) {
        auto piece = board_.piece_at(sq);
        if (piece.has_value()) {
            h ^= z.piece_square[*piece][sq];
        }
    }
    h ^= z.castling[castling_.mask & 0xF];
    if (en_passant_square_.has_value()) {
        const int file = *en_passant_square_ & 7;
        h ^= z.en_passant_file[file];
    }
    if (side_to_move_ == Color::Black) {
        h ^= z.black_to_move;
    }
    return h;
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
