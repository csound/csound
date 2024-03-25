<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac -Ma --midi-key=4 --midi-velocity-amp=5 -m0  ;;;realtime audio out and midi in
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o %.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

giScale ftgen 1, 0, -12, -2, 0, -34, -7, 10, -14, 3, -21, -3, -27, -10, 7, -17	;12 note scale with detuned keys

instr 1

ikey	= p4
ivel	= p5
indx	= ikey % 12								;work on the twelftone scale
icent	tab_i indx, giScale							;load the scale
ifreqeq	= cpsmidinn(ikey)
ifreq	= ifreqeq * cent(icent)							;change frequency by cents from table
prints	"Key %d modulus 12 =  %d. ", ikey, indx
prints	"Equal-tempered frequency of this key  = %f,", ifreqeq
prints  " but here with cent deviation %d = %f%n", icent, ifreq
asig	vco2 ivel*.5, ifreq
	outs asig, asig

endin
</CsInstruments>
<CsScore>
f 0 60		;run for 60 seconds

e
</CsScore>
</CsoundSynthesizer>

