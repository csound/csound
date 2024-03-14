<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  -m0  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pvcross.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

; by Menno Knevel 2021

gilen1 filelen "flutec3.wav"    ; get length of soundfiles
gilen2  filelen "flute.aiff"    ; these files are all
gilen3  filelen "wave.wav"      ; around 2.7 seconds...

; analyze sound files and output results to pvoc-ex files
ires1 system_i 1,{{ pvanal flutec3.wav flutec3.pvx }}   ; use default settings
ires2 system_i 1,{{ pvanal flute.aiff flute.pvx }} 
ires3 system_i 1,{{ pvanal wave.wav wave.pvx }}  

instr 1 ; untreated signals
asig    diskin2   p4 , 1
print gilen1
print gilen2
print gilen3
prints  "\n---***YOU NOW HEAR THE UNTREATED SOUND SAMPLES***---\n"
    outs    asig, asig
endin

instr 2

prints  "\n---***YOU NOW HEAR THE RESULT OF THE ANALYZED FILES:***---\n"
ktime1 line 0, p3, gilen1		    ; timepointer in "flutec3.pvx" file
kcross expon     0.001, p3, 1
pvbufread ktime1, "flutec3.pvx"	    ;take only amplitude from "flute3c.pvx"
    
if p4 = 0 then
    ktime2 line 0, p3, gilen2		; timepointer in "flute.pvx"
    kfreq line .5, p3, 2			; frequency rise
    asig   pvcross	ktime2, kfreq, "flute.pvx", 1-kcross, kcross, p5 ;p5 = preserve spectral envelope
    prints  "--** preserve spectral envelope = %d **--\n", p5
else
    ktime2 line 0, p3, gilen3		; timepointer in "wave.pvx"
    kfreq line .5, p3, 2			; frequency rise
    asig   pvcross	ktime2, kfreq, "wave.pvx", 1-kcross, kcross, p5  ;p5 = preserve spectral envelope
    prints  "--** preserve spectral envelope = %d **--\n", p5
endif
    outs asig, asig

endin
</CsInstruments>
<CsScore>
s                       
i1 0 2.76   "flutec3.wav"   ; this is the file to cross with
i1 3 2.62   "flute.aiff"    ; this one, or..
i1 6 2      "wave.wav"      ; this one

s;       sample  formant    ; all slowed down to have a good listen   
i2 0 10     0       0       ; cross flutec3 with the flute, do not preserve formants
i2 10 10    1       0       ; cross flutec3 with wave, do not preserve formants
s;       sample   formant
i2 0 10     0       1       ; cross flutec3 with the flute & preserve formants
i2 10 10    1       1       ; cross flutec3 with wave & preserve formants
e
</CsScore>
</CsoundSynthesizer>
