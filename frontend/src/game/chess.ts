import { Chess, type Square } from "chess.js";
import type { Color, GameOverInfo } from "../ws/reducer";

export const START_FEN =
  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

export type MoveAttempt =
  | {
      ok: true;
      uci: string;
      san: string;
      fen: string;
      turn: Color;
      inCheck: boolean;
      gameOver?: GameOverInfo;
    }
  | { ok: false };

export type Promotion = "q" | "r" | "b" | "n";

export function newGame(fen?: string): Chess {
  return new Chess(fen ?? START_FEN);
}

export function tryUserMove(
  chess: Chess,
  from: Square,
  to: Square,
  promotion: Promotion = "q"
): MoveAttempt {
  try {
    const move = chess.move({ from, to, promotion });
    if (!move) {
      return { ok: false };
    }
    return {
      ok: true,
      uci: moveToUci(from, to, move.promotion),
      san: move.san,
      fen: chess.fen(),
      turn: turnOf(chess),
      inCheck: chess.isCheck(),
      gameOver: detectGameOver(chess),
    };
  } catch {
    return { ok: false };
  }
}

export function applyUciMove(
  chess: Chess,
  uci: string
): MoveAttempt {
  const { from, to, promotion } = splitUci(uci);
  if (from === null || to === null) {
    return { ok: false };
  }
  try {
    const move = chess.move({
      from,
      to,
      ...(promotion ? { promotion } : {}),
    });
    if (!move) {
      return { ok: false };
    }
    return {
      ok: true,
      uci: moveToUci(from, to, move.promotion),
      san: move.san,
      fen: chess.fen(),
      turn: turnOf(chess),
      inCheck: chess.isCheck(),
      gameOver: detectGameOver(chess),
    };
  } catch {
    return { ok: false };
  }
}

export function turnOf(chess: Chess): Color {
  return chess.turn() === "b" ? "black" : "white";
}

export function detectGameOver(chess: Chess): GameOverInfo | undefined {
  if (!chess.isGameOver()) {
    return undefined;
  }
  if (chess.isCheckmate()) {
    return {
      reason: "checkmate",
      winner: chess.turn() === "b" ? "white" : "black",
    };
  }
  if (chess.isStalemate()) {
    return { reason: "stalemate" };
  }
  if (chess.isThreefoldRepetition()) {
    return { reason: "threefold" };
  }
  if (chess.isInsufficientMaterial()) {
    return { reason: "insufficient" };
  }
  if (chess.isDraw()) {
    return { reason: "fifty" };
  }
  return { reason: "unknown" };
}

export function findKingSquare(chess: Chess): Square | null {
  const sideChar = chess.turn();
  const board = chess.board();
  for (let r = 0; r < 8; r += 1) {
    const row = board[r];
    for (let f = 0; f < 8; f += 1) {
      const piece = row[f];
      if (piece && piece.type === "k" && piece.color === sideChar) {
        const file = "abcdefgh"[f];
        const rank = 8 - r;
        return `${file}${rank}` as Square;
      }
    }
  }
  return null;
}

export function moveToUci(
  from: string,
  to: string,
  promotion?: string
): string {
  return `${from}${to}${promotion ?? ""}`;
}

export function splitUci(uci: string): {
  from: Square | null;
  to: Square | null;
  promotion: Promotion | null;
} {
  if (uci.length < 4 || uci.length > 5) {
    return { from: null, to: null, promotion: null };
  }
  const from = uci.slice(0, 2);
  const to = uci.slice(2, 4);
  const promotionChar = uci.length === 5 ? uci.charAt(4) : "";
  const promotion: Promotion | null =
    promotionChar === "q" ||
    promotionChar === "r" ||
    promotionChar === "b" ||
    promotionChar === "n"
      ? (promotionChar as Promotion)
      : null;
  if (!isSquare(from) || !isSquare(to)) {
    return { from: null, to: null, promotion: null };
  }
  return {
    from: from as Square,
    to: to as Square,
    promotion,
  };
}

function isSquare(s: string): boolean {
  if (s.length !== 2) return false;
  const file = s.charCodeAt(0);
  const rank = s.charCodeAt(1);
  return file >= 97 && file <= 104 && rank >= 49 && rank <= 56;
}
