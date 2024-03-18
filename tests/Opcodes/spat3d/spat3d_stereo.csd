<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o spat3d_stereo.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

/* Written by Istvan Varga */
sr      =  48000
kr      =  1000
ksmps   =  48
nchnls  =  2

/* room parameters */

idep    =  3    /* early reflection depth       */

itmp    ftgen   1, 0, 64, -2,                                           \
		/* depth1, depth2, max delay, IR length, idist, seed */ \
		idep, 48, -1, 0.01, 0.25, 123,                          \
		1, 21.982, 0.05, 0.87, 4000.0, 0.6, 0.7, 2, /* ceil  */ \
		1,  1.753, 0.05, 0.87, 3500.0, 0.5, 0.7, 2, /* floor */ \
		1, 15.220, 0.05, 0.87, 5000.0, 0.8, 0.7, 2, /* front */ \
		1,  9.317, 0.05, 0.87, 5000.0, 0.8, 0.7, 2, /* back  */ \
		1, 17.545, 0.05, 0.87, 5000.0, 0.8, 0.7, 2, /* right */ \
		1, 12.156, 0.05, 0.87, 5000.0, 0.8, 0.7, 2  /* left  */

	instr 1

/* some source signal */

a1      phasor 150              ; oscillator
a1      butterbp a1, 500, 200   ; filter
a1      =  taninv(a1 * 100)
a2      phasor 3                ; envelope
a2      mirror 40*a2, -100, 5
a2      limit a2, 0, 1
a1      =  a1 * a2 * 9000

kazim   line 0, 2.5, 360        ; move sound source around
kdist   line 1, 10, 4           ; distance

; convert polar coordinates
kX      =  sin(kazim * 3.14159 / 180) * kdist
kY      =  cos(kazim * 3.14159 / 180) * kdist
kZ      =  0

a1      =  a1 + 0.000001 * 0.000001     ; avoid underflows

imode   =  1    ; change this to 3 for 8 spk in a cube,
		; or 1 for simple stereo

aW, aX, aY, aZ  spat3d a1, kX, kY, kZ, 1.0, 1, imode, 2, 2

aW      =  aW * 1.4142

; stereo
;
aL     =  aW + aY              /* left                 */
aR     =  aW - aY              /* right                */

; quad (square)
;
;aFL     =  aW + aX + aY         /* front left           */
;aFR     =  aW + aX - aY         /* front right          */
;aRL     =  aW - aX + aY         /* rear left            */
;aRR     =  aW - aX - aY         /* rear right           */

; eight channels (cube)
;
;aUFL   =  aW + aX + aY + aZ    /* upper front left     */
;aUFR   =  aW + aX - aY + aZ    /* upper front right    */
;aURL   =  aW - aX + aY + aZ    /* upper rear left      */
;aURR   =  aW - aX - aY + aZ    /* upper rear right     */
;aLFL   =  aW + aX + aY - aZ    /* lower front left     */
;aLFR   =  aW + aX - aY - aZ    /* lower front right    */
;aLRL   =  aW - aX + aY - aZ    /* lower rear left      */
;aLRR   =  aW - aX - aY - aZ    /* lower rear right     */

	outs aL, aR

	endin


</CsInstruments>
<CsScore>

/* Written by Istvan Varga */
i 1 0 10
e


</CsScore>
</CsoundSynthesizer>
