<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o lpreson-2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1
; works with or without -a option when analyzing "fox.wav" from the manual
;both options sound a little different
instr 1

ilen   filelen "fox.wav"	;length of soundfile 1
prints "fox.wav = %f seconds\\n",ilen

ktime  line 0, p3, ilen
krmsr,krmso,kerr,kcps lpread ktime,"fox_nopoles.lpc"
asig   diskin2	"flute.aiff", 1
aout   lpreson asig
ares   balance aout, asig
       outs ares, ares

endin
</CsInstruments>
<CsScore>

i 1 0 2.8
e
</CsScore>
</CsoundSynthesizer>
