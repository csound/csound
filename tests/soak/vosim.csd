<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o vosim.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
sr     = 44100
ksmps  = 100
nchnls = 1

;#################################################
; By Rasmus Ekman 2008

; Square each point in table #p4. This should only be run once in the performance.
instr 10

	index tableng p4
	index = index - 1  ; start from last point
loop:
	ival table index, p4
	ival = ival * ival
	tableiw ival, index, p4
	index = index - 1
	if index < 0 igoto endloop
		igoto loop
endloop: 
endin

;#################################################

; Main vosim instrument. Sweeps from a fund1/form1 to fund2/form2,
; trying for narrowest formant bandwidth (still quite wide by the looks of it)
; p4:     amp
; p5, p6: fund beg-end
; p7, p8: form beg-end
; p9:     amp decay (ignored)
; p10:    pulse count (ignored - calc internally)
; p11:    pulse length mod
; p12:    skip (for tied events)
; p13:    don't fade out (if followed by tied note)
instr 1
    kamp  init  p4
    ; freq start, end
    kfund  line  p5, p3, p6
    ; formant start, end
    kform  line  p7, p3, p8

	; Try for constant ratio burst/silence, and narrowest formant bandwidth
	kPulseCount  = (kform / kfund)  ;init p10
	; Attempt to smooth steps between format bandwidths,
	; increasing decay before we are forced to a lower pulse count
	kDecay = kPulseCount/(kform % kfund)  ; init p9
	if (kDecay * kPulseCount) > kamp then
		kDecay = kamp / kPulseCount
	endif
	kDecay = 0.3 * kDecay

	kPulseFactor init p11
	
;  ar	vosim	kamp, kFund, kForm, kDecay, kPulseCount, kPulseFactor, ifn [, iskip]
    ar1	vosim 	kamp, kfund, kform, kDecay, kPulseCount, kPulseFactor, 17, p12

    ; scale amplitude for 16-bit files, with quick fade out
    amp init 20000
    if (p13 != 0) goto nofade
	amp linseg 20000, p3-.02, 20000, .02, 0
nofade:
	out ar1 * amp
endin


</CsInstruments>
<CsScore>

f1       0  32768    9  1    1  0   ; sine wave
f17      0  32768    9  0.5  1  0   ; half sine wave
i10 0 0 17 ; init run only, square table 17

; Vosim score

; Picking some formants from the table in Csound manual

;      p4=amp  fund     form      decay pulses pulsemod [skip] nofade
; tenor a -> e
i1 0  .5  .5   280 240  650  400   .03   5      1
i1 .  .   .3   .   .    1080 1700  .03   5      .
i1 .  .   .2   .   .    2650 2600  .03   5      .
i1 .  .   .15  .   .    2900 3200  .03   5      .

; tenor a -> o
i1 0.6 .2  .5  300 210  650  400   .03   5      1      0      1
i1 .   .   .3  .   .    1080 800   .03   5      .      .      .
i1 .   .   .2  .   .    2650 2600  .03   5      .      .      .
i1 .   .   .15 .   .    2900 2800  .03   5      .      .      .
; tenor o -> aah
i1 .8  .3  .5  210 180  400  650   .03   5      1      1      1
i1 .   .   .3  .   .    800  1080  .03   5      .      .      .
i1 .   .   .2  .   .    2600 2650  .03   5      .      .      .
i1 .   .   .15 .   .    2800 2900  .03   5      .      .      .
; tenor aa -> i
i1 1.1 .2  .5  180 250  650  290   .03   5      1      1      1
i1 .   .   .3  .   .    1080 1870  .03   5      .      .      .
i1 .   .   .2  .   .    2650 2800  .03   5      .      .      .
i1 .   .   .15 .   .    2900 3250  .03   5      .      .      .
; tenor i -> u
i1 1.3 .3  .5  250 270  290  350   .03   5      1      1      0
i1 .   .   .3  .   .    1870 600   .03   5      .      .      .
i1 .   .   .2  .   .    2800 2700  .03   5      .      .      .
i1 .   .   .15 .   .    3250 2900  .03   5      .      .      .

e

</CsScore>
</CsoundSynthesizer>