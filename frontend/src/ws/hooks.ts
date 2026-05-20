import { useCallback, useEffect, useMemo, useReducer, useRef } from "react";
import { Chess } from "chess.js";
import type { Square } from "chess.js";
import {
  applyUciMove,
  detectGameOver,
  newGame,
  splitUci,
  START_FEN,
  tryUserMove,
  turnOf,
  type Promotion,
} from "../game/chess";
import { ReconnectingClient } from "./client";
import { parseServerMessage, type ClientMessage } from "./messages";
import {
  initialState,
  reduce,
  type AppState,
  type Color,
} from "./reducer";

export type NewGameOptions = {
  engine: string;
  humanColor: Color;
  startingFen?: string;
};

export type UseWsResult = {
  state: AppState;
  startNewGame: (opts: NewGameOptions) => void;
  attemptUserMove: (from: Square, to: Square, promotion?: Promotion) => boolean;
  clearNotice: () => void;
};

const DEFAULT_WS_URL = "ws://localhost:8080/ws";

export function useWs(url: string = DEFAULT_WS_URL): UseWsResult {
  const [state, dispatch] = useReducer(reduce, initialState);
  const stateRef = useRef(state);
  stateRef.current = state;

  const chessRef = useRef<Chess>(newGame());
  const clientRef = useRef<ReconnectingClient | null>(null);

  const sendClient = useCallback((msg: ClientMessage) => {
    clientRef.current?.send(msg);
  }, []);

  const resync = useCallback(() => {
    const game = stateRef.current.game;
    if (game === null) {
      return;
    }
    sendClient({
      type: "new_game",
      engine: game.engine,
      starting_fen: game.startingFen,
      moves: [...game.moves],
    });
  }, [sendClient]);

  const handleServerMessage = useCallback(
    (raw: string) => {
      const parsed = parseServerMessage(raw);
      if (parsed === null) {
        return;
      }
      if (parsed.type === "engine_move" && parsed.uci !== "0000") {
        const result = applyUciMove(chessRef.current, parsed.uci);
        if (result.ok) {
          dispatch({
            kind: "engineMoveApplied",
            uci: result.uci,
            san: result.san,
            fen: result.fen,
            turn: result.turn,
            inCheck: result.inCheck,
            gameOver: result.gameOver,
          });
          return;
        }
        dispatch({
          kind: "serverMessage",
          msg: {
            type: "error",
            message: `could not apply engine move ${parsed.uci}`,
          },
        });
        return;
      }
      if (parsed.type === "engine_move" && parsed.uci === "0000") {
        dispatch({
          kind: "engineMoveApplied",
          uci: "0000",
          san: "(null)",
          fen: chessRef.current.fen(),
          turn: turnOf(chessRef.current),
          inCheck: chessRef.current.isCheck(),
          gameOver: detectGameOver(chessRef.current),
        });
        return;
      }
      dispatch({ kind: "serverMessage", msg: parsed });
    },
    [dispatch]
  );

  useEffect(() => {
    const client = new ReconnectingClient({
      url,
      callbacks: {
        onOpen: () => dispatch({ kind: "wsOpen" }),
        onMessage: handleServerMessage,
        onClose: () => dispatch({ kind: "wsClose" }),
        onReconnecting: (attempt) =>
          dispatch({ kind: "wsReconnecting", attempt }),
        onReconnected: () => {
          dispatch({ kind: "wsReconnected" });
          resync();
        },
      },
    });
    clientRef.current = client;
    client.connect();
    return () => {
      client.close();
      clientRef.current = null;
    };
  }, [url, handleServerMessage, resync]);

  const startNewGame = useCallback(
    (opts: NewGameOptions) => {
      const startingFen = opts.startingFen ?? START_FEN;
      chessRef.current = newGame(startingFen);
      dispatch({
        kind: "startNewGame",
        engine: opts.engine,
        startingFen,
        humanColor: opts.humanColor,
      });
      sendClient({
        type: "new_game",
        engine: opts.engine,
        starting_fen: startingFen,
        moves: [],
      });
      const initialTurn = turnOf(chessRef.current);
      if (initialTurn !== opts.humanColor) {
        dispatch({ kind: "requestEngineMove" });
        sendClient({ type: "request_engine_move" });
      }
    },
    [sendClient]
  );

  const attemptUserMove = useCallback(
    (from: Square, to: Square, promotion: Promotion = "q"): boolean => {
      const game = stateRef.current.game;
      if (game === null) {
        return false;
      }
      if (game.gameOver !== undefined) {
        return false;
      }
      if (stateRef.current.pendingEngineRequest) {
        return false;
      }
      if (turnOf(chessRef.current) !== game.humanColor) {
        return false;
      }
      const result = tryUserMove(chessRef.current, from, to, promotion);
      if (!result.ok) {
        return false;
      }
      dispatch({
        kind: "userMoveApplied",
        uci: result.uci,
        san: result.san,
        fen: result.fen,
        turn: result.turn,
        inCheck: result.inCheck,
        gameOver: result.gameOver,
      });
      sendClient({ type: "user_move", uci: result.uci });
      if (result.gameOver === undefined) {
        dispatch({ kind: "requestEngineMove" });
        sendClient({ type: "request_engine_move" });
      }
      return true;
    },
    [sendClient]
  );

  const clearNotice = useCallback(() => {
    dispatch({ kind: "clearNotice" });
  }, []);

  return useMemo(
    () => ({
      state,
      startNewGame,
      attemptUserMove,
      clearNotice,
    }),
    [state, startNewGame, attemptUserMove, clearNotice]
  );
}

// Re-export for tests that want to construct UCI tokens.
export { splitUci };
