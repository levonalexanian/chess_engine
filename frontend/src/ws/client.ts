import type { ClientMessage } from "./messages";
import { serializeClientMessage } from "./messages";

export type ClientCallbacks = {
  onOpen: () => void;
  onMessage: (raw: string) => void;
  onClose: () => void;
  onReconnecting: (attempt: number) => void;
  onReconnected: () => void;
};

export type ClientOptions = {
  url: string;
  callbacks: ClientCallbacks;
  initialBackoffMs?: number;
  maxBackoffMs?: number;
  createSocket?: (url: string) => WebSocketLike;
};

export interface WebSocketLike {
  send(data: string): void;
  close(): void;
  readyState: number;
  onopen: ((this: WebSocketLike, ev: unknown) => void) | null;
  onmessage: ((this: WebSocketLike, ev: { data: string }) => void) | null;
  onclose: ((this: WebSocketLike, ev: unknown) => void) | null;
  onerror: ((this: WebSocketLike, ev: unknown) => void) | null;
}

const OPEN = 1;

export class ReconnectingClient {
  private socket: WebSocketLike | null = null;
  private buffer: string[] = [];
  private attempt = 0;
  private closed = false;
  private hasOpenedOnce = false;
  private retryTimer: ReturnType<typeof setTimeout> | null = null;
  private readonly url: string;
  private readonly callbacks: ClientCallbacks;
  private readonly initialBackoffMs: number;
  private readonly maxBackoffMs: number;
  private readonly createSocket: (url: string) => WebSocketLike;

  constructor(options: ClientOptions) {
    this.url = options.url;
    this.callbacks = options.callbacks;
    this.initialBackoffMs = options.initialBackoffMs ?? 500;
    this.maxBackoffMs = options.maxBackoffMs ?? 30000;
    this.createSocket =
      options.createSocket ??
      ((url: string) => new WebSocket(url) as unknown as WebSocketLike);
  }

  connect(): void {
    if (this.closed) {
      return;
    }
    const socket = this.createSocket(this.url);
    this.socket = socket;
    socket.onopen = () => {
      const wasReconnect = this.hasOpenedOnce;
      this.hasOpenedOnce = true;
      this.attempt = 0;
      this.flushBuffer();
      if (wasReconnect) {
        this.callbacks.onReconnected();
      } else {
        this.callbacks.onOpen();
      }
    };
    socket.onmessage = (ev) => {
      this.callbacks.onMessage(ev.data);
    };
    socket.onclose = () => {
      this.socket = null;
      this.callbacks.onClose();
      if (!this.closed) {
        this.scheduleRetry();
      }
    };
    socket.onerror = () => {
      // close handler will follow
    };
  }

  send(msg: ClientMessage): void {
    const payload = serializeClientMessage(msg);
    if (this.socket !== null && this.socket.readyState === OPEN) {
      this.socket.send(payload);
      return;
    }
    this.buffer.push(payload);
  }

  close(): void {
    this.closed = true;
    if (this.retryTimer !== null) {
      clearTimeout(this.retryTimer);
      this.retryTimer = null;
    }
    if (this.socket !== null) {
      this.socket.close();
      this.socket = null;
    }
  }

  backoffMs(attempt: number): number {
    const base = this.initialBackoffMs * Math.pow(2, attempt - 1);
    return Math.min(base, this.maxBackoffMs);
  }

  currentAttempt(): number {
    return this.attempt;
  }

  private scheduleRetry(): void {
    this.attempt += 1;
    const delay = this.backoffMs(this.attempt);
    this.callbacks.onReconnecting(this.attempt);
    this.retryTimer = setTimeout(() => {
      this.retryTimer = null;
      this.connect();
    }, delay);
  }

  private flushBuffer(): void {
    if (this.socket === null) {
      return;
    }
    while (this.buffer.length > 0) {
      const payload = this.buffer.shift();
      if (payload !== undefined) {
        this.socket.send(payload);
      }
    }
  }
}
