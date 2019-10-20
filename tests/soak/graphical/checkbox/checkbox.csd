<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 10
nchnls = 2
 
instr 1
  ; Get the value from the checkbox.
  k1 checkbox 1

  ; If the checkbox is selected then k2=440, otherwise k2=880.
  k2 = (k1 == 0 ? 440 : 880)

  a1 oscil 10000, k2, 1
  outs a1, a1
endin

</CsInstruments>
<CsScore>

; sine wave.
f 1 0 32768 10 1

i 1 0 10 
e

</CsScore>
</CsoundSynthesizer>

