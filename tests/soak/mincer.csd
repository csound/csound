<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o mincer.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

idur  = p3
ilock = p4
ipitch = 1
itimescale = 0.5
iamp  = 0.8

atime line   0,idur,idur*itimescale
asig  mincer atime, iamp, ipitch, 1, ilock
      outs asig, asig

endin
</CsInstruments>
<CsScore>
f 1 0 0 1 "fox.wav" 0 0 0

i 1 0 5 0	;not locked
i 1 6 5 1	;locked

e

</CsScore>
</CsoundSynthesizer>
