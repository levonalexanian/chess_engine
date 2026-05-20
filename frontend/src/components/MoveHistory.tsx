import { groupSansIntoPairs } from "../game/san";

export type MoveHistoryProps = {
  sans: string[];
  startingFullmove?: number;
  startingSide?: "white" | "black";
};

export default function MoveHistory({
  sans,
  startingFullmove = 1,
  startingSide = "white",
}: MoveHistoryProps) {
  const pairs = groupSansIntoPairs(sans, startingFullmove, startingSide);
  return (
    <div className="border border-slate-200 rounded-md bg-white max-h-96 overflow-y-auto">
      <h2 className="text-xs uppercase tracking-wider text-slate-500 px-3 py-2 border-b border-slate-200 sticky top-0 bg-white">
        Moves
      </h2>
      {pairs.length === 0 ? (
        <p className="text-sm text-slate-400 px-3 py-4">No moves yet.</p>
      ) : (
        <ol className="text-sm font-mono divide-y divide-slate-100">
          {pairs.map((pair) => (
            <li
              key={pair.index}
              className="flex items-center gap-3 px-3 py-1.5"
            >
              <span className="text-slate-400 w-6 text-right">
                {pair.index}.
              </span>
              <span className="w-16">{pair.white}</span>
              <span className="w-16 text-slate-700">{pair.black ?? ""}</span>
            </li>
          ))}
        </ol>
      )}
    </div>
  );
}
