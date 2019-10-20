<CsoundSynthesizer>
<CsInstruments>

; Initialize the global variables.
sr = 44100
kr = 4410
ksmps = 10
nchnls = 1

; Instrument #1
instr 1
kfreq	line p4, p3, 440
   a1 shaker 10000, kfreq, 8, 0.999, 100, 0
   out a1
endin


</CsInstruments>
<CsScore>

i 1 0 1 440
i 1 + 1 4000

e
</CsScore>
</CsoundSynthesizer>
