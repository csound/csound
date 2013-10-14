<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o voice.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kamp  = p4
kphon = p5
asig  voice kamp, 200, kphon, 0.488, 0, 1, 1, 2
      outs asig, asig

endin
</CsInstruments>
<CsScore>
f 1 0 256 1 "impuls20.aiff" 0 0 0	;audio file for the carrier waveform
f 2 0 256 10 1				;sine wave for the vibrato waveform

;       ampl phoneme
i 1 0 2 0.8    1
i 1 + . 0.6    2
i 1 + . 1.8    3
i 1 + . 15.0   4
i 1 + . 0.05   5
i 1 + . 0.06   6
i 1 + . 0.03   7
i 1 + . 0.0002 8
i 1 + . 0.1    9
i 1 + . 0.5   10
i 1 + . 100   11
i 1 + . 0.03  12
i 1 + . 0.04  13
i 1 + . 0.04  14
i 1 + . 0.04  15
i 1 + . 0.05  16

e
</CsScore>
</CsoundSynthesizer>

