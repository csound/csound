<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pvsftw.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

inbins	=	512
ifn	ftgen	0,0,inbins,10,1		; make ftable
fsrc	pvsdiskin "fox.pvx", 1, 1	; read PVOCEX file
kflag	pvsftw	fsrc,ifn		; export amps to table,
kamp	init	0
if      kflag==0   kgoto contin		; only proc when frame is ready							
	tablew	kamp,1,ifn		; kill lowest bins, for obvious effect
	tablew	kamp,2,ifn
	tablew	kamp,3,ifn
	tablew	kamp,4,ifn
; read modified data back to fsrc
	pvsftr	fsrc,ifn
contin:
; and resynth
aout	pvsynth	fsrc
	outs	aout, aout

endin

</CsInstruments>
<CsScore>

i 1 0 4
e

</CsScore>
</CsoundSynthesizer>
