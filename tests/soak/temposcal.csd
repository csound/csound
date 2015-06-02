<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o temposcal.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ilock  =  p4
itab   =  1
ipitch =  1
iamp   =  0.8
ktime  linseg 0.3, p3/2, 0.8, p3/2, 0.3
asig   temposcal ktime, iamp, ipitch, itab, ilock
       outs asig, asig

endin
</CsInstruments>
<CsScore>
f 1 0 0 1 "fox.wav" 0 4 0

i 1 0 3.8 0	;no locking
i 1 4 3.8 1	;locking
e
</CsScore>
</CsoundSynthesizer>
