<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o tbvcf.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

;---------------------------------------------------------
; TBVCF Test
; Coded by Hans Mikelson December, 2000
;---------------------------------------------------------
  sr =  44100   ; Sample rate
  ksmps =  10   ; Samples/Kontrol period
  nchnls =  2        ; Normal stereo
  0dbfs = 1


          instr 10

  idur	=	p3			; Duration
  iamp	=	p4			; Amplitude
  ifqc	=	cpspch(p5)		; Pitch to frequency
  ipanl	=	sqrt(p6)		; Pan left
  ipanr	=	sqrt(1-p6)		; Pan right
  iq	=	p7
  idist	=	p8
  iasym =	p9

kdclck		linseg	0, .002, 0.9, idur-.004, 0.9, .002, 0	; Declick envelope
kfco		expseg	10000, idur, 1000			; Frequency envelope
ax		vco		1, ifqc, 2, 0.5			; Square wave
ay		tbvcf		ax, kfco, iq, idist, iasym		; TB-VCF
ay		buthp	ay/1, 100				; Hi-pass

		outs		ay*iamp*ipanl*kdclck, ay*iamp*ipanr*kdclck
          endin


</CsInstruments>
<CsScore>

f1 0 65536 10 1

; TeeBee Test
;   Sta  Dur  Amp    Pitch Pan  Q    Dist1 Asym
i10 0    0.2  0.5    7.00  .5   0.0  2.0   0.0
i10 0.3  0.2  0.5    7.00  .5   0.8  2.0   0.0
i10 0.6  0.2  0.5    7.00  .5   1.6  2.0   0.0
i10 0.9  0.2  0.5    7.00  .5   1.7  2.0   0.0
i10 1.2  0.2  0.5    7.00  .5   1.8  2.0   0.0
i10 1.8  0.2  0.5    7.00  .5   0.0  2.0   0.25
i10 2.1  0.2  0.5    7.00  .5   0.8  2.0   0.25
i10 2.4  0.2  0.5    7.00  .5   1.6  2.0   0.25
i10 2.7  0.2  0.5    7.00  .5   1.8  2.0   0.25
i10 3.0  0.2  0.5    7.00  .5   1.9  2.0   0.25
i10 3.3  0.2  0.5    7.00  .5   2.0  2.0   0.25
i10 3.6  0.2  0.5    7.00  .5   0.0  2.0   0.5
i10 3.9  0.2  0.5    7.00  .5   0.8  2.0   0.5
i10 4.2  0.2  0.5    7.00  .5   1.6  2.0   0.5
i10 4.5  0.2  0.5    7.00  .5   1.8  2.0   0.5
i10 4.8  0.2  0.5    7.00  .5   1.9  2.0   0.5
i10 5.1  0.2  0.5    7.00  .5   2.0  2.0   0.5
i10 5.4  0.2  0.5    7.00  .5   0.0  2.0   0.75
i10 5.7  0.2  0.5    7.00  .5   0.8  2.0   0.75
i10 6.0  0.2  0.5    7.00  .5   1.6  2.0   0.75
i10 6.3  0.2  0.5    7.00  .5   1.8  2.0   0.75
i10 6.6  0.2  0.5    7.00  .5   1.9  2.0   0.75
i10 6.9  0.2  0.5    7.00  .5   2.0  2.0   0.75
i10 7.2  0.2  0.5    7.00  .5   0.0  2.0   1.0
i10 7.5  0.2  0.5    7.00  .5   0.8  2.0   1.0
i10 7.8  0.2  0.5    7.00  .5   1.6  2.0   1.0
i10 8.1  0.2  0.5    7.00  .5   1.8  2.0   1.0
i10 8.4  0.2  0.5    7.00  .5   1.9  2.0   1.0
i10 8.7  0.2  0.5    7.00  .5   2.0  2.0   1.0
e


</CsScore>
</CsoundSynthesizer>
