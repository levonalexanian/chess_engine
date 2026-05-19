# chess_training

The Python training pipeline for the chess_engine project. This is the third pillar of the system, alongside the C++ backend (`../backend/`) and the React frontend (`../frontend/`). See [`../docs/ROADMAP.md`](../docs/ROADMAP.md) and [`../docs/architecture.md`](../docs/architecture.md) for the full picture.

## Intended pipeline

```
PGN games  ->  encoding  ->  model  ->  checkpoint  ->  ONNX export  ->  consumed by C++ engine
```

1. **PGN ingestion** — read master games and self-play games from PGN files into a streaming dataset.
2. **Encoding** — convert positions into the canonical tensor layout described by the `encoding_spec.yaml` shared with the C++ side (so Python and C++ always agree on byte layout).
3. **Model** — transformer policy + value head, trained either supervised on PGN data or via AlphaZero-style self-play.
4. **Checkpointing** — periodic snapshots under `training/checkpoints/` (gitignored).
5. **ONNX export** — frozen graphs the C++ engine loads via ONNX Runtime for inference.
6. **Consumption** — the C++ engine picks up `*.onnx` files placed in `backend/engine/models/` (gitignored) and serves them through the same `Engine` interface as any other engine.

Phase 0 of the project lands only this stub. Real training code arrives in Phase 4+; see [`../docs/branches.md`](../docs/branches.md) for the planned feature branches.

## Local layout

```
training/
├── pyproject.toml           # chess_training package + deps
├── chess_training/
│   └── scripts/
│       └── train.py         # CLI entry point (stub today)
└── tests/
    └── test_smoke.py
```

Future work will add `chess_training/data/`, `chess_training/models/`, `chess_training/selfplay/`, and `configs/`, matching the layout in [`../docs/architecture.md`](../docs/architecture.md).

## Usage

Everything runs inside the dev container; the host-side `Makefile` is the entry point.

```sh
make install      # pip install -e training[dev] (alongside Conan + npm)
make train -- --config configs/smoke.yaml
make test         # runs pytest training/ alongside the backend + frontend test suites
```

The console-script `chess-train` is installed by the editable install and is equivalent to `python -m chess_training.scripts.train`.
