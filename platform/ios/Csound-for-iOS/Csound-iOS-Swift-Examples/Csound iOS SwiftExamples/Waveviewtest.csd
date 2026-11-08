<CsoundSynthesizer>
<CsOptions>
-o dac
-d
-i adc
</CsOptions>
<CsInstruments>

sr        = 44100
ksmps     = 64
nchnls    = 2
0dbfs	  = 1

instr 1
endin

</CsInstruments>
<CsScore>

f1 0 16384 10 1
f2 0 1025 25 0 0.01 200 1 400 1 513 0.01
f3 0 9 2 0 2 10 25 15 12
f4 0 513 6 1 128 -1 128 1 64 -.5 64 .5 16 -.5 8 1 16 -.5 8 1 16 -.5 84 1 16 -.5 8 .1 16 -.1 17 0
f5 0 4096 -27 0 -1 1024 1 2048 -1 3072 1 4096 -1

i1 0 360000

 
</CsScore>
</CsoundSynthesizer>
