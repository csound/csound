<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
;-o pvscfs.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

a1   diskin "fox.wav", 1 
a2   diskin "wave.wav", 1, 0, 1         ; uses noisy wave
fenv pvsanal a1, 1024, 256, 1024, 1
kcfs[],krms,kerr pvscfs fenv, p4
a3 allpole a2*krms*kerr, kcfs
a3 dcblock a3
outs a3*8, a3*8
endin

</CsInstruments>
<CsScore>
;           order
i1 0 2.7      3
i1 3 2.7      30
i1 6 2.7      70
</CsScore>
</CsoundSynthesizer>
