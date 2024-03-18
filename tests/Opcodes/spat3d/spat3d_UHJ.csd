<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o spat3d_UHJ.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

/* Written by Istvan Varga */
sr	=  48000
kr	=  750
ksmps	=  64
nchnls	=  2

itmp    ftgen   1, 0, 64, -2,                                           \
		/* depth1, depth2, max delay, IR length, idist, seed */ \
		3, 48, -1, 0.01, 0.25, 123,				\
		1, 21.982, 0.05, 0.87, 4000.0, 0.6, 0.7, 2, /* ceil  */ \
		1,  1.753, 0.05, 0.87, 3500.0, 0.5, 0.7, 2, /* floor */ \
		1, 15.220, 0.05, 0.87, 5000.0, 0.8, 0.7, 2, /* front */ \
		1,  9.317, 0.05, 0.87, 5000.0, 0.8, 0.7, 2, /* back  */ \
		1, 17.545, 0.05, 0.87, 5000.0, 0.8, 0.7, 2, /* right */ \
		1, 12.156, 0.05, 0.87, 5000.0, 0.8, 0.7, 2  /* left  */

	instr 1

p3	=  p3 + 1.0

kazim	line 0.0, 4.0, 360.0		; azimuth
kelev	line 40, p3 - 1.0, -20		; elevation
kdist	=  2.0				; distance
; convert coordinates
kX	=  kdist * cos(kelev * 0.01745329) * sin(kazim * 0.01745329)
kY	=  kdist * cos(kelev * 0.01745329) * cos(kazim * 0.01745329)
kZ	=  kdist * sin(kelev * 0.01745329)

; source signal
a1	phasor 160.0
a2	delay1 a1
a1	=  a1 - a2
kffrq1	port 200.0, 0.8, 12000.0
affrq	upsamp kffrq1
affrq	pareq affrq, 5.0, 0.0, 1.0, 2
kffrq	downsamp affrq
aenv4	phasor 3.0
aenv4	limit 2.0 - aenv4 * 8.0, 0.0, 1.0
a1	butterbp a1 * aenv4, kffrq, 160.0
aenv	linseg 1.0, p3 - 1.0, 1.0, 0.04, 0.0, 1.0, 0.0
a_	=  4000000 * a1 * aenv + 0.00000001

; spatialize
a_W, a_X, a_Y, a_Z	spat3d a_, kX, kY, kZ, 1.0, 1, 2, 2.0, 2

; convert to UHJ format (stereo)
aWre, aWim	hilbert a_W
aXre, aXim	hilbert a_X
aYre, aYim	hilbert a_Y

aWXre	=  0.0928*aXre + 0.4699*aWre
aWXim	=  0.2550*aXim - 0.1710*aWim

aL	=  aWXre + aWXim + 0.3277*aYre
aR	=  aWXre - aWXim - 0.3277*aYre

	outs aL, aR

	endin


</CsInstruments>
<CsScore>

/* Written by Istvan Varga */
t 0 60

i 1 0.0 8.0
e


</CsScore>
</CsoundSynthesizer>
