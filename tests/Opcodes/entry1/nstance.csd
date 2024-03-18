<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac         ;;;RT audio out
; For Non-realtime ouput leave only the line below:
; -o nstance.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1


instr 1 ; Instrument #1 - oscillator with a high note.
  
  iHandle nstance 2, .3, p3     ; Play Instrument #2 0.3 seconds later
  a1 oscils .5, 880, 1          ; a high note
  outs a1 * .5, a1              ; ... a bit to the right
  print iHandle
endin


instr 2 ; Instrument #2 - oscillator with a low note.
  
  a1 oscils .5, 110, 1          ; a low note
  outs a1, a1 * .5              ; ... a bit to the left
endin


</CsInstruments>
<CsScore>
f 1 0 16384 10 1    ; a sine wave.

i 1 0 1
i 1 2 .5
e
</CsScore>
</CsoundSynthesizer>

