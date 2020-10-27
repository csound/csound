import { Csound } from "./";

declare namespace CsoundObj {
  function getNode(): Promise<AudioContext>;
  function start(): Promise<number>;
  function reset(): Promise<number>;
}

declare function Csound(): Promise<CsoundObj | undefined>;
export { Csound };
export default Csound;
