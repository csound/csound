<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o harmon.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
 
aout diskin2 "sing.wav", 1

kestfrq = p4	;different estimated frequency
kmaxvar = 0.1
imode   = 1
iminfrq = 100
iprd    = 0.02
  
asig harmon aout, kestfrq, kmaxvar, kestfrq*.5, kestfrq*4, \
            imode, iminfrq, iprd
     outs (asig + aout)*.6, (asig + aout)*.6	;mix dry&wet signal

endin
</CsInstruments>
<CsScore>

i 1 0 2.7 100
i 1 + .   200
i 1 + .   500
e
</CsScore>
</CsoundSynthesizer>
