<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o exp.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs =1

gisine    ftgen     0, 0, 2^10, 10, 1 ;table for a sine wave

instr 1 ;master instrument
koct      linseg    6, p3, 12 ; octave register straight rising from 6 to 12
kexp      linseg    0, p3/3, 5, p3/3, 5, p3/3, 0 ;exponent goes from 0 to 5 and back
kdens     =         exp(kexp) ;density is e to the power of kexp
          printks   "Generated events per second: %d\n", 1, kdens
ktrig     metro     kdens ;trigger single notes in kdens frequency
 if ktrig == 1 then
;call instr 10 for 1/kdens duration, .5 amplitude and koct register
          event     "i", 10, 0, 1/kdens, .5, koct
 endif
endin

instr 10 ;performs one tone
ioct      rnd31     1, 0 ;random deviation maximum one octave plus/minus
aenv      transeg   p4, p3, -6, 0 ;fast decaying envelope for p4 amplitude
asin      poscil    aenv, cpsoct(p5+ioct), gisine ;sine for p5 octave register plus random deviation
          outs      asin, asin
endin

</CsInstruments>
<CsScore>
i 1 0 30
e
</CsScore>
</CsoundSynthesizer>