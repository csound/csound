<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-n      ; no sound   
; For Non-realtime ouput leave only the line below:
; -o rnd31_krate.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1 
  
k1 rnd31 1, 0, 10           ; Create random numbers at k-rate in the range -1 to 1 
printks "k1=%f\\n", 0.1, k1 ; with a uniform distribution, seed=10.

endin

</CsInstruments>
<CsScore>

i 1 0 .3
e
</CsScore>
</CsoundSynthesizer>
