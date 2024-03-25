<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  -+rtmidi=virtual -M0  ;;;realtime audio out and realtime midi in
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o transegr.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
 	 
icps cpsmidi	 	 
iamp ampmidi .2	 
;             st,dur1,typ1,val,dur2,typ2,end	 	 	 
kenv transegr 0,  .2,  2,  .5,  1, - 3,   0
asig pluck kenv*iamp, icps, icps, 1, 1	 
     outs asig, asig

	 
endin
</CsInstruments>
<CsScore>
f1 0 4096 10 1	;sine

f0 30	;runs 30 seconds
e
</CsScore>
</CsoundSynthesizer>
