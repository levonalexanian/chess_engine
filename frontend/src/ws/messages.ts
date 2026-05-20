export type ClientMessage =
  | {
      type: "new_game";
      engine: string;
      starting_fen?: string;
      moves?: string[];
    }
  | { type: "user_move"; uci: string }
  | { type: "request_engine_move" };

export type ServerMessage =
  | { type: "state"; fen: string }
  | { type: "engine_move"; uci: string }
  | { type: "error"; message: string };

export function parseServerMessage(raw: string): ServerMessage | null {
  let value: unknown;
  try {
    value = JSON.parse(raw);
  } catch {
    return null;
  }
  if (typeof value !== "object" || value === null) {
    return null;
  }
  const obj = value as Record<string, unknown>;
  if (typeof obj.type !== "string") {
    return null;
  }
  if (obj.type === "state" && typeof obj.fen === "string") {
    return { type: "state", fen: obj.fen };
  }
  if (obj.type === "engine_move" && typeof obj.uci === "string") {
    return { type: "engine_move", uci: obj.uci };
  }
  if (obj.type === "error" && typeof obj.message === "string") {
    return { type: "error", message: obj.message };
  }
  return null;
}

export function serializeClientMessage(msg: ClientMessage): string {
  return JSON.stringify(msg);
}
