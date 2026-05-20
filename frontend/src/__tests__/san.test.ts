import { describe, expect, it } from "vitest";
import { groupSansIntoPairs, uciListToSans } from "../game/san";

describe("uciListToSans", () => {
  it("converts the Italian opening to SAN", () => {
    const sans = uciListToSans([
      "e2e4",
      "e7e5",
      "g1f3",
      "b8c6",
      "f1c4",
    ]);
    expect(sans).toEqual(["e4", "e5", "Nf3", "Nc6", "Bc4"]);
  });

  it("formats castling correctly", () => {
    const sans = uciListToSans([
      "e2e4",
      "e7e5",
      "g1f3",
      "b8c6",
      "f1c4",
      "f8c5",
      "e1g1",
    ]);
    expect(sans[6]).toBe("O-O");
  });

  it("formats a queen promotion with check", () => {
    const sans = uciListToSans(
      ["g7g8q"],
      "8/6P1/8/8/8/8/k7/4K3 w - - 0 1"
    );
    expect(sans[0]).toBe("g8=Q+");
  });

  it("throws on illegal moves in the history", () => {
    expect(() => uciListToSans(["e2e5"])).toThrow();
  });
});

describe("groupSansIntoPairs", () => {
  it("pairs white and black moves with move numbers", () => {
    const pairs = groupSansIntoPairs(["e4", "e5", "Nf3", "Nc6", "Bc4"]);
    expect(pairs).toEqual([
      { index: 1, white: "e4", black: "e5" },
      { index: 2, white: "Nf3", black: "Nc6" },
      { index: 3, white: "Bc4", black: undefined },
    ]);
  });

  it("handles single-side lists", () => {
    expect(groupSansIntoPairs(["e4"])).toEqual([
      { index: 1, white: "e4", black: undefined },
    ]);
    expect(groupSansIntoPairs([])).toEqual([]);
  });

  it("handles starting from black's move", () => {
    const pairs = groupSansIntoPairs(["e5", "Nf3"], 1, "black");
    expect(pairs).toEqual([
      { index: 1, white: "...", black: "e5" },
      { index: 2, white: "Nf3", black: undefined },
    ]);
  });
});
