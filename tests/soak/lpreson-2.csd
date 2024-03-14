<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  --limiter=0.95   ;;;realtime audio out & limit loud pops
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o lpreson-2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; by Menno Knevel - 2021
;works with or without -a option when analyzing "fox.wav" from the manual
;both options sound a little different
ires  system_i 1,{{ lpanal -a -p40 -h200 -P50  -Q15000 -v1 fox.wav fox_poles.lpc }}  ; pole filter file

instr 1

ilen   filelen "fox.wav"	;length of soundfile 1
prints "fox.wav = %f seconds\\n",ilen

ktime  line 0, p3, ilen
krmsr,krmso,kerr,kcps lpread ktime,"fox_poles.lpc"
krmso  *= .000085             ; scale amplitude
asig   diskin2	"flute.aiff", p4     ; use pitch deifferences from the flute
asig   *= krmso             ; & use the scaled back "krmso"
aout   lpreson asig
ares   balance aout, asig
       outs ares, ares

endin
</CsInstruments>
<CsScore>
s 
;             pitch
i 1 0 2.756     1
i 1 3 2.756     1.5
s
i 1 0 2.756     1      ; chord
i 1 0 2.756     .7
i 1 0 2.756     1.3
e
</CsScore>
</CsoundSynthesizer>
