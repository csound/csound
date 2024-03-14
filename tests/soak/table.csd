<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac    ;;;realtime audio out
; For Non-realtime ouput leave only the line below:
; -o table.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; by Kevin Conder

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
  
kndx line 0, p3, 1                  ; Vary our index linearly from 0 to 1.

ifn = 1                             ; Read Table #1 with our index.
ixmode = 1
kfreq table kndx, ifn, ixmode
a1 oscil .5, kfreq, 2; Generate a sine waveform, use our table values to vary its frequency.
outs a1, a1
endin

</CsInstruments>
<CsScore>
f 1 0 1025 -7 200 1024 2000 ; Table #1, a line from 200 to 2,000.
f 2 0 16384 10 1            ; Table #2, a sine wave.

i 1 0 2
e
</CsScore>
</CsoundSynthesizer>
