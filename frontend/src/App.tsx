import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import { Chess } from "chess.js";
import type { Square } from "chess.js";
import GameOverBanner from "./components/GameOverBanner";
import NewGameDialog, {
  type NewGameChoice,
} from "./components/NewGameDialog";
import Toast from "./components/Toast";
import Play from "./pages/Play";
import { useWs } from "./ws/hooks";
import { findKingSquare, START_FEN } from "./game/chess";

const DEFAULT_WS_URL = "ws://localhost:8080/ws";

export default function App() {
  const url = (import.meta.env.VITE_WS_URL as string | undefined) ?? DEFAULT_WS_URL;
  const ws = useWs(url);
  const startedRef = useRef(false);
  const [dialogOpen, setDialogOpen] = useState(false);

  useEffect(() => {
    if (ws.state.connection === "connected" && !startedRef.current) {
      startedRef.current = true;
      ws.startNewGame({ engine: "random", humanColor: "white" });
    }
  }, [ws]);

  const startingFullmove = useMemo(() => {
    const fen = ws.state.game?.startingFen ?? START_FEN;
    const parts = fen.split(" ");
    const parsed = Number.parseInt(parts[5] ?? "1", 10);
    return Number.isFinite(parsed) ? parsed : 1;
  }, [ws.state.game?.startingFen]);

  const startingSide = useMemo(() => {
    const fen = ws.state.game?.startingFen ?? START_FEN;
    return fen.split(" ")[1] === "b" ? "black" : "white";
  }, [ws.state.game?.startingFen]);

  const highlightSquare: Square | null = useMemo(() => {
    const game = ws.state.game;
    if (game === null || !game.inCheck) return null;
    const chess = new Chess(game.fen);
    return findKingSquare(chess);
  }, [ws.state.game]);

  const game = ws.state.game;

  const handleStart = useCallback(
    (choice: NewGameChoice) => {
      setDialogOpen(false);
      ws.startNewGame({
        engine: choice.engine,
        humanColor: choice.humanColor,
        startingFen: choice.startingFen,
      });
    },
    [ws]
  );

  return (
    <div className="min-h-screen flex flex-col items-stretch py-6 px-4 bg-slate-50 text-slate-900">
      <header className="w-full max-w-5xl mx-auto flex items-center justify-between mb-6">
        <h1 className="text-xl font-semibold tracking-tight">Chess Engine</h1>
        <button
          type="button"
          onClick={() => setDialogOpen(true)}
          className="text-sm bg-slate-800 hover:bg-slate-700 text-white font-medium px-3 py-1.5 rounded-md"
        >
          New game
        </button>
      </header>
      <main className="flex-1 w-full">
        <Play
          fen={game?.fen ?? START_FEN}
          sans={game?.sans ?? []}
          connection={ws.state.connection}
          turn={game?.turn ?? "white"}
          inCheck={game?.inCheck ?? false}
          humanColor={game?.humanColor ?? "white"}
          engineThinking={ws.state.pendingEngineRequest}
          gameOver={game?.gameOver}
          highlightSquare={highlightSquare}
          startingFullmove={startingFullmove}
          startingSide={startingSide}
          onPieceDrop={(from, to) => ws.attemptUserMove(from, to)}
        />
      </main>
      {game?.gameOver !== undefined && (
        <GameOverBanner
          gameOver={game.gameOver}
          onNewGame={() => setDialogOpen(true)}
        />
      )}
      <NewGameDialog
        open={dialogOpen}
        defaultEngine={game?.engine ?? "random"}
        onCancel={() => setDialogOpen(false)}
        onStart={handleStart}
      />
      <Toast notice={ws.state.notice} onDismiss={ws.clearNotice} />
    </div>
  );
}
