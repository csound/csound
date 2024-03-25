<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o ampdb.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2

instr 1

idb  =  p4
iamp =  ampdb(idb)
asig	oscil iamp, 220
	print iamp
	outs  asig, asig
endin


</CsInstruments>
<CsScore>
i 1 0 1 50
i 1 + 1 90
i 1 + 1 68
i 1 + 1 80

e

</CsScore>
</CsoundSynthesizer>
