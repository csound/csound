<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

; Test basic g-variables in main CSD
giValue = 42
gkValue init 100

instr 1
  if (giValue != 42) then
    prints "FAIL: Expected giValue = 42, got %d\\n", giValue
    exitnow 1
  endif
  prints "PASS: Basic global variables\\n"
  turnoff
endin

</CsInstruments>
<CsScore>
i1 0 0.1
</CsScore>
</CsoundSynthesizer>
