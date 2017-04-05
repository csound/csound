<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o grain3.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr	=  48000
kr	=  1000
ksmps   =  48
nchnls	=  1

/* Bartlett window */
itmp	ftgen 1, 0, 16384, 20, 3, 1
/* sawtooth wave */
itmp	ftgen 2, 0, 16384, 7, 1, 16384, -1
/* sine */
itmp	ftgen 4, 0, 1024, 10, 1
/* window for "soft sync" with 1/32 overlap */
itmp	ftgen 5, 0, 16384, 7, 0, 256, 1, 7936, 1, 256, 0, 7936, 0
/* generate bandlimited sawtooth waves */
itmp	ftgen 3, 0, 4096, -30, 2, 1, 2048
icnt	=  0
loop01:
; 100 tables for 8 octaves from 30 Hz
ifrq	=  30 * exp(log(2) * 8 * icnt / 100)
itmp	ftgen icnt + 100, 0, 4096, -30, 3, 1, sr / (2 * ifrq)
icnt	=  icnt + 1
	if (icnt < 99.5) igoto loop01
/* convert frequency to table number */
#define FRQ2FNUM(xout'xcps'xbsfn) #

$xout	=  int(($xbsfn) + 0.5 + (100 / 8) * log(($xcps) / 30) / log(2))
$xout	limit $xout, $xbsfn, $xbsfn + 99

#

/* instr 1: pulse width modulated grains */

	instr 1

kfrq	=  523.25		; frequency
$FRQ2FNUM(kfnum'kfrq'100)	; table number
kfmd	=  kfrq * 0.02		; random variation in frequency
kgdur	=  0.2			; grain duration
kdens	=  200			; density
iseed	=  1			; random seed

kphs	oscili 0.45, 1, 4	; phase

a1	grain3	kfrq, 0, kfmd, 0.5, kgdur, kdens, 100,		\
		kfnum, 1, -0.5, 0, iseed, 2
a2	grain3	kfrq, 0.5 + kphs, kfmd, 0.5, kgdur, kdens, 100,	\
		kfnum, 1, -0.5, 0, iseed, 2

; de-click
aenv	linseg 0, 0.01, 1, p3 - 0.05, 1, 0.04, 0, 1, 0

	out aenv * 2250 * (a1 - a2)

	endin

/* instr 2: phase variation */

	instr 2

kfrq	=  220			; frequency
$FRQ2FNUM(kfnum'kfrq'100)	; table number
kgdur	=  0.2			; grain duration
kdens	=  200			; density
iseed	=  2			; random seed

kprdst	expon 0.5, p3, 0.02	; distribution

a1	grain3	kfrq, 0.5, 0, 0.5, kgdur, kdens, 100,		\
		kfnum, 1, 0, -kprdst, iseed, 64

; de-click
aenv	linseg 0, 0.01, 1, p3 - 0.05, 1, 0.04, 0, 1, 0

	out aenv * 1500 * a1

	endin

/* instr 3: "soft sync" */

	instr 3

kdens	=  130.8		; base frequency
kgdur	=  2 / kdens		; grain duration

kfrq	expon 880, p3, 220	; oscillator frequency
$FRQ2FNUM(kfnum'kfrq'100)	; table number

a1	grain3 kfrq, 0, 0, 0, kgdur, kdens, 3, kfnum, 5, 0, 0, 0, 2
a2	grain3 kfrq, 0.667, 0, 0, kgdur, kdens, 3, kfnum, 5, 0, 0, 0, 2

; de-click
aenv	linseg 0, 0.01, 1, p3 - 0.05, 1, 0.04, 0, 1, 0

	out aenv * 10000 * (a1 - a2)

	endin


</CsInstruments>
<CsScore>

t 0 60
i 1 0 3
i 2 4 3
i 3 8 3
e


</CsScore>
</CsoundSynthesizer>
