import Board from "../components/Board";
import MoveHistory from "../components/MoveHistory";
import StatusBar from "../components/StatusBar";
import { START_FEN } from "../game/chess";
import type { Color, ConnectionState } from "../ws/reducer";

export type PlayProps = {
  fen?: string;
  sans?: string[];
  connection?: ConnectionState;
  turn?: Color;
  inCheck?: boolean;
  humanColor?: Color;
  engineThinking?: boolean;
};

export default function Play({
  fen = START_FEN,
  sans = [],
  connection = "connecting",
  turn = "white",
  inCheck = false,
  humanColor = "white",
  engineThinking = false,
}: PlayProps) {
  return (
    <div className="grid grid-cols-1 lg:grid-cols-[minmax(0,1fr)_18rem] gap-4 w-full max-w-5xl mx-auto">
      <div className="w-full max-w-xl mx-auto lg:mx-0">
        <Board fen={fen} orientation={humanColor} draggable={false} />
      </div>
      <div className="flex flex-col gap-3 w-full">
        <StatusBar
          connection={connection}
          turn={turn}
          inCheck={inCheck}
          humanColor={humanColor}
          engineThinking={engineThinking}
          gameOver={false}
        />
        <MoveHistory sans={sans} />
      </div>
    </div>
  );
}
