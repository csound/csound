<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac --limiter=0.9 ;;;realtime audio out and limiter
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o lpslot.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; by Menno Knevel 2021

ires1 system_i 1,{{ lpanal -a -p40 -h200 -P50  -Q5000 -v1 fox.wav fox.lpc }}
ires2 system_i 1,{{ lpanal -a -p40 -h200 -P50  -Q5000 -v1 stereoJungle.wav stereoJungle.lpc }}

instr 1 ; lpinterp works only with poles files.

isine  ftgen 1, 0, 1024, 10, 1     ; sine wave
ilen1  filelen "fox.wav"            ; get length of soundfiles
ilen2  filelen "stereoJungle.wav"  
prints "fox.wav = %f seconds and stereoJungle.wav = %f seconds\\n",ilen1, ilen2
asrc   buzz .5, 440, 20, 1
  
ktime  line 0, p3, p3              ; from start to end for fox.wav
       lpslot 0                    ; Read data poles
krmsr, krmso, kerr, kcps lpread ktime,"fox.lpc"                     
       lpslot 1                    ; Read data poles
krmsr, krmso, kerr, kcps lpread ktime,"stereoJungle.lpc"
kmix   line 0, p3, 1               ; mixing the 2 lpc files
       lpinterp 1, 0, kmix         ; move from jungle (slot 1) to fox (slot 0)
ares   lpreson asrc
aout   balance ares, asrc
       outs aout, aout

endin

</CsInstruments>
<CsScore>
s 
i1 0 2.8        	
i1 5 6.75
i1 15 8        ; last frame freezes until note end (6.75-8 secs)

e
</CsScore>
</CsoundSynthesizer>
