import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import {
  ReconnectingClient,
  type WebSocketLike,
} from "../ws/client";

class FakeSocket implements WebSocketLike {
  readyState = 0;
  sent: string[] = [];
  closed = false;
  onopen: ((this: WebSocketLike, ev: unknown) => void) | null = null;
  onmessage: ((this: WebSocketLike, ev: { data: string }) => void) | null = null;
  onclose: ((this: WebSocketLike, ev: unknown) => void) | null = null;
  onerror: ((this: WebSocketLike, ev: unknown) => void) | null = null;

  send(data: string): void {
    this.sent.push(data);
  }

  close(): void {
    this.closed = true;
    this.fireClose();
  }

  open(): void {
    this.readyState = 1;
    this.onopen?.call(this, {});
  }

  receive(data: string): void {
    this.onmessage?.call(this, { data });
  }

  fireClose(): void {
    this.readyState = 3;
    this.onclose?.call(this, {});
  }
}

describe("ReconnectingClient", () => {
  beforeEach(() => {
    vi.useFakeTimers();
  });

  afterEach(() => {
    vi.useRealTimers();
  });

  it("invokes onOpen on first successful connection", () => {
    const sockets: FakeSocket[] = [];
    const onOpen = vi.fn();
    const client = new ReconnectingClient({
      url: "ws://localhost/ws",
      callbacks: {
        onOpen,
        onMessage: vi.fn(),
        onClose: vi.fn(),
        onReconnecting: vi.fn(),
        onReconnected: vi.fn(),
      },
      createSocket: () => {
        const s = new FakeSocket();
        sockets.push(s);
        return s;
      },
    });
    client.connect();
    sockets[0].open();
    expect(onOpen).toHaveBeenCalledTimes(1);
  });

  it("queues sends made before the socket is open", () => {
    const sockets: FakeSocket[] = [];
    const client = new ReconnectingClient({
      url: "ws://localhost/ws",
      callbacks: {
        onOpen: vi.fn(),
        onMessage: vi.fn(),
        onClose: vi.fn(),
        onReconnecting: vi.fn(),
        onReconnected: vi.fn(),
      },
      createSocket: () => {
        const s = new FakeSocket();
        sockets.push(s);
        return s;
      },
    });
    client.connect();
    client.send({ type: "user_move", uci: "e2e4" });
    expect(sockets[0].sent).toEqual([]);
    sockets[0].open();
    expect(sockets[0].sent).toEqual([
      JSON.stringify({ type: "user_move", uci: "e2e4" }),
    ]);
  });

  it("sends directly when already open", () => {
    const sockets: FakeSocket[] = [];
    const client = new ReconnectingClient({
      url: "ws://localhost/ws",
      callbacks: {
        onOpen: vi.fn(),
        onMessage: vi.fn(),
        onClose: vi.fn(),
        onReconnecting: vi.fn(),
        onReconnected: vi.fn(),
      },
      createSocket: () => {
        const s = new FakeSocket();
        sockets.push(s);
        return s;
      },
    });
    client.connect();
    sockets[0].open();
    client.send({ type: "request_engine_move" });
    expect(sockets[0].sent).toEqual([
      JSON.stringify({ type: "request_engine_move" }),
    ]);
  });

  it("uses exponential backoff capped at maxBackoffMs", () => {
    const client = new ReconnectingClient({
      url: "ws://localhost/ws",
      callbacks: {
        onOpen: vi.fn(),
        onMessage: vi.fn(),
        onClose: vi.fn(),
        onReconnecting: vi.fn(),
        onReconnected: vi.fn(),
      },
      initialBackoffMs: 500,
      maxBackoffMs: 4000,
      createSocket: () => new FakeSocket(),
    });
    expect(client.backoffMs(1)).toBe(500);
    expect(client.backoffMs(2)).toBe(1000);
    expect(client.backoffMs(3)).toBe(2000);
    expect(client.backoffMs(4)).toBe(4000);
    expect(client.backoffMs(10)).toBe(4000);
  });

  it("reconnects after close and fires onReconnected on re-open", () => {
    const sockets: FakeSocket[] = [];
    const onReconnecting = vi.fn();
    const onReconnected = vi.fn();
    const onClose = vi.fn();
    const client = new ReconnectingClient({
      url: "ws://localhost/ws",
      callbacks: {
        onOpen: vi.fn(),
        onMessage: vi.fn(),
        onClose,
        onReconnecting,
        onReconnected,
      },
      initialBackoffMs: 100,
      createSocket: () => {
        const s = new FakeSocket();
        sockets.push(s);
        return s;
      },
    });
    client.connect();
    sockets[0].open();
    sockets[0].fireClose();
    expect(onClose).toHaveBeenCalledTimes(1);
    expect(onReconnecting).toHaveBeenCalledWith(1);
    vi.advanceTimersByTime(100);
    expect(sockets.length).toBe(2);
    sockets[1].open();
    expect(onReconnected).toHaveBeenCalledTimes(1);
  });

  it("close() prevents further reconnect attempts", () => {
    const sockets: FakeSocket[] = [];
    const onReconnecting = vi.fn();
    const client = new ReconnectingClient({
      url: "ws://localhost/ws",
      callbacks: {
        onOpen: vi.fn(),
        onMessage: vi.fn(),
        onClose: vi.fn(),
        onReconnecting,
        onReconnected: vi.fn(),
      },
      initialBackoffMs: 100,
      createSocket: () => {
        const s = new FakeSocket();
        sockets.push(s);
        return s;
      },
    });
    client.connect();
    sockets[0].open();
    client.close();
    vi.advanceTimersByTime(1000);
    expect(onReconnecting).not.toHaveBeenCalled();
    expect(sockets.length).toBe(1);
  });
});
