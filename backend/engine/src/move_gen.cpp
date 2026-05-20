#include <array>
#include <bit>
#include <cstdint>
#include <vector>

#include "chess_engine/attacks.hpp"
#include "chess_engine/board.hpp"
#include "chess_engine/move.hpp"
#include "chess_engine/position.hpp"

namespace chess_engine {

namespace {

constexpr Bitboard kRank1 = Bitboard{0xFF};
constexpr Bitboard kRank2 = Bitboard{0xFF} << 8;
constexpr Bitboard kRank7 = Bitboard{0xFF} << 48;
constexpr Bitboard kRank8 = Bitboard{0xFF} << 56;

int pop_lsb(Bitboard& b) {
    const int sq = std::countr_zero(b);
    b &= b - 1;
    return sq;
}

void emit_promotions(std::vector<Move>& moves, int from, int to) {
    moves.emplace_back(static_cast<std::uint8_t>(from), static_cast<std::uint8_t>(to),
                       Move::FlagPromotion, Move::PromoQueen);
    moves.emplace_back(static_cast<std::uint8_t>(from), static_cast<std::uint8_t>(to),
                       Move::FlagPromotion, Move::PromoRook);
    moves.emplace_back(static_cast<std::uint8_t>(from), static_cast<std::uint8_t>(to),
                       Move::FlagPromotion, Move::PromoBishop);
    moves.emplace_back(static_cast<std::uint8_t>(from), static_cast<std::uint8_t>(to),
                       Move::FlagPromotion, Move::PromoKnight);
}

void emit_quiet_or_capture(std::vector<Move>& moves, int from, int to) {
    moves.emplace_back(static_cast<std::uint8_t>(from), static_cast<std::uint8_t>(to));
}

void generate_pawn_moves(const Board& board, Color stm, std::optional<int> ep_square,
                         std::vector<Move>& out) {
    const Color opp_color = stm == Color::White ? Color::Black : Color::White;
    const Bitboard opp = board.occupancy(opp_color);
    const Bitboard all = board.occupancy();
    const Bitboard pawns = stm == Color::White
                               ? board.pieces(Piece::WhitePawn)
                               : board.pieces(Piece::BlackPawn);

    if (stm == Color::White) {
        // Single push: pawn shifted up by 8, not blocked.
        const Bitboard single_push = (pawns << 8) & ~all;
        // Promotions from single push.
        Bitboard promos = single_push & kRank8;
        Bitboard quiets = single_push & ~kRank8;
        while (promos) {
            const int to = pop_lsb(promos);
            emit_promotions(out, to - 8, to);
        }
        while (quiets) {
            const int to = pop_lsb(quiets);
            emit_quiet_or_capture(out, to - 8, to);
        }
        // Double push: pawn on rank 2, push 16 squares, neither intermediate nor destination blocked.
        Bitboard double_push = ((pawns & kRank2) << 8) & ~all;
        double_push = (double_push << 8) & ~all;
        while (double_push) {
            const int to = pop_lsb(double_push);
            emit_quiet_or_capture(out, to - 16, to);
        }
        // Captures: NW (shift 7, must not be from a-file) and NE (shift 9, must not be from h-file).
        constexpr Bitboard kFileA = 0x0101010101010101ULL;
        constexpr Bitboard kFileH = 0x8080808080808080ULL;
        Bitboard nw_cap = ((pawns & ~kFileA) << 7) & opp;
        Bitboard ne_cap = ((pawns & ~kFileH) << 9) & opp;
        // Promotion captures.
        Bitboard nw_promo = nw_cap & kRank8;
        nw_cap &= ~kRank8;
        Bitboard ne_promo = ne_cap & kRank8;
        ne_cap &= ~kRank8;
        while (nw_promo) {
            const int to = pop_lsb(nw_promo);
            emit_promotions(out, to - 7, to);
        }
        while (ne_promo) {
            const int to = pop_lsb(ne_promo);
            emit_promotions(out, to - 9, to);
        }
        while (nw_cap) {
            const int to = pop_lsb(nw_cap);
            emit_quiet_or_capture(out, to - 7, to);
        }
        while (ne_cap) {
            const int to = pop_lsb(ne_cap);
            emit_quiet_or_capture(out, to - 9, to);
        }
        // En passant.
        if (ep_square.has_value()) {
            const int ep = *ep_square;
            const Bitboard ep_bb = Bitboard{1} << ep;
            // White pawns that can capture onto ep: those whose NW or NE attack hits ep.
            Bitboard ep_attackers = 0;
            if ((ep_bb >> 7) & ~kFileA) ep_attackers |= (ep_bb >> 7) & pawns;
            if ((ep_bb >> 9) & ~kFileH) ep_attackers |= (ep_bb >> 9) & pawns;
            while (ep_attackers) {
                const int from = pop_lsb(ep_attackers);
                out.emplace_back(static_cast<std::uint8_t>(from), static_cast<std::uint8_t>(ep),
                                 Move::FlagEnPassant);
            }
        }
    } else {
        // Single push down.
        const Bitboard single_push = (pawns >> 8) & ~all;
        Bitboard promos = single_push & kRank1;
        Bitboard quiets = single_push & ~kRank1;
        while (promos) {
            const int to = pop_lsb(promos);
            emit_promotions(out, to + 8, to);
        }
        while (quiets) {
            const int to = pop_lsb(quiets);
            emit_quiet_or_capture(out, to + 8, to);
        }
        Bitboard double_push = ((pawns & kRank7) >> 8) & ~all;
        double_push = (double_push >> 8) & ~all;
        while (double_push) {
            const int to = pop_lsb(double_push);
            emit_quiet_or_capture(out, to + 16, to);
        }
        constexpr Bitboard kFileA = 0x0101010101010101ULL;
        constexpr Bitboard kFileH = 0x8080808080808080ULL;
        // SW: shift right 9, must not be from h-file (because shifting right 9 of an h-file pawn wraps).
        // Actually for black moving south: SE (relative to white viewpoint) is shift-down-9 from h-file -> a-file (wrong).
        // Let's be careful: black pawn at sq goes to sq-9 (SW from white's view but SE in absolute) and sq-7.
        // sq-9: file decreases by 1 (and rank decreases by 1). Source file must be > 0 (not on file A).
        // sq-7: file increases by 1 (rank decreases). Source file must be < 7 (not on file H).
        Bitboard sw_cap = ((pawns & ~kFileA) >> 9) & opp;
        Bitboard se_cap = ((pawns & ~kFileH) >> 7) & opp;
        Bitboard sw_promo = sw_cap & kRank1;
        sw_cap &= ~kRank1;
        Bitboard se_promo = se_cap & kRank1;
        se_cap &= ~kRank1;
        while (sw_promo) {
            const int to = pop_lsb(sw_promo);
            emit_promotions(out, to + 9, to);
        }
        while (se_promo) {
            const int to = pop_lsb(se_promo);
            emit_promotions(out, to + 7, to);
        }
        while (sw_cap) {
            const int to = pop_lsb(sw_cap);
            emit_quiet_or_capture(out, to + 9, to);
        }
        while (se_cap) {
            const int to = pop_lsb(se_cap);
            emit_quiet_or_capture(out, to + 7, to);
        }
        if (ep_square.has_value()) {
            const int ep = *ep_square;
            const Bitboard ep_bb = Bitboard{1} << ep;
            Bitboard ep_attackers = 0;
            // Attacker at ep+9 (SW-attacker). Shift wraps when ep is on h-file:
            // result lands on a-file. Exclude that.
            if ((ep_bb << 9) & ~kFileA) ep_attackers |= (ep_bb << 9) & pawns;
            // Attacker at ep+7 (SE-attacker). Wraps when ep on a-file: result on h-file.
            if ((ep_bb << 7) & ~kFileH) ep_attackers |= (ep_bb << 7) & pawns;
            while (ep_attackers) {
                const int from = pop_lsb(ep_attackers);
                out.emplace_back(static_cast<std::uint8_t>(from), static_cast<std::uint8_t>(ep),
                                 Move::FlagEnPassant);
            }
        }
    }
}

void generate_piece_moves(const Board& board, Color stm, std::vector<Move>& out) {
    const Bitboard own = board.occupancy(stm);
    const Bitboard all = board.occupancy();
    const Bitboard target_mask = ~own;

    Bitboard knights = stm == Color::White ? board.pieces(Piece::WhiteKnight)
                                           : board.pieces(Piece::BlackKnight);
    while (knights) {
        const int from = pop_lsb(knights);
        Bitboard targets = attacks::knight_attacks(from) & target_mask;
        while (targets) {
            const int to = pop_lsb(targets);
            emit_quiet_or_capture(out, from, to);
        }
    }

    Bitboard bishops = stm == Color::White ? board.pieces(Piece::WhiteBishop)
                                           : board.pieces(Piece::BlackBishop);
    while (bishops) {
        const int from = pop_lsb(bishops);
        Bitboard targets = attacks::bishop_attacks(from, all) & target_mask;
        while (targets) {
            const int to = pop_lsb(targets);
            emit_quiet_or_capture(out, from, to);
        }
    }

    Bitboard rooks = stm == Color::White ? board.pieces(Piece::WhiteRook)
                                         : board.pieces(Piece::BlackRook);
    while (rooks) {
        const int from = pop_lsb(rooks);
        Bitboard targets = attacks::rook_attacks(from, all) & target_mask;
        while (targets) {
            const int to = pop_lsb(targets);
            emit_quiet_or_capture(out, from, to);
        }
    }

    Bitboard queens = stm == Color::White ? board.pieces(Piece::WhiteQueen)
                                          : board.pieces(Piece::BlackQueen);
    while (queens) {
        const int from = pop_lsb(queens);
        Bitboard targets = attacks::queen_attacks(from, all) & target_mask;
        while (targets) {
            const int to = pop_lsb(targets);
            emit_quiet_or_capture(out, from, to);
        }
    }

    Bitboard king = stm == Color::White ? board.pieces(Piece::WhiteKing)
                                        : board.pieces(Piece::BlackKing);
    if (king) {
        const int from = std::countr_zero(king);
        Bitboard targets = attacks::king_attacks(from) & target_mask;
        while (targets) {
            const int to = pop_lsb(targets);
            emit_quiet_or_capture(out, from, to);
        }
    }
}

}  // namespace

namespace {

bool square_attacked(const Board& board, int square, Color attacker) {
    const Bitboard all = board.occupancy();
    // Pawn attackers: invert the pawn-attack table — a square is attacked by a
    // pawn of `attacker` iff the OTHER colour's pawn-attack map from `square`
    // intersects `attacker`'s pawns.
    const Color victim = attacker == Color::White ? Color::Black : Color::White;
    const Bitboard pawn_attackers =
        attacks::pawn_attacks(victim, square) &
        board.pieces(make_piece(attacker, PieceType::Pawn));
    if (pawn_attackers) return true;
    if (attacks::knight_attacks(square) & board.pieces(make_piece(attacker, PieceType::Knight))) {
        return true;
    }
    if (attacks::king_attacks(square) & board.pieces(make_piece(attacker, PieceType::King))) {
        return true;
    }
    const Bitboard bishops_queens = board.pieces(make_piece(attacker, PieceType::Bishop)) |
                                     board.pieces(make_piece(attacker, PieceType::Queen));
    if (attacks::bishop_attacks(square, all) & bishops_queens) {
        return true;
    }
    const Bitboard rooks_queens = board.pieces(make_piece(attacker, PieceType::Rook)) |
                                   board.pieces(make_piece(attacker, PieceType::Queen));
    if (attacks::rook_attacks(square, all) & rooks_queens) {
        return true;
    }
    return false;
}

void generate_castling(const Position& pos, std::vector<Move>& out) {
    const Color stm = pos.side_to_move();
    const Color opp = stm == Color::White ? Color::Black : Color::White;
    const auto castling = pos.castling();
    const Bitboard all = pos.board().occupancy();
    const Board& board = pos.board();

    if (stm == Color::White) {
        // The king must currently be on e1 and not in check.
        if (square_attacked(board, 4, opp)) return;
        if (castling.has(CastlingRights::WhiteKingSide)) {
            // f1, g1 empty; f1 not attacked (g1 will be checked by legality filter).
            constexpr Bitboard between = (Bitboard{1} << 5) | (Bitboard{1} << 6);
            if ((all & between) == 0 && !square_attacked(board, 5, opp) &&
                !square_attacked(board, 6, opp)) {
                out.emplace_back(static_cast<std::uint8_t>(4), static_cast<std::uint8_t>(6),
                                 Move::FlagCastling);
            }
        }
        if (castling.has(CastlingRights::WhiteQueenSide)) {
            // b1, c1, d1 empty; c1, d1 not attacked (b1 may be attacked — only the
            // squares the king moves through matter).
            constexpr Bitboard between = (Bitboard{1} << 1) | (Bitboard{1} << 2) | (Bitboard{1} << 3);
            if ((all & between) == 0 && !square_attacked(board, 2, opp) &&
                !square_attacked(board, 3, opp)) {
                out.emplace_back(static_cast<std::uint8_t>(4), static_cast<std::uint8_t>(2),
                                 Move::FlagCastling);
            }
        }
    } else {
        if (square_attacked(board, 60, opp)) return;
        if (castling.has(CastlingRights::BlackKingSide)) {
            constexpr Bitboard between = (Bitboard{1} << 61) | (Bitboard{1} << 62);
            if ((all & between) == 0 && !square_attacked(board, 61, opp) &&
                !square_attacked(board, 62, opp)) {
                out.emplace_back(static_cast<std::uint8_t>(60), static_cast<std::uint8_t>(62),
                                 Move::FlagCastling);
            }
        }
        if (castling.has(CastlingRights::BlackQueenSide)) {
            constexpr Bitboard between = (Bitboard{1} << 57) | (Bitboard{1} << 58) | (Bitboard{1} << 59);
            if ((all & between) == 0 && !square_attacked(board, 58, opp) &&
                !square_attacked(board, 59, opp)) {
                out.emplace_back(static_cast<std::uint8_t>(60), static_cast<std::uint8_t>(58),
                                 Move::FlagCastling);
            }
        }
    }
}

}  // namespace

bool Position::is_square_attacked(int square, Color attacker) const {
    return square_attacked(board_, square, attacker);
}

std::vector<Move> Position::generate_legal_moves() const {
    auto pseudo = generate_pseudo_legal_moves();
    std::vector<Move> legal;
    legal.reserve(pseudo.size());
    Position scratch = *this;
    for (const auto& m : pseudo) {
        const Color mover = scratch.side_to_move();
        auto undo = scratch.make_move(m);
        // After make_move, side_to_move flipped; we check that mover's king
        // is not attacked by the new side to move.
        const Color opponent = scratch.side_to_move();
        Bitboard king_bb = mover == Color::White
                               ? scratch.board().pieces(Piece::WhiteKing)
                               : scratch.board().pieces(Piece::BlackKing);
        if (king_bb != 0) {
            const int king_sq = std::countr_zero(king_bb);
            if (!scratch.is_square_attacked(king_sq, opponent)) {
                legal.push_back(m);
            }
        }
        scratch.unmake_move(undo);
    }
    return legal;
}

std::vector<Move> Position::generate_pseudo_legal_moves() const {
    std::vector<Move> out;
    out.reserve(64);
    generate_pawn_moves(board_, side_to_move_, en_passant_square_, out);
    generate_piece_moves(board_, side_to_move_, out);
    generate_castling(*this, out);
    return out;
}

}  // namespace chess_engine
