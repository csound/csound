<CsoundSynthesizer>
<CsOptions>
; Select audio flags here according to platform
; Audio out   Audio in
;-odac           -iadc    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
 -o modmatrix.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

	sr	=	44100
	kr	=	441
	ksmps	=	100
	nchnls	=	2
	0dbfs	= 	1

; basic waveforms
giSine	ftgen	0, 0, 65537, 10, 1	; sine wave
giSaw 	ftgen	0, 0, 4097, 7, 1, 4096, -1	; saw (linear)
giSoftSaw ftgen	0, 0, 65537, 30, giSaw, 1, 10	; soft saw (only 10 first harmonics)

; modmatrix tables
giMaxNumParam	= 128
giMaxNumMod	= 32
giParam_In ftgen 0, 0, giMaxNumParam, 2, 0	; input parameters table
; output parameters table (parameter values with added modulators)
giParam_Out ftgen 0, 0, giMaxNumParam, 2, 0	
giModulators ftgen 0, 0, giMaxNumMod, 2, 0	 ; modulators table
; modulation scaling and routing (mod matrix) table, start with empty table
giModScale ftgen 0, 0, giMaxNumParam*giMaxNumMod, -2, 0		

;********************************************
; generate the modulator signals
;********************************************
	instr 1

; LFO1, 1.5 Hz, normalized range (0.0 to 1.0)
kLFO1	oscil	0.5, 1.5, giSine		; generate LFO signal
kLFO1	= kLFO1+0.5				; offset

; LFO2, 0.4 Hz, normalized range (0.0 to 1.0)
kLFO2	oscil	0.5, 0.4, giSine		; generate LFO signal
kLFO2	= kLFO2+0.5				; offset


; write modulators to table
	tablew	kLFO1, 0, giModulators
	tablew	kLFO2, 1, giModulators

	endin

;********************************************
; set parameter values
;********************************************
	instr 2

; Here we can set the parameter values
icps1	= p4
icps2	= p5
icutoff	= p6
	
; write parameters to table
	tableiw	icps1, 0, giParam_In
	tableiw	icps2, 1, giParam_In
	tableiw	icutoff, 2, giParam_In

	endin

;********************************************
; mod matrix edit
;********************************************
	instr 3

; Here we can write to the modmatrix table by using tablew or tableiw

iLfo1ToCps1	= p4
iLfo1ToCps2	= p5
iLfo1ToCutoff	= p6
iLfo2ToCps1	= p7
iLfo2ToCps2	= p8
iLfo2ToCutoff	= p9

	tableiw	iLfo1ToCps1, 0, giModScale
	tableiw	iLfo1ToCps2, 1, giModScale
	tableiw	iLfo1ToCutoff, 2, giModScale
	tableiw	iLfo2ToCps1, 3, giModScale
	tableiw	iLfo2ToCps2, 4, giModScale
	tableiw	iLfo2ToCutoff, 5, giModScale
	
; and set the update flag for modulator matrix 
; ***(must update to enable changes)
ktrig	init 1
	chnset	ktrig, "modulatorUpdateFlag"
ktrig	= 0

	endin

;********************************************
; mod matrix
;********************************************
	instr 4

; get the update flag
kupdate	chnget	"modulatorUpdateFlag"		

; run the mod matrix 
inum_mod	= 2
inum_parm	= 3
	modmatrix giParam_Out, giModulators, giParam_In, \
	giModScale, inum_mod, inum_parm, kupdate

; and reset the update flag
	chnset	0, "modulatorUpdateFlag"  ; reset the update flag

	endin

;********************************************
; audio generator to test values
;********************************************
	instr 5

; basic parameters
	iamp	= ampdbfs(-5)

; read modulated parameters from table
	kcps1	table	0, giParam_Out
	kcps2	table	1, giParam_Out
	kcutoff	table	2, giParam_Out

; set filter parameters
	kCF_freq1	= kcps1*kcutoff
	kCF_freq2	= kcps2*kcutoff
	kReso		= 0.7
	kDist		= 0.3

; oscillators and filters
	a1	oscili	iamp, kcps1, giSoftSaw
	a1	lpf18	a1, kCF_freq1, kReso, kDist

	a2	oscili	iamp, kcps2, giSoftSaw
	a2	lpf18	a2, kCF_freq2, kReso, kDist
	
		outs 	a1, a2

	endin

</CsInstruments>
<CsScore>

;********************************************
; set initial parameters
;	cps1	cps2	cutoff
i2 0 1	400	800	3

;********************************************
; set modmatrix values
;	lfo1ToCps1 lfo1ToCps2 lfo1ToCut lfo2ToCps1 lfo2ToCps2 lfo2ToCut
i3 0 1 	40         0          -2        -50        100        3

;********************************************
; start "always on" instruments
#define SCORELEN # 20 #	 ; set length of score

i1 0 $SCORELEN			; start modulators
i4 0 $SCORELEN			; start mod matrix
i5 0 $SCORELEN			; start audio oscillator

e	

</CsScore>
</CsoundSynthesizer>