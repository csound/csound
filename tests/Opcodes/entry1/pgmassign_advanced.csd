<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   -F   pgmassign_advanced.mid  ;;;realtime audio out with midifile in
;-iadc    ;;;uncomment -iadc if real audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pgmassign_advanced.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

	massign 1, 1	; channels 1 to 4 use instr 1 by default
	massign 2, 1
	massign 3, 1
	massign 4, 1

; pgmassign_advanced.mid can be found in /manual/examples
; pgmassign.mid has 4 notes with these parameters:
;
;	    Start time	Channel	Program
;
; note 1	0.5	   1	  10
; note 2	1.5	   2	  11
; note 3	2.5	   3	  12
; note 4	3.5	   4	  13

	pgmassign 0, 0		; disable program changes
	pgmassign 11, 3		; program 11 uses instr 3
	pgmassign 12, 2		; program 12 uses instr 2

; waveforms for instruments
itmp	ftgen 1, 0, 1024, 10, 1
itmp	ftgen 2, 0, 1024, 10, 1, 0.5, 0.3333, 0.25, 0.2, 0.1667, 0.1429, 0.125
itmp	ftgen 3, 0, 1024, 10, 1, 0, 0.3333, 0, 0.2, 0, 0.1429, 0, 0.10101

	instr 1		/* sine */

kcps	cpsmidib 2	; note frequency
asnd	oscili .6, kcps, 1
	outs asnd, asnd

	endin

	instr 2		/* band-limited sawtooth */

kcps	cpsmidib 2	; note frequency
asnd	oscili .6, kcps, 2
	outs asnd, asnd

	endin

	instr 3		/* band-limited square */

kcps	cpsmidib 2	; note frequency
asnd	oscili .6, kcps, 3
	outs asnd, asnd

	endin


</CsInstruments>
<CsScore>

t 0 120
f 0 8.5 2 -2 0
e


</CsScore>
</CsoundSynthesizer>
