# chess_engine

> **Status: work in progress.** The skeleton builds end-to-end (browser ↔ WebSocket ↔ C++ server ↔ stub engine), but no real chess logic is implemented yet — move generation, neural-net engines, and self-play training all land on future feature branches.

A webhosted chess platform with three pillars: a React + Tailwind **web client** in the browser, a single-binary C++ **server and engine** (HTTP/WS server linked against our chess engine library, with ONNX Runtime for neural-network inference and UCI subprocess support for external engines like Stockfish), and a Python **training pipeline** that trains transformer policy + value models via supervised learning and AlphaZero-style self-play, exporting checkpoints to ONNX for the C++ side to consume. The interesting work happens in the learned models — we deliberately skip hand-rolled alpha-beta search.

## Quickstart

All real work runs inside a Docker dev container; the host only needs Docker and `make`.

```bash
make image-build   # build the dev image (one-time, a few minutes)
make install       # conan install backend + npm install frontend + pip install training
make web           # build chess-server + frontend, serve on :8080
```

Open `http://localhost:8080` in a browser. You'll see a chessboard at the starting position; the WebSocket handshake with the server is visible in the browser console.

Other useful targets: `make build` (C++ only), `make test` (ctest + vitest + pytest), `make typecheck` (frontend TypeScript), `make sh` (interactive shell in the container), `make down` / `make clean` (teardown). `make help` prints the full menu.
