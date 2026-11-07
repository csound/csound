import type {
  Csound as Csound32,
  CsoundObj as Csound32Obj,
  libcsound as libcsound32,
  LibCsoundObj as LibCsound32Obj,
} from "./index.js";

export type * from "./index.js";

/** The opt-in wasm64 engine uses double samples and 64-bit pointers. */
export interface CsoundObj extends Omit<Csound32Obj, "tableCopyOut" | "getTable" | "getTableArgs"> {
  tableCopyOut: (tableNum: string) => Promise<Float64Array | undefined>;
  getTable: (tableNum: string) => Promise<Float64Array | undefined>;
  getTableArgs: (tableNum: string) => Promise<Float64Array | undefined>;
}

export interface LibCsoundObj extends Omit<
  LibCsound32Obj,
  "csoundTableCopyOut" | "csoundGetTable" | "csoundGetTableArgs"
> {
  csoundTableCopyOut: (csound: number, table: number) => Float64Array | undefined;
  csoundGetTable: (csound: number, table: number) => Float64Array | undefined;
  csoundGetTableArgs: (csound: number, table: number) => Float64Array | undefined;
}

declare function Csound(params?: Parameters<Csound32>[0]): Promise<CsoundObj>;
declare type Csound = typeof Csound;
declare function libcsound(params?: Parameters<typeof libcsound32>[0]): Promise<LibCsoundObj>;
export { Csound, libcsound };
export default Csound;
