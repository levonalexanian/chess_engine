import type { Color, ConnectionState } from "../ws/reducer";

export type StatusBarProps = {
  connection: ConnectionState;
  turn: Color;
  inCheck: boolean;
  humanColor: Color;
  engineThinking: boolean;
  gameOver: boolean;
};

function connectionLabel(c: ConnectionState): string {
  switch (c) {
    case "connecting":
      return "connecting";
    case "connected":
      return "connected";
    case "disconnected":
      return "disconnected";
    case "reconnecting":
      return "reconnecting";
  }
}

function connectionDot(c: ConnectionState): string {
  switch (c) {
    case "connecting":
      return "bg-amber-400";
    case "connected":
      return "bg-emerald-500";
    case "disconnected":
      return "bg-rose-500";
    case "reconnecting":
      return "bg-amber-400";
  }
}

export default function StatusBar({
  connection,
  turn,
  inCheck,
  humanColor,
  engineThinking,
  gameOver,
}: StatusBarProps) {
  const yourTurn = !gameOver && !engineThinking && turn === humanColor;
  let turnLabel: string;
  let turnClass: string;
  if (gameOver) {
    turnLabel = "Game over";
    turnClass = "text-slate-500";
  } else if (engineThinking) {
    turnLabel = "Engine thinking…";
    turnClass = "text-amber-600";
  } else if (yourTurn) {
    turnLabel = "Your move";
    turnClass = "text-emerald-700";
  } else {
    turnLabel = turn === "white" ? "White to move" : "Black to move";
    turnClass = "text-slate-700";
  }

  return (
    <div className="flex items-center justify-between border border-slate-200 rounded-md bg-white px-3 py-2">
      <div className="flex flex-col gap-1">
        <span className={`text-sm font-semibold ${turnClass}`}>{turnLabel}</span>
        {inCheck && !gameOver && (
          <span className="text-xs uppercase tracking-wider text-rose-600 font-semibold">
            Check
          </span>
        )}
      </div>
      <div className="flex items-center gap-2 text-xs text-slate-600">
        <span
          className={`inline-block w-2.5 h-2.5 rounded-full ${connectionDot(connection)}`}
          aria-hidden="true"
        />
        <span>{connectionLabel(connection)}</span>
      </div>
    </div>
  );
}
