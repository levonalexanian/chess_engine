import { describe, expect, it } from "vitest";
import {
  initialState,
  reduce,
  type Action,
  type AppState,
} from "../ws/reducer";

const START_FEN =
  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
const AFTER_E4 =
  "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1";
const AFTER_E4_E5 =
  "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2";

function apply(state: AppState, ...actions: Action[]): AppState {
  return actions.reduce((acc, action) => reduce(acc, action), state);
}

describe("reducer", () => {
  it("starts in connecting state with no game", () => {
    expect(initialState.connection).toBe("connecting");
    expect(initialState.game).toBeNull();
    expect(initialState.pendingEngineRequest).toBe(false);
  });

  it("transitions to connected on wsOpen", () => {
    const next = reduce(initialState, { kind: "wsOpen" });
    expect(next.connection).toBe("connected");
    expect(next.reconnectAttempt).toBe(0);
  });

  it("transitions to disconnected on wsClose", () => {
    const opened = reduce(initialState, { kind: "wsOpen" });
    const closed = reduce(opened, { kind: "wsClose" });
    expect(closed.connection).toBe("disconnected");
  });

  it("records reconnect attempt and emits reconnecting notice", () => {
    const next = reduce(initialState, {
      kind: "wsReconnecting",
      attempt: 3,
    });
    expect(next.connection).toBe("reconnecting");
    expect(next.reconnectAttempt).toBe(3);
    expect(next.notice).toEqual({ kind: "reconnecting" });
  });

  it("clears reconnect counter and emits reconnected notice on wsReconnected", () => {
    const dropped = apply(initialState, { kind: "wsReconnecting", attempt: 2 });
    const back = reduce(dropped, { kind: "wsReconnected" });
    expect(back.connection).toBe("connected");
    expect(back.reconnectAttempt).toBe(0);
    expect(back.notice).toEqual({ kind: "reconnected" });
  });

  it("startNewGame initializes a fresh game for white", () => {
    const state = reduce(initialState, {
      kind: "startNewGame",
      engine: "random",
      startingFen: START_FEN,
      humanColor: "white",
    });
    expect(state.game).not.toBeNull();
    expect(state.game?.fen).toBe(START_FEN);
    expect(state.game?.turn).toBe("white");
    expect(state.game?.humanColor).toBe("white");
    expect(state.game?.moves).toEqual([]);
    expect(state.game?.sans).toEqual([]);
    expect(state.pendingEngineRequest).toBe(false);
  });

  it("startNewGame initializes for black with same starting FEN", () => {
    const state = reduce(initialState, {
      kind: "startNewGame",
      engine: "random",
      startingFen: START_FEN,
      humanColor: "black",
    });
    expect(state.game?.humanColor).toBe("black");
    expect(state.game?.turn).toBe("white");
  });

  it("startNewGame derives turn from custom starting FEN", () => {
    const state = reduce(initialState, {
      kind: "startNewGame",
      engine: "random",
      startingFen: AFTER_E4,
      humanColor: "white",
    });
    expect(state.game?.turn).toBe("black");
  });

  it("userMoveApplied appends to history and updates fen/turn", () => {
    const gameStarted = reduce(initialState, {
      kind: "startNewGame",
      engine: "random",
      startingFen: START_FEN,
      humanColor: "white",
    });
    const next = reduce(gameStarted, {
      kind: "userMoveApplied",
      uci: "e2e4",
      san: "e4",
      fen: AFTER_E4,
      turn: "black",
      inCheck: false,
    });
    expect(next.game?.moves).toEqual(["e2e4"]);
    expect(next.game?.sans).toEqual(["e4"]);
    expect(next.game?.fen).toBe(AFTER_E4);
    expect(next.game?.turn).toBe("black");
  });

  it("userMoveApplied is a no-op when no game is in flight", () => {
    const next = reduce(initialState, {
      kind: "userMoveApplied",
      uci: "e2e4",
      san: "e4",
      fen: AFTER_E4,
      turn: "black",
      inCheck: false,
    });
    expect(next).toBe(initialState);
  });

  it("requestEngineMove sets pending flag", () => {
    const gameStarted = reduce(initialState, {
      kind: "startNewGame",
      engine: "random",
      startingFen: START_FEN,
      humanColor: "white",
    });
    const next = reduce(gameStarted, { kind: "requestEngineMove" });
    expect(next.pendingEngineRequest).toBe(true);
  });

  it("engineMoveApplied appends history, clears pending flag", () => {
    const gameStarted = reduce(initialState, {
      kind: "startNewGame",
      engine: "random",
      startingFen: START_FEN,
      humanColor: "black",
    });
    const requested = reduce(gameStarted, { kind: "requestEngineMove" });
    const replied = reduce(requested, {
      kind: "engineMoveApplied",
      uci: "e2e4",
      san: "e4",
      fen: AFTER_E4,
      turn: "black",
      inCheck: false,
    });
    expect(replied.pendingEngineRequest).toBe(false);
    expect(replied.game?.moves).toEqual(["e2e4"]);
    expect(replied.game?.sans).toEqual(["e4"]);
    expect(replied.game?.fen).toBe(AFTER_E4);
  });

  it("engineMoveApplied carries gameOver info", () => {
    const gameStarted = reduce(initialState, {
      kind: "startNewGame",
      engine: "random",
      startingFen: START_FEN,
      humanColor: "white",
    });
    const next = reduce(gameStarted, {
      kind: "engineMoveApplied",
      uci: "d8h4",
      san: "Qxh4#",
      fen: AFTER_E4_E5,
      turn: "white",
      inCheck: true,
      gameOver: { reason: "checkmate", winner: "black" },
    });
    expect(next.game?.gameOver).toEqual({
      reason: "checkmate",
      winner: "black",
    });
  });

  it("serverMessage state matching local fen is a no-op", () => {
    const gameStarted = reduce(initialState, {
      kind: "startNewGame",
      engine: "random",
      startingFen: START_FEN,
      humanColor: "white",
    });
    const applied = reduce(gameStarted, {
      kind: "userMoveApplied",
      uci: "e2e4",
      san: "e4",
      fen: AFTER_E4,
      turn: "black",
      inCheck: false,
    });
    const next = reduce(applied, {
      kind: "serverMessage",
      msg: { type: "state", fen: AFTER_E4 },
    });
    expect(next).toBe(applied);
  });

  it("serverMessage state diverging from local fen records warning and adopts server fen", () => {
    const gameStarted = reduce(initialState, {
      kind: "startNewGame",
      engine: "random",
      startingFen: START_FEN,
      humanColor: "white",
    });
    const next = reduce(gameStarted, {
      kind: "serverMessage",
      msg: { type: "state", fen: AFTER_E4_E5 },
    });
    expect(next.game?.fen).toBe(AFTER_E4_E5);
    expect(next.game?.turn).toBe("white");
    expect(next.notice?.kind).toBe("warning");
  });

  it("serverMessage error stores notice and clears pending flag", () => {
    const requested = apply(
      initialState,
      {
        kind: "startNewGame",
        engine: "random",
        startingFen: START_FEN,
        humanColor: "white",
      },
      { kind: "requestEngineMove" }
    );
    const next = reduce(requested, {
      kind: "serverMessage",
      msg: { type: "error", message: "boom" },
    });
    expect(next.notice).toEqual({ kind: "error", message: "boom" });
    expect(next.pendingEngineRequest).toBe(false);
  });

  it("clearNotice clears any active notice", () => {
    const withNotice = reduce(initialState, {
      kind: "wsReconnecting",
      attempt: 1,
    });
    const cleared = reduce(withNotice, { kind: "clearNotice" });
    expect(cleared.notice).toBeNull();
  });

  it("preserves game state across disconnect and reconnect for resync", () => {
    const afterMove = apply(
      initialState,
      { kind: "wsOpen" },
      {
        kind: "startNewGame",
        engine: "random",
        startingFen: START_FEN,
        humanColor: "white",
      },
      {
        kind: "userMoveApplied",
        uci: "e2e4",
        san: "e4",
        fen: AFTER_E4,
        turn: "black",
        inCheck: false,
      }
    );
    const droppedAndBack = apply(
      afterMove,
      { kind: "wsClose" },
      { kind: "wsReconnecting", attempt: 1 },
      { kind: "wsReconnected" }
    );
    expect(droppedAndBack.game?.startingFen).toBe(START_FEN);
    expect(droppedAndBack.game?.moves).toEqual(["e2e4"]);
    expect(droppedAndBack.game?.fen).toBe(AFTER_E4);
    expect(droppedAndBack.connection).toBe("connected");
  });
});
