<CsoundSynthesizer>

<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    Silent  MIDI in
-odac           -iadc     -d       -M0  ;;;realtime I/O
</CsOptions>

<CsInstruments>
;xtratim example by Jonathan Murphy Dec. 2006
sr = 44100
ksmps = 10
nchnls = 2

	    ; sine wave for oscillators
gisin	    ftgen     1, 0, 4096, 10, 1
	    ; set volume initially to midpoint
	    ctrlinit  1, 7,64

;;; simple two oscil, two envelope synth
    instr 1

	    ; frequency
  kcps	    cpsmidib
	    ; initial velocity (noteon)
  ivel	    veloc

	    ; master volume
  kamp	    ctrl7     1, 7, 0, 127
  kamp	    =  kamp * ivel

	    ; parameters for aenv1
  iatt1	    =  0.03	
  idec1	    =  1
  isus1	    =  0.25	
  irel1	    =  1
	    ; parameters for aenv2
  iatt2	    =  0.06	
  idec2	    =  2	
  isus2	    =  0.5
  irel2	    =  2

	    ; extra (release) time allocated
	    xtratim   (irel1>irel2 ? irel1 : irel2)
            ; krel is used to trigger envelope release
  krel	    init      0
  krel	    release
	    ; if noteoff received, krel == 1, otherwise krel == 0
if (krel == 1) kgoto rel

	    ; attack, decay, sustain segments
  atmp1	    linseg    0, iatt1, 1, idec1, isus1	, 1, isus1
  atmp2	    linseg    0, iatt2, 1, idec2, isus2	, 1, isus2
  aenv1	    =  atmp1
  aenv2	    =  atmp2
	    kgoto     done

	    ; release segment
rel:
  atmp3	    linseg    1, irel1, 0, 1, 0
  atmp4	    linseg    1, irel2, 0, 1, 0
  aenv1	    =  atmp1 * atmp3  ;to go from the current value (in case
  aenv2	    =  atmp2 * atmp4  ;the attack hasn't finished) to the release.

	    ; control oscillator amplitude using envelopes
done:
  aosc1	    oscil     aenv1, kcps, gisin
  aosc2	    oscil     aenv2, kcps * 1.5, gisin
  aosc1	    =  aosc1 * kamp
  aosc2	    =  aosc2 * kamp

	    ; send aosc1 to left channel, aosc2 to right,
	    ; release times are noticably different
 
	    outs      aosc1, aosc2

    endin

</CsInstruments>

<CsScore>

f 0 3600 ;dummy table to wait for realtime MIDI events

</CsScore>

</CsoundSynthesizer>
