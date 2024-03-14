<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   -m0 ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pvinterp.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

; by Menno Knevel 2021

gilen1  filelen "fox.wav"	    ; get length of impulse soundfile
gilen2  filelen "finneganswake1.flac"

; analyze sound file and output result to pvoc-ex file
ires1 system_i 1,{{ pvanal fox.wav fox1.pvx }}          ; default settings
ires2 system_i 1,{{ pvanal finneganswake1.flac finneganswake1.pvx }}          ; default settings

instr 1 ; untreated signals
asig    diskin2   p4 , 1
print gilen1
print gilen2
prints  "\n---***YOU NOW HEAR THE UNTREATED SOUND SAMPLES***---\n"
outs    asig, asig
endin

instr 2

prints  "\n---***YOU NOW HEAR THE RESULT OF THE ANALYZED FILES:***---\n"
kfox    line 0, p3, gilen1		 ; timepointer in the "fox.pvx" file
kflute  line 0, p3, gilen2		 ; & in the "finneganswake1.pvx" file
kinterp line 1, p3, 0
	    pvbufread kfox, "fox1.pvx"
asig	pvinterp kflute, 1, "finneganswake1.pvx", p4, p5, 1, 1, 1-kinterp, 1-kinterp
        outs asig, asig

endin
</CsInstruments>
<CsScore>
s
i1 0 2.76   "fox.wav"
i1 3 12.7   "finneganswake1.flac"
s 
;           freq.fox    freq.finne 
i2 0 2.76      1           1   ; use duration of the fox
i2 5 12.7      1           1   ; duration of finneganswake
s  
i2 0 2.76      .1          4   ; duration of the fox &
i2 5 12.7      .1          4   ; duration of finneganswake but different pitches
e
</CsScore>
</CsoundSynthesizer>
