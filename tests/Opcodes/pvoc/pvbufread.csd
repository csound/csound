<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  -m0  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pvbufread.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

; by Menno Knevel 2021

gilen1  filelen "flute.aiff"    ; get length of soundfiles
gilen2  filelen "drumsMlp.wav"

; analyze sound files and output results to pvoc-ex files
ires1 system_i 1,{{ pvanal flute.aiff flute.pvx }} ; use default settings
ires2 system_i 1,{{ pvanal drumsMlp.wav beats.pvx }}  

instr 1 ; untreated signals

asig    diskin2   p4 , 1
print gilen1
print gilen2
prints  "\n---***YOU NOW HEAR THE UNTREATED SOUND SAMPLES***---\n"
    outs    asig, asig
endin

instr 2

prints  "\n---***YOU NOW HEAR THE RESULT OF THE ANALYZED FILES:***---\n"
ktime1 line  0, p3, .8		; use a part of "flute.pvx" file
ktime2 line  0, p3, 1		; use a part of "beats.pvx" file
kcross line  0, p3, 1       ; from flute to beats
       pvbufread ktime1, "flute.pvx"
asig   pvcross   ktime2, 1, "beats.pvx", 1-kcross, kcross
       outs asig, asig

endin
</CsInstruments>
<CsScore>
s
i1 0 2.62   "flute.aiff"
i1 3 2      "drumsMlp.wav"
s
i2 1 3
i2 5 10
e
</CsScore>
</CsoundSynthesizer>
