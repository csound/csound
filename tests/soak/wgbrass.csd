<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o wgbrass.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; Initialize the global variables.
sr = 44100
ksmps = 10
nchnls = 1
0dbfs = 1

; Instrument #1.
instr 1
  kamp = 0.7
  kfreq = p4
  ktens = p5
  iatt = p6
  kvibf = p7
  ifn = 1

  ; Create an amplitude envelope for the vibrato.
  kvamp line 0, p3, 0.5

  a1 wgbrass kamp, kfreq, ktens, iatt, kvibf, kvamp, ifn
  out a1
endin


</CsInstruments>
<CsScore>

; Table #1, a sine wave.
f 1 0 1024 10 1

;        freq   tens  att  vibf
i 1 0 4  440    0.4   0.1  6.137
i 1 4 4  440    0.4   0.01 0.137
i 1 8 4  880    0.4   0.1  6.137
e


</CsScore>
</CsoundSynthesizer>
