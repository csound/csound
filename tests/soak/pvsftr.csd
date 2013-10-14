<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pvsftr.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

gifil ftgen 0, 0, 0, -1, "fox.wav", 0, 0, 1

instr 1

ifftsize = 1024				;fft size
ioverlap = 256				;overlap
knewamp  = 0				;new value for amplitudes

;create fsig stream from function table
fsrc	pvstanal 1, 1, 1, gifil, 0, 0, 0, ifftsize, ioverlap, 0
ifn	ftgen	0, 0, ifftsize/2, 2, 0	;create empty function table
kflag	pvsftw	fsrc,ifn		;export amps to table	
;overwrite the first 10 bins each time the table has been filled new
if kflag == 1 then
kndx = 0
kmaxbin = 10
loop:
tablew knewamp, kndx, ifn
loop_le kndx, 1, kmaxbin, loop
endif
	pvsftr	fsrc,ifn		;read modified data back to fsrc
aout	pvsynth	fsrc			;and resynth
	outs	aout, aout

endin
</CsInstruments>
<CsScore>
i 1 0 4
e
</CsScore>
</CsoundSynthesizer>
