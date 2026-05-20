import type { GameOverInfo } from "../ws/reducer";

export type GameOverBannerProps = {
  gameOver: GameOverInfo;
  onNewGame: () => void;
};

function reasonText(info: GameOverInfo): string {
  switch (info.reason) {
    case "checkmate":
      return info.winner === "white"
        ? "White wins by checkmate"
        : "Black wins by checkmate";
    case "stalemate":
      return "Draw by stalemate";
    case "threefold":
      return "Draw by threefold repetition";
    case "insufficient":
      return "Draw by insufficient material";
    case "fifty":
      return "Draw by the fifty-move rule";
    case "unknown":
      return "Game over";
  }
}

export default function GameOverBanner({
  gameOver,
  onNewGame,
}: GameOverBannerProps) {
  return (
    <div
      className="fixed inset-0 z-30 flex items-center justify-center bg-black/40"
      role="dialog"
      aria-modal="true"
    >
      <div className="bg-white rounded-lg shadow-lg px-8 py-6 max-w-sm w-full mx-4 flex flex-col gap-4 items-center text-center">
        <h2 className="text-lg font-semibold text-slate-800">{reasonText(gameOver)}</h2>
        <button
          type="button"
          onClick={onNewGame}
          className="bg-emerald-600 hover:bg-emerald-700 text-white font-semibold px-4 py-2 rounded-md text-sm"
        >
          New game
        </button>
      </div>
    </div>
  );
}
