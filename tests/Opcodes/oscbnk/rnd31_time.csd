<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-n      ; no sound
; For Non-realtime ouput leave only the line below:
; -o rnd31_time.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1


instr 1
  
i1 rnd31 1, 0.5, 0      ; i-rate random numbers with linear distribution,
i2 rnd31 1, 0.5         ; seeding from the current time. (Note that the seed
i3 rnd31 1, 0.5         ; was used only in the first call.)

print i1
print i2
print i3

endin

</CsInstruments>
<CsScore>

i 1 0 0
e
</CsScore>
</CsoundSynthesizer>
