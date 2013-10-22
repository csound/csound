<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o denorm.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
; slightly simplified example from Istvan Varga 2006
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

garvb init 0

instr 1

a1  oscils 0.6, 440, 0
a2  linsegr 0, 0.005, 1, 3600, 1, 0.08, 0
a1  =  a1 * a2
    vincr garvb, a1
    outs a1, a1
endin

instr 99	;"Always on"

       denorm garvb
aL, aR reverbsc garvb * 0.5, garvb * 0.5, 0.92, 10000
       clear garvb
       outs aL, aR
endin

</CsInstruments>
<CsScore>

i 99 0 -1	;held by a negative p3, means "always on" 
i 1 0 0.5
i 1 4 0.5
e 8		;8 extra seconds after the performance

</CsScore>
</CsoundSynthesizer>
