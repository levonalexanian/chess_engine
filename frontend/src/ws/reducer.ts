import type { ServerMessage } from "./messages";

export type ConnectionState =
  | "connecting"
  | "connected"
  | "disconnected"
  | "reconnecting";

export type Color = "white" | "black";

export type GameOverReason =
  | "checkmate"
  | "stalemate"
  | "threefold"
  | "insufficient"
  | "fifty"
  | "unknown";

export type GameOverInfo = {
  reason: GameOverReason;
  winner?: Color;
};

export type GameState = {
  startingFen: string;
  moves: string[];
  sans: string[];
  fen: string;
  turn: Color;
  inCheck: boolean;
  humanColor: Color;
  engine: string;
  gameOver?: GameOverInfo;
};

export type Notice =
  | { kind: "reconnecting" }
  | { kind: "reconnected" }
  | { kind: "error"; message: string }
  | { kind: "warning"; message: string };

export type AppState = {
  connection: ConnectionState;
  game: GameState | null;
  pendingEngineRequest: boolean;
  reconnectAttempt: number;
  notice: Notice | null;
};

export type Action =
  | { kind: "wsOpen" }
  | { kind: "wsClose" }
  | { kind: "wsReconnecting"; attempt: number }
  | { kind: "wsReconnected" }
  | { kind: "serverMessage"; msg: ServerMessage }
  | {
      kind: "startNewGame";
      engine: string;
      startingFen: string;
      humanColor: Color;
    }
  | {
      kind: "userMoveApplied";
      uci: string;
      san: string;
      fen: string;
      turn: Color;
      inCheck: boolean;
      gameOver?: GameOverInfo;
    }
  | {
      kind: "engineMoveApplied";
      uci: string;
      san: string;
      fen: string;
      turn: Color;
      inCheck: boolean;
      gameOver?: GameOverInfo;
    }
  | { kind: "requestEngineMove" }
  | { kind: "clearNotice" };

export const initialState: AppState = {
  connection: "connecting",
  game: null,
  pendingEngineRequest: false,
  reconnectAttempt: 0,
  notice: null,
};

function turnFromFen(fen: string): Color {
  const parts = fen.split(" ");
  return parts[1] === "b" ? "black" : "white";
}

export function reduce(state: AppState, action: Action): AppState {
  switch (action.kind) {
    case "wsOpen":
      return {
        ...state,
        connection: "connected",
        reconnectAttempt: 0,
      };
    case "wsClose":
      return {
        ...state,
        connection: "disconnected",
      };
    case "wsReconnecting":
      return {
        ...state,
        connection: "reconnecting",
        reconnectAttempt: action.attempt,
        notice: { kind: "reconnecting" },
      };
    case "wsReconnected":
      return {
        ...state,
        connection: "connected",
        reconnectAttempt: 0,
        notice: { kind: "reconnected" },
      };
    case "startNewGame": {
      const game: GameState = {
        startingFen: action.startingFen,
        moves: [],
        sans: [],
        fen: action.startingFen,
        turn: turnFromFen(action.startingFen),
        inCheck: false,
        humanColor: action.humanColor,
        engine: action.engine,
      };
      return {
        ...state,
        game,
        pendingEngineRequest: false,
        notice: null,
      };
    }
    case "userMoveApplied": {
      if (state.game === null) {
        return state;
      }
      const game: GameState = {
        ...state.game,
        moves: [...state.game.moves, action.uci],
        sans: [...state.game.sans, action.san],
        fen: action.fen,
        turn: action.turn,
        inCheck: action.inCheck,
        gameOver: action.gameOver,
      };
      return { ...state, game };
    }
    case "engineMoveApplied": {
      if (state.game === null) {
        return state;
      }
      const game: GameState = {
        ...state.game,
        moves: [...state.game.moves, action.uci],
        sans: [...state.game.sans, action.san],
        fen: action.fen,
        turn: action.turn,
        inCheck: action.inCheck,
        gameOver: action.gameOver,
      };
      return { ...state, game, pendingEngineRequest: false };
    }
    case "requestEngineMove":
      return { ...state, pendingEngineRequest: true };
    case "serverMessage": {
      const msg = action.msg;
      if (msg.type === "state") {
        if (state.game === null) {
          return state;
        }
        if (state.game.fen === msg.fen) {
          return state;
        }
        return {
          ...state,
          game: { ...state.game, fen: msg.fen, turn: turnFromFen(msg.fen) },
          notice: {
            kind: "warning",
            message: `server FEN diverged from local: ${msg.fen}`,
          },
        };
      }
      if (msg.type === "error") {
        return {
          ...state,
          notice: { kind: "error", message: msg.message },
          pendingEngineRequest: false,
        };
      }
      return state;
    }
    case "clearNotice":
      return { ...state, notice: null };
    default:
      return state;
  }
}
