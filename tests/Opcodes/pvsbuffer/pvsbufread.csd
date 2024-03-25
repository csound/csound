<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o pvsbufread.wav -W ;;; for file output any platform
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
kcycles		= ilen * kr; number of k-cycles to write the fft-buffer
kcount		init		0
loop:
ain		soundin	Sfile
fftin		pvsanal	ain, ifftsize, ioverlap, iwinsize, iwinshape
ibufln      =   ilen + (ifftsize / sr)    
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
ibuffer, ilen, k0	FileToPvsBuf "drumsMlp.wav", ifftsize, ioverlap, iwinsize, iwinshape, ilength
ktmpnt		linseg	ilen, p3, 0							; reads the buffer backwards in p3 seconds
fread 		pvsbufread  ktmpnt, ibuffer, p4, p5			; use the filter options
aout		pvsynth	fread
outs    aout, aout
endin

</CsInstruments>
<CsScore>
;			low		high
i 1 0 5		100		1000 	; filter to this range
i 1 5 5		1000	10000 	; ...and to this range
i 1 10 5	0		0		; no filtering, equal to not using ilo and ihi at all!
e
</CsScore>
</CsoundSynthesizer>
