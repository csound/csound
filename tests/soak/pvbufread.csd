<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pvbufread.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

instr 1
; analyze "fox.wav" and "flute.aiff" with PVANAL first
ktime1 line  0, p3, .8		; use a part of "flute.pvx" file
ktime2 line  0, p3, 1.2		; use a part of "beats.pvx" file
kcross expon .03, p3, 1
       pvbufread ktime1, "flute.pvx"
asig   pvcross   ktime2, 1, "beats.pvx", 1-kcross, kcross
       outs asig, asig

endin
</CsInstruments>
<CsScore>
i 1 0 3
i 1 + 10

e
</CsScore>
</CsoundSynthesizer>
