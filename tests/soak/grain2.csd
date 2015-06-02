<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o grain2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr	=  48000
kr	=  750
ksmps	=  64
nchnls	=  2

/* square wave */
i_	ftgen 1, 0, 4096, 7, 1, 2048, 1, 0, -1, 2048, -1
/* window */
i_	ftgen 2, 0, 16384, 7, 0, 4096, 1, 4096, 0.3333, 8192, 0
/* sine wave */
i_	ftgen 3, 0, 1024, 10, 1
/* room parameters */
i_	ftgen 7, 0, 64, -2, 4, 50, -1, -1, -1, 11,			\
			    1, 26.833, 0.05, 0.85, 10000, 0.8, 0.5, 2,	\
			    1,  1.753, 0.05, 0.85,  5000, 0.8, 0.5, 2,	\
			    1, 39.451, 0.05, 0.85,  7000, 0.8, 0.5, 2,	\
			    1, 33.503, 0.05, 0.85,  7000, 0.8, 0.5, 2,	\
			    1, 36.151, 0.05, 0.85,  7000, 0.8, 0.5, 2,	\
			    1, 29.633, 0.05, 0.85,  7000, 0.8, 0.5, 2

ga01	init 0

/* generate bandlimited square waves */

i0	=  0
loop1:
imaxh	=  sr / (2 * 440.0 * exp(log(2.0) * (i0 - 69) / 12))
i_	ftgen i0 + 256, 0, 4096, -30, 1, 1, imaxh
i0	=  i0 + 1
	if (i0 < 127.5) igoto loop1

	instr 1

p3	=  p3 + 0.2

/* note velocity */
iamp	=  0.0039 + p5 * p5 / 16192
/* vibrato */
kcps	oscili 1, 8, 3
kenv	linseg 0, 0.05, 0, 0.1, 1, 1, 1
/* frequency */
kcps	=  (kcps * kenv * 0.01 + 1) * 440 * exp(log(2) * (p4 - 69) / 12)
/* grain ftable */
kfn	=  int(256 + 69 + 0.5 + 12 * log(kcps / 440) / log(2))
/* grain duration */
kgdur	port 100, 0.1, 20
kgdur	=  kgdur / kcps

a1	grain2 kcps, kcps * 0.02, kgdur, 50, kfn, 2, -0.5, 22, 2
a1	butterlp a1, 3000
a2	grain2 kcps, kcps * 0.02, 4 / kcps, 50, kfn, 2, -0.5, 23, 2
a2	butterbp a2, 12000, 8000
a2	butterbp a2, 12000, 8000
aenv1	linseg 0, 0.01, 1, 1, 1
aenv2	linseg 3, 0.05, 1, 1, 1
aenv3	linseg 1, p3 - 0.2, 1, 0.07, 0, 1, 0

a1	=  aenv1 * aenv3 * (a1 + a2 * 0.7 * aenv2)

ga01	=  ga01 + a1 * 10000 * iamp

	endin

/* output instr */

	instr 81

i1	=  0.000001
aLl, aLh, aRl, aRh	spat3di ga01 + i1*i1*i1*i1, 3.0, 4.0, 0.0, 0.5, 7, 4
ga01	=  0
aLl	butterlp aLl, 800.0
aRl	butterlp aRl, 800.0

	outs aLl + aLh, aRl + aRh

	endin


</CsInstruments>
<CsScore>

t 0 60

i 1 0.0 1.3 60 127
i 1 2.0 1.3 67 127
i 1 4.0 1.3 64 112
i 1 4.0 1.3 72 112

i 81 0 6.4

e


</CsScore>
</CsoundSynthesizer>
