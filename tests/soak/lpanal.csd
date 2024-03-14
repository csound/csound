<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o lpanal.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; by Menno Knevel 2021

ires system_i 1,{{ lpanal -p34 -h200 -P50 -Q500 -v1 finneganswake1.flac finneganswake11.lpc }}       ; filter coefficient file
ires system_i 1,{{ lpanal -a -p50 -h300 -P70 -Q100 -v1 finneganswake1.flac finneganswake12.lpc }}    ; create pole file

instr 1 ; untreated signal
asig    diskin2   "finneganswake1.flac", 1
outs    asig, asig
endin

instr 2
ilen  filelen "finneganswake1.flac"	    ; get length of soundfile
prints "\nfinneganswake1.flac = %f seconds\\n",ilen
prints  "\n---***YOU NOW HEAR THE RESULT OF THIS ANALYZED FILE:***---\n"
ktime line 0, p3, ilen-0.1                  ; timepointer
krmsr,krmso,kerr,kcps lpread ktime, p4
krmso *=.00005			                ; lower volume
aout  foscil krmso, kcps, 1, 10, 1
aout  butlp aout, 5000                  ; filter low pass
krmsr *=.0003			                ; lower volume
asig  rand krmsr                        ; use residual for the noise 
      outs aout+asig, aout+asig	        ; add saw wave and noise

endin
</CsInstruments>
<CsScore>

s
i1 0  12.7       ; original sample
s
i2 0  12.7 "finneganswake11.lpc"	; whole sentence
i2 15 12.7 "finneganswake12.lpc"	; whole sentence, pole file
e
</CsScore>
</CsoundSynthesizer>
