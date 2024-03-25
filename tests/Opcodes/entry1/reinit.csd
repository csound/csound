<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac      ;;;RT audio out
; For Non-realtime ouput leave only the line below:
; -o reinit.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

reset:
        timout 0, p3/p4, contin         
        reinit reset

contin:
        kLine expon 440, p3/p4, 880
        aSig poscil 1, kLine
        outs aSig, aSig
        rireturn

endin

</CsInstruments>
<CsScore>

i1 0 10 10
i1 + 10 50      
e
</CsScore>
</CsoundSynthesizer>
