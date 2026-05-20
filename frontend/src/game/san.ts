import { Chess } from "chess.js";
import { applyUciMove, START_FEN } from "./chess";

export function uciListToSans(
  uciMoves: string[],
  startingFen: string = START_FEN
): string[] {
  const chess = new Chess(startingFen);
  const sans: string[] = [];
  for (const uci of uciMoves) {
    const result = applyUciMove(chess, uci);
    if (!result.ok) {
      throw new Error(`illegal move in history: ${uci}`);
    }
    sans.push(result.san);
  }
  return sans;
}

export type SanPair = {
  index: number;
  white: string;
  black?: string;
};

export function groupSansIntoPairs(
  sans: string[],
  startingFullmove: number = 1,
  startingSide: "white" | "black" = "white"
): SanPair[] {
  const pairs: SanPair[] = [];
  let i = 0;
  let move = startingFullmove;
  if (startingSide === "black" && sans.length > 0) {
    pairs.push({ index: move, white: "...", black: sans[0] });
    i = 1;
    move += 1;
  }
  while (i < sans.length) {
    const white = sans[i];
    const black = i + 1 < sans.length ? sans[i + 1] : undefined;
    pairs.push({ index: move, white, black });
    i += 2;
    move += 1;
  }
  return pairs;
}
