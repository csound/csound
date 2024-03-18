<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   -m0  --limiter=.95 ;;;realtime audio out, with limiter protection
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pvread.wav -W ;;; for file output any platform
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
ibin  = p4
ktime line 0, p3, 2.8
kfreq, kamp pvread ktime, "fox1.pvx", ibin	;read data from 7th analysis bin.
asig  poscil kamp, kfreq
      outs asig*5, asig*5			        ;compensate loss of volume

endin
</CsInstruments>
<CsScore>

i1 0 2.76           ; original sample
s
i2 0 2.76   7       ; 3 different bins
i2 + 2.76   15
i2 + 2.76   25
s
i2 2 6      7       ; slow chord
i2 2 6      50
i2 2 2      75
e
</CsScore>
</CsoundSynthesizer>
