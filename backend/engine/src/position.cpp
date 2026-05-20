#include "chess_engine/position.h"

#include <array>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "chess_engine/move.h"
#include "chess_engine/zobrist.h"

namespace chess_engine {

static constexpr std::string_view kStartingFen =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

static std::optional<Piece> piece_from_fen_char(char c) {
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

static char fen_char_for(Piece p) {
    static constexpr char kChars[] = "PNBRQKpnbrqk";
    return kChars[p];
}

static std::vector<std::string_view> split_fields(std::string_view fen) {
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

static std::optional<int> parse_int_field(std::string_view field) {
    int value = 0;
    const auto* first = field.data();
    const auto* last = field.data() + field.size();
    auto res = std::from_chars(first, last, value);
    if (res.ec != std::errc{} || res.ptr != last) {
        return std::nullopt;
    }
    return value;
}

static std::optional<int> parse_en_passant(std::string_view field) {
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

static std::optional<CastlingRights> parse_castling(std::string_view field) {
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

static constexpr int square_index(int file, int rank) {
    return rank * 8 + file;
}

static std::uint8_t castling_loss_for_square(int square) {
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

bool Position::make_move(std::string_view uci) {
    auto parsed = Move::from_uci(uci);
    if (!parsed.has_value() || parsed->raw() == 0) {
        return false;
    }
    const int from = parsed->from();
    const int to = parsed->to();
    auto moving = board_.piece_at(from);
    if (!moving.has_value()) {
        return false;
    }
    const PieceType moving_type = piece_type_of(*moving);

    // Infer flags from the board state when only the bare from/to/promotion
    // bits are available from the UCI string. `from_uci` already encodes the
    // promotion flag when a fifth character is present, so we only need to
    // detect castling and en passant here.
    Move::Flag flag = static_cast<Move::Flag>(parsed->flags());
    Move::Promo promo = static_cast<Move::Promo>(parsed->promotion());
    if (flag != Move::FlagPromotion) {
        if (moving_type == PieceType::King) {
            const int from_file = from & 7;
            const int to_file = to & 7;
            if (from_file == 4 && (to_file == 6 || to_file == 2)) {
                flag = Move::FlagCastling;
            }
        }
        if (flag == Move::FlagNone && moving_type == PieceType::Pawn &&
            en_passant_square_.has_value() && to == *en_passant_square_) {
            flag = Move::FlagEnPassant;
        }
    }

    Move move{static_cast<std::uint8_t>(from), static_cast<std::uint8_t>(to), flag, promo};
    make_move(move);
    return true;
}

UndoInfo Position::make_move(Move move) {
    UndoInfo info;
    info.move = move;
    info.prior_castling = castling_;
    info.prior_ep_square = en_passant_square_;
    info.prior_halfmove = halfmove_clock_;
    info.prior_fullmove = fullmove_number_;
    info.prior_zobrist = zobrist_hash_;
    info.prior_side = side_to_move_;

    const int from = move.from();
    const int to = move.to();
    auto moving = board_.piece_at(from);
    // Caller guarantees `from` is occupied; if not, this is a programmer error.
    const Piece mover_piece = *moving;
    const Color mover = color_of(mover_piece);
    const PieceType moving_type = piece_type_of(mover_piece);

    const auto& z = zobrist::table();

    // Undo current state contributions to the hash; new contributions XOR back in below.
    zobrist_hash_ ^= z.castling[castling_.mask & 0xF];
    if (en_passant_square_.has_value()) {
        zobrist_hash_ ^= z.en_passant_file[*en_passant_square_ & 7];
    }
    if (side_to_move_ == Color::Black) {
        zobrist_hash_ ^= z.black_to_move;
    }

    bool is_capture = false;
    if (move.is_en_passant()) {
        const int captured_square = mover == Color::White ? to - 8 : to + 8;
        auto ep_target = board_.piece_at(captured_square);
        if (ep_target.has_value()) {
            info.captured = *ep_target;
            info.captured_square = captured_square;
            zobrist_hash_ ^= z.piece_square[*ep_target][captured_square];
            board_.remove_piece(captured_square);
            is_capture = true;
        }
    } else {
        auto captured = board_.piece_at(to);
        if (captured.has_value()) {
            info.captured = *captured;
            info.captured_square = to;
            zobrist_hash_ ^= z.piece_square[*captured][to];
            board_.remove_piece(to);
            is_capture = true;
        }
    }

    zobrist_hash_ ^= z.piece_square[mover_piece][from];
    board_.remove_piece(from);

    Piece destination_piece = mover_piece;
    if (move.is_promotion()) {
        PieceType promo_type = PieceType::Queen;
        switch (move.promotion()) {
            case Move::PromoKnight: promo_type = PieceType::Knight; break;
            case Move::PromoBishop: promo_type = PieceType::Bishop; break;
            case Move::PromoRook:   promo_type = PieceType::Rook;   break;
            case Move::PromoQueen:  promo_type = PieceType::Queen;  break;
        }
        destination_piece = make_piece(mover, promo_type);
    }
    board_.set_piece(to, destination_piece);
    zobrist_hash_ ^= z.piece_square[destination_piece][to];

    if (move.is_castling()) {
        const int rank = from >> 3;
        const int to_file = to & 7;
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

    if (moving_type == PieceType::King) {
        if (mover == Color::White) {
            castling_.clear(CastlingRights::WhiteKingSide | CastlingRights::WhiteQueenSide);
        } else {
            castling_.clear(CastlingRights::BlackKingSide | CastlingRights::BlackQueenSide);
        }
    }
    castling_.clear(castling_loss_for_square(from));
    castling_.clear(castling_loss_for_square(to));

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

    if (moving_type == PieceType::Pawn || is_capture) {
        halfmove_clock_ = 0;
    } else {
        halfmove_clock_ += 1;
    }

    if (side_to_move_ == Color::Black) {
        fullmove_number_ += 1;
    }
    side_to_move_ = side_to_move_ == Color::White ? Color::Black : Color::White;

    zobrist_hash_ ^= z.castling[castling_.mask & 0xF];
    if (en_passant_square_.has_value()) {
        zobrist_hash_ ^= z.en_passant_file[*en_passant_square_ & 7];
    }
    if (side_to_move_ == Color::Black) {
        zobrist_hash_ ^= z.black_to_move;
    }

    return info;
}

void Position::unmake_move(const UndoInfo& info) {
    const Move move = info.move;
    const int from = move.from();
    const int to = move.to();

    // Flip side back; the piece now on `to` belongs to the side that moved.
    side_to_move_ = info.prior_side;
    fullmove_number_ = info.prior_fullmove;
    halfmove_clock_ = info.prior_halfmove;
    castling_ = info.prior_castling;
    en_passant_square_ = info.prior_ep_square;
    zobrist_hash_ = info.prior_zobrist;

    // Determine the piece sitting on `to`. For promotions this is the promoted
    // piece; we need to replace it with a pawn of the moving side.
    auto on_to = board_.piece_at(to);
    if (!on_to.has_value()) {
        return;  // shouldn't happen for a well-formed UndoInfo
    }
    const Color mover = info.prior_side;
    Piece moving_back;
    if (move.is_promotion()) {
        moving_back = make_piece(mover, PieceType::Pawn);
    } else {
        moving_back = *on_to;
    }

    board_.remove_piece(to);
    board_.set_piece(from, moving_back);

    if (info.captured.has_value()) {
        board_.set_piece(info.captured_square, *info.captured);
    }

    if (move.is_castling()) {
        const int rank = from >> 3;
        const int to_file = to & 7;
        int rook_from = 0;
        int rook_to = 0;
        if (to_file == 6) {
            rook_from = square_index(7, rank);
            rook_to = square_index(5, rank);
        } else {
            rook_from = square_index(0, rank);
            rook_to = square_index(3, rank);
        }
        auto rook = board_.piece_at(rook_to);
        if (rook.has_value()) {
            board_.remove_piece(rook_to);
            board_.set_piece(rook_from, *rook);
        }
    }
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
