<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac -M0 -+rtmidi=virtual ;;; midi file input
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1	 
;change channel on virtual midi keyboard
i1 midichn
   print i1

endin
</CsInstruments>
<CsScore>
f 0 20	;runs for 20 seconds

e
</CsScore>
</CsoundSynthesizer>
