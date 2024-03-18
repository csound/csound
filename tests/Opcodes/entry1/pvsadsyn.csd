<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o pvsadsyn.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 16
nchnls = 2
0dbfs = 1

;; example written by joachim heintz 2009
; (additions by Menno Knevel 2021)
opcode FileToPvsBuf, iik, Siiii
;;writes an audio file at the first k-cycle to a fft-buffer (via pvsbuffer)
Sfile, ifftsize, ioverlap, iwinsize, iwinshape xin
ktimek		timeinstk
if ktimek == 1 then
ilen		filelen	Sfile
kcycles	=		ilen * kr; number of k-cycles to write the fft-buffer
kcount		init		0
loop:
ain		soundin	Sfile
fftin		pvsanal	ain, ifftsize, ioverlap, iwinsize, iwinshape
ibuf, ktim	pvsbuffer	fftin, ilen + (ifftsize / sr)
		loop_lt	kcount, 1, kcycles, loop
		xout		ibuf, ilen, ktim
endif
endop


instr 1
istretch	=		p4; time stretching factor
ifftsize	=		1024
ioverlap	=		ifftsize / 4
iwinsize	=		ifftsize
iwinshape	=		1; von-Hann window
ibuffer, ilen, k0		FileToPvsBuf	"fox.wav", ifftsize, ioverlap, iwinsize, iwinshape
p3		=		istretch * ilen; set p3 to the correct value
prints  "--**new duration of note = %f seconds**--\n", p3
ktmpnt		linseg		0, p3, ilen; time pointer
fread 		pvsbufread  	ktmpnt, ibuffer; read the buffer
aout		pvsadsyn	fread, 50, 1, p5; resynthesis of 50 bins, with offset p5
aout        *=  .7      ; scale amplitude
		outs		aout, aout
endin

</CsInstruments>
<CsScore>
;         stretch   offset  
i 1  0 1    10        0
i 1 28 1    5        30
e
</CsScore>
</CsoundSynthesizer>
