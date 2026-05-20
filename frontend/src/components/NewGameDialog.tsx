import { useState } from "react";
import type { Color } from "../ws/reducer";

export type NewGameChoice = {
  engine: string;
  humanColor: Color;
  startingFen?: string;
};

export type NewGameDialogProps = {
  open: boolean;
  defaultEngine?: string;
  engines?: string[];
  onCancel: () => void;
  onStart: (choice: NewGameChoice) => void;
};

type ColorPick = "white" | "black" | "random";

export default function NewGameDialog({
  open,
  defaultEngine = "random",
  engines = ["random", "placeholder"],
  onCancel,
  onStart,
}: NewGameDialogProps) {
  const [colorPick, setColorPick] = useState<ColorPick>("white");
  const [engine, setEngine] = useState(defaultEngine);
  const [fen, setFen] = useState("");

  if (!open) {
    return null;
  }

  const handleStart = () => {
    const resolvedColor: Color =
      colorPick === "random"
        ? Math.random() < 0.5
          ? "white"
          : "black"
        : colorPick;
    const trimmed = fen.trim();
    onStart({
      engine,
      humanColor: resolvedColor,
      startingFen: trimmed === "" ? undefined : trimmed,
    });
  };

  return (
    <div
      className="fixed inset-0 z-40 flex items-center justify-center bg-black/40"
      role="dialog"
      aria-modal="true"
    >
      <div className="bg-white rounded-lg shadow-lg px-6 py-5 max-w-md w-full mx-4 flex flex-col gap-4">
        <h2 className="text-lg font-semibold text-slate-800">New game</h2>

        <div className="flex flex-col gap-2">
          <span className="text-sm font-medium text-slate-700">Play as</span>
          <div className="flex gap-3 text-sm">
            {(["white", "black", "random"] as const).map((opt) => (
              <label
                key={opt}
                className="flex items-center gap-1.5 cursor-pointer"
              >
                <input
                  type="radio"
                  name="color"
                  value={opt}
                  checked={colorPick === opt}
                  onChange={() => setColorPick(opt)}
                />
                <span className="capitalize">{opt}</span>
              </label>
            ))}
          </div>
        </div>

        <div className="flex flex-col gap-2">
          <label
            htmlFor="engine-select"
            className="text-sm font-medium text-slate-700"
          >
            Engine
          </label>
          <select
            id="engine-select"
            value={engine}
            onChange={(e) => setEngine(e.target.value)}
            className="border border-slate-300 rounded-md px-2 py-1.5 text-sm bg-white"
          >
            {engines.map((name) => (
              <option key={name} value={name}>
                {name}
              </option>
            ))}
          </select>
        </div>

        <div className="flex flex-col gap-2">
          <label
            htmlFor="fen-input"
            className="text-sm font-medium text-slate-700"
          >
            Starting FEN (optional)
          </label>
          <input
            id="fen-input"
            type="text"
            placeholder="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
            value={fen}
            onChange={(e) => setFen(e.target.value)}
            className="border border-slate-300 rounded-md px-2 py-1.5 text-xs font-mono"
          />
        </div>

        <div className="flex justify-end gap-2 pt-2">
          <button
            type="button"
            onClick={onCancel}
            className="px-3 py-1.5 rounded-md text-sm text-slate-600 hover:bg-slate-100"
          >
            Cancel
          </button>
          <button
            type="button"
            onClick={handleStart}
            className="bg-emerald-600 hover:bg-emerald-700 text-white font-semibold px-4 py-1.5 rounded-md text-sm"
          >
            Start
          </button>
        </div>
      </div>
    </div>
  );
}
