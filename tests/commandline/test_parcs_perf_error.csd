<CsoundSynthesizer>
<CsOptions>
-n -d -m0 -j 4
</CsOptions>
<CsInstruments>
/* Csound-test
{
  "description": "PARCS exits on perf error",
  "expect": {
    "exit": "nonzero",
    "stderr": [
      "Array index 2 out of range (0,0) for dimension 1"
    ]
  },
  "profiles": {
    "wasm": {
      "skip": "PARCS requires native worker threads."
    }
  }
}
*/
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

instr 1
  kvalues[] init 1
  kindex init 2
  kvalue = kvalues[kindex]
  kcycle timeinstk
  printks "after-error cycle=%d value=%f\n", 0, kcycle, kvalue
endin


</CsInstruments>
<CsScore>
i 1 0 0.01
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>