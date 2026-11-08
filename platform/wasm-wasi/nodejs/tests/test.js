import Csound from "../src/index.js";

describe("@csound/nodjes", () => {
  it("starts up", () => {
    const csoundObj = Csound({});
    expect(typeof csoundObj === "object").toEqual(true);
  });
});
