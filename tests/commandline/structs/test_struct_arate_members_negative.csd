<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
; initializing a-rate struct members takes a-rate arguments only:
; the auto-generated init entry has "aa" intypes and constants are
; not promoted to a-rate, so this must be a compile error.
sr = 44100
ksmps = 64
nchnls = 2
0dbfs = 1

struct AudioBus left:a, right:a

instr 1
  bus:AudioBus = init(0.0, 0.0)
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>
