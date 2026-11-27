<CsTest>
description = "Under --realtime, rebinding an Opcode object to a longer colliding variable is a performance error, not a reallocation"
args = []
[expect]
exit = "nonzero"
stderr = ["could not rebind arguments"]
output_excludes = ["AddressSanitizer", "runtime error:"]
[profiles.wasm]
skip = "The command-line WASI runtime does not support --realtime."
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-odac -+rtaudio=null -d -m4 --realtime
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 32
nchnls = 1
0dbfs = 1

gkPerfRan init 0

opcode Echo(Sin:S):S
  Sout strcpyk Sin
  xout Sout
endop

instr 1
  ; Sa and Sb are each both the input and the output of the object.
  objects:Opcode[] = create(opcoderef("Echo"), 1)
  Sa strcpy "short"
  Sb strcpy "a long text, longer than the sixty-four bytes the input variable has from init, by far"
  Sa init objects[0], Sa
  kCycle timeinstk
  if kCycle == 1 then
    gkPerfRan = 1
    Sb perf objects[0], Sb
    printf "Sb is %s\n", 1, Sb
  endif
endin

instr 2
  if gkPerfRan != 0 then
    event "e", 0, 0
    turnoff
  endif
endin

</CsInstruments>
<CsScore>
i 1 0 10
i 2 0 10
e
</CsScore>
</CsoundSynthesizer>
