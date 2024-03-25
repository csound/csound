<CsoundSynthesizer>

<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    Silent  MIDI in
-odac           -iadc     -d       -M0  ;;;realtime I/O
</CsOptions>

<CsInstruments>
;Simple usage of the xtratim opcode
sr = 44100
ksmps = 10
nchnls = 2

; sine wave for oscillators
gisin	    ftgen     1, 0, 4096, 10, 1

instr 1

  inum notnum
  icps cpsmidi
  iamp ampmidi 4000
 ;
 ;------- complex envelope block ------
  xtratim 1 ;extra-time, i.e. release dur
  krel init 0
  krel release ;outputs release-stage flag (0 or 1 values)
  if (krel == 1) kgoto rel ;if in release-stage goto release section
 ;
 ;************ attack and sustain section ***********
  kmp1 linseg 0, .03, 1, .05, 1, .07, 0, .08, .5, 4, 1, 50, 1
  kmp = kmp1*iamp
   kgoto done
 ;
 ;--------- release section --------
   rel:
  kmp2 linseg 1, .3, .2, .7, 0
  kmp = kmp1*kmp2*iamp
  done:
 ;------
  a1 oscili kmp, icps, gisin
  outs a1, a1
 endin

</CsInstruments>

<CsScore>
f 0 3600 ;dummy table to wait for realtime MIDI events
e
</CsScore>

</CsoundSynthesizer> 
