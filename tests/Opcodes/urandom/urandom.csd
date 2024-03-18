<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac      ;;;RT audio out
; For Non-realtime ouput leave only the line below:
; -o urandom.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1     ; Create random numbers at a-rate in the range -2 to 2
  
aur     urandom  -2, 2
afreq1  =   aur * 500 + 100         ; Use the random numbers to choose a frequency.
afreq2  =   aur * 500 + 200
a1 oscil .3, afreq1
a2 oscil .3, afreq2
outs a1, a2
endin

</CsInstruments>
<CsScore>
i 1 0 3
e
</CsScore>
</CsoundSynthesizer>
