<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o resonxk.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

gisin ftgen 0, 0, 2^10, 10, 1

instr 1

ksig randomh  400, 1800, 150
aout poscil   .2, 1000+ksig, gisin
     outs     aout, aout
endin

instr 2

ksig randomh  400, 1800, 150
kcf  line     1, p3, 1000		;vary high-pass
ilay = p4
ksig resonxk  ksig, kcf, 100, ilay
aout poscil   .2, 1000+ksig, gisin
asig interp   ksig			;convert k-rate to a-rate
aout balance  asig, aout		;avoid getting asig out of range
     outs     aout, aout
endin

</CsInstruments>
<CsScore>
i 1 0  5
i 2 6  5 1	;number of filter stack = 1
i 2 12 5 5	;number of filter stack = 5
e
</CsScore>
</CsoundSynthesizer>
