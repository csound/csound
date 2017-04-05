<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o nlfilt.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1	;unfiltered noise

asig rand .7
     outs asig, asig

endin

instr 2	;filtered noise

ka = p4
kb = p5
kd = p6
kC = p7
kL = p8
asig  rand .3
afilt nlfilt2 asig, ka, kb, kd, kC, kL
      outs    afilt, afilt
endin
</CsInstruments>
<CsScore>

i 1 0 2				; unfiltered

;        a    b    d    C    L
i 2 2 2  0    0   0.8  0.5  20	; non-linear effect
i 2 + 2 .4   0.2  0.7  0.11 200	; low=pass with non-linear
i 2 + 2 0.35 -0.3 0.95 0.1  200	; high-pass with non-linear
i 2 + 2 0.7 -0.2  0.9  0.2  20 	; high-pass with non-linear

e
</CsScore>
</CsoundSynthesizer>
