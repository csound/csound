<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   -m0  --limiter=.95 ;;;realtime audio out, with limiter protection
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pvadd.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

; by Menno Knevel 2021

gilen  filelen "fox.wav"	    ; get length of impulse soundfile

; analyze sound file and output result to pvoc-ex file
ires system_i 1,{{ pvanal fox.wav fox1.pvx }}          ; default settings

instr 1 ; untreated signal
asig    diskin2   "fox.wav", 1
prints  "\n---***YOU NOW HEAR THE UNTREATED SOUND SAMPLE***---\n"
outs    asig, asig
endin

instr 2
prints  "\n---***YOU NOW HEAR THE RESULT OF THIS ANALYZED FILE:***---\n"
ibins   =  p4
igatefn =  p5
ktime line 0, p3, p3
asig  pvadd ktime, 1, "fox1.pvx", 1, ibins, 0, 1, 0, 0, igatefn
outs asig*.7, asig*.7
endin

</CsInstruments>
<CsScore>
f 1 0 16384 10 1	; sine wave
f 2 0 512 7 1 512 1 ; 3 different tables for scaling of the gate
f 3 0 512 5 1 256 .001
f 4 0 512 7 0 256 1 256 1


i1 0 2.76          ; original sample
s
;          bins   table
i2 0 2.76   300     2    ; lots of bins & table 2 scales amplitudes (= no change)  
i2 3 2.76   300     3    ; table 3 scales amplitudes
i2 6 2.76   300     4    ; table 4 scales amplitudes
s
;          bins   table
i2 0 2.76   30      2    ; a few bins & table 2 scales amplitudes (= no change)  
i2 3 2.76   30      3    ; table 3 scales amplitudes
i2 6 2.76   30      4    ; table 4 scales amplitudes
e
</CsScore>
</CsoundSynthesizer>
