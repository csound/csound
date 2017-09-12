<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pvinterp.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

instr 1
; analyze "fox.wav" and "flute.aiff" with PVANAL first
ktime1 line 0, p3, 2.8		 ; used as index in the "fox.pvx" file
ktime2 line 0, p3, 3		 ; used as index in the "flute.pvx" file
kinterp line 1, p3, 0
	pvbufread ktime1, "fox.pvx"
asig	pvinterp ktime2,1,"flute.pvx",.9, 3, .6, 1, kinterp,1-kinterp
        outs asig, asig

endin
</CsInstruments>
<CsScore>
i 1 0 3
i 1 + 10

e
</CsScore>
</CsoundSynthesizer>
