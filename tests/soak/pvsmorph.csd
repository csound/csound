<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o pvsmorph.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

;; example written by joachim heintz 2009
; additions by Menno Knevel 2021

giSine		ftgen		0, 0, 4096, 10, 1

instr 1
iampint1    = p4
iampint2	= p5
ifrqint1	= p6
ifrqint2	= p7
kampint	    linseg	iampint1, p3*.2, iampint1, p3*.6, iampint2, p3*.2, iampint2
kfrqint	    linseg	ifrqint1, p3*.2, ifrqint1, p3*.6, ifrqint2, p3*.2, ifrqint2
ifftsize	= 1024
ioverlap	= ifftsize / 4
iwinsize	= ifftsize
iwinshape	= 1                                     ; von-Hann window
Sfile1		= "fox.wav"
ain1		diskin2	Sfile1, 1, 0, 1
kfreq       line    30, p3, 500
ain2		buzz	.8, kfreq, 50, giSine
fftin1		pvsanal	ain1, ifftsize, ioverlap, iwinsize, iwinshape
fftin2		pvsanal	ain2, ifftsize, ioverlap, iwinsize, iwinshape
fmorph		pvsmorph fftin1, fftin2, kampint, kfrqint
aout		pvsynth	fmorph
outs		aout * .6, aout * .6
endin

</CsInstruments>
<CsScore>
;         iampint1  iampint2    ifrqint1    ifrqint2    
i 1 0  12.2     0         0           1           1
i 1 14 12.2     1         0           1           0
i 1 28 12.2     0         1           0           1
e
</CsScore>
</CsoundSynthesizer>
