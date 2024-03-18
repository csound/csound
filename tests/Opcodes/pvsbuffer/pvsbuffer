<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o pvsbuffer.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

;; example written by joachim heintz 2009
; additions by Menno Knevel 2021

opcode FileToPvsBuf, iik, Siiiii
;;writes an audio file at the first k-cycle to a fft-buffer (via pvsbuffer)
Sfile, ifftsize, ioverlap, iwinsize, iwinshape, ilength xin
ktimek		timeinstk
if ktimek == 1 then
ilen		filelen	Sfile
kcycles	=		ilen * kr; number of k-cycles to write the fft-buffer
kcount		init		0
loop:
ain		soundin	Sfile
fftin		pvsanal	ain, ifftsize, ioverlap, iwinsize, iwinshape
ibufln      =   (ilen * ilength) + (ifftsize / sr)    
ibuf, ktim	pvsbuffer	fftin, ibufln

		loop_lt	kcount, 1, kcycles, loop
		xout		ibuf, ilen, ktim
endif
endop


instr 1
ifftsize	=	1024
ioverlap	=	ifftsize / 4
iwinsize	=	ifftsize
iwinshape	=	1; von-Hann window
ilength     =   p4
ibuffer, ilen, k0	FileToPvsBuf "fox.wav", ifftsize, ioverlap, iwinsize, iwinshape, ilength
ktmpnt		linseg	ilen, p3, 0; reads the buffer backwards in p3 seconds
fread 		pvsbufread  ktmpnt, ibuffer
aout		pvsynth	fread
outs    aout, aout
endin

</CsInstruments>
<CsScore>
i 1 0 5 .05 ; buffer a small portion
i 1 6 5 .5  ; half of total length
i 1 12 5  1 ; buffer total length
e
</CsScore>
</CsoundSynthesizer>
