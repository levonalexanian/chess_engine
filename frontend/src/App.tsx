import { useEffect, useState } from "react";
import { Chessboard } from "react-chessboard";

type ConnectionStatus = "connecting" | "connected" | "disconnected";

const DEFAULT_WS_URL = "ws://localhost:8080/ws";
const START_FEN =
  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

function statusLabel(status: ConnectionStatus): string {
  switch (status) {
    case "connecting":
      return "connecting…";
    case "connected":
      return "connected";
    case "disconnected":
      return "disconnected";
  }
}

function statusColor(status: ConnectionStatus): string {
  switch (status) {
    case "connecting":
      return "bg-amber-400";
    case "connected":
      return "bg-emerald-500";
    case "disconnected":
      return "bg-rose-500";
  }
}

export default function App() {
  const [status, setStatus] = useState<ConnectionStatus>("connecting");

  useEffect(() => {
    const url = import.meta.env.VITE_WS_URL ?? DEFAULT_WS_URL;
    const socket = new WebSocket(url);

    let opened = false;

    socket.addEventListener("open", () => {
      opened = true;
      setStatus("connected");
      socket.send(JSON.stringify({ type: "new_game", engine: "placeholder" }));
      socket.send(JSON.stringify({ type: "user_move", uci: "e2e4" }));
      socket.send(JSON.stringify({ type: "request_engine_move" }));
    });

    socket.addEventListener("message", (event) => {
      console.log("ws message:", event.data);
    });

    socket.addEventListener("error", (event) => {
      console.log("ws error:", event);
    });

    socket.addEventListener("close", () => {
      setStatus("disconnected");
    });

    return () => {
      if (opened || socket.readyState === WebSocket.OPEN) {
        socket.close();
      } else {
        socket.addEventListener("open", () => socket.close(), { once: true });
      }
    };
  }, []);

  return (
    <div className="min-h-screen flex flex-col items-center justify-start py-8 px-4">
      <header className="w-full max-w-xl flex items-center justify-between mb-6">
        <h1 className="text-xl font-semibold tracking-tight">Chess Engine</h1>
        <div className="flex items-center gap-2 text-sm text-slate-600">
          <span
            className={`inline-block w-2.5 h-2.5 rounded-full ${statusColor(status)}`}
            aria-hidden="true"
          />
          <span>{statusLabel(status)}</span>
        </div>
      </header>
      <main className="w-full max-w-xl">
        <div className="aspect-square">
          <Chessboard id="main-board" position={START_FEN} />
        </div>
      </main>
    </div>
  );
}
