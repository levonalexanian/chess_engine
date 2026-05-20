import { Chessboard } from "react-chessboard";
import type { Square } from "chess.js";

export type BoardProps = {
  fen: string;
  orientation?: "white" | "black";
  highlightSquare?: Square | null;
  draggable?: boolean;
  onPieceDrop?: (from: Square, to: Square, piece: string) => boolean;
};

export default function Board({
  fen,
  orientation = "white",
  highlightSquare,
  draggable = true,
  onPieceDrop,
}: BoardProps) {
  const customSquareStyles =
    highlightSquare !== null && highlightSquare !== undefined
      ? {
          [highlightSquare]: {
            backgroundColor: "rgba(220, 38, 38, 0.45)",
          },
        }
      : undefined;

  return (
    <div className="aspect-square w-full">
      <Chessboard
        id="main-board"
        position={fen}
        boardOrientation={orientation}
        arePiecesDraggable={draggable}
        onPieceDrop={(from, to, piece) => {
          if (onPieceDrop === undefined) {
            return false;
          }
          return onPieceDrop(from as Square, to as Square, piece);
        }}
        customSquareStyles={customSquareStyles}
        customDarkSquareStyle={{ backgroundColor: "#779556" }}
        customLightSquareStyle={{ backgroundColor: "#edeed1" }}
      />
    </div>
  );
}
