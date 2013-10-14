<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pvadd.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

instr 1
; analyze "fox.wav" with PVANAL first
igatefn = p4
ktime line 0, p3, p3
asig  pvadd ktime, 1, "fox.pvx", 1, 300, 2, 2, 0, 0, igatefn
      outs asig*3, asig*3

endin
</CsInstruments>
<CsScore>
f 1 0 16384 10 1	;sine wave
f 2 0 512 5 1 256 .001
f 3 0 512 7 0 256 1 256 1

i 1 0 2.8 2 
i 1 + 2.8 3
e
</CsScore>
</CsoundSynthesizer>
