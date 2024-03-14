<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc       -M0 ;;;RT audio I/O with MIDI in
</CsOptions>
<CsInstruments>

; Example by Jonathan Murphy and Charles Gran 2007
  sr	    =  44100
  ksmps	    =  10
  nchnls    =  2

        ; new, and important. Make sure that midi note events are only
        ; received by instruments that actually need them.

	; turn default midi routing off
	massign		0, 0
	; route note events on channel 1 to instr 1
	massign		1, 1

; Define your midi controllers
#define C1 #21#
#define C2 #22#
#define C3 #23#

; Initialize MIDI controllers
	    initc7    1, $C1, 0.5 		  ;delay send
	    initc7    1, $C2, 0.5 		  ;delay: time to zero
	    initc7    1, $C3, 0.5 		  ;delay: rate 

  gaosc	    init      0

; Define an opcode to "smooth" the MIDI controller signal
    opcode smooth, k, k
  kin       xin
  kport	    linseg    0, 0.0001, 0.01, 1, 0.01
  kin       portk     kin, kport
            xout      kin
    endop

instr   1  
 ; Generate a sine wave at the frequency of the MIDI note that triggered the intrument
  ifqc	    cpsmidi
  iamp	    ampmidi   10000
  aenv	    linenr    iamp, .01, .1, .01 	  ;envelope
  a1	    oscil     aenv, ifqc, 1
; All sound goes to the global variable gaosc
  gaosc	    =  gaosc + a1
    endin

    instr     198 ; ECHO
  kcmbsnd   ctrl7     1, $C1, 0, 1 		  ;delay send
  ktime	    ctrl7     1, $C2, 0.01, 6 		  ;time loop fades out
  kloop	    ctrl7     1, $C3, 0.01, 1 		  ;loop speed
; Receive MIDI controller values and then smooth them
  kcmbsnd   smooth    kcmbsnd
  ktime	    smooth    ktime
  kloop	    smooth    kloop
  imaxlpt   =  1 				  ;max loop time
; Create a variable reverberation (delay) of the gaosc signal
  acomb	    vcomb     gaosc, ktime, kloop, imaxlpt, 1
  aout	    =  (acomb * kcmbsnd) + gaosc * (1 - kcmbsnd)
	    outs      aout, aout
  gaosc	    =  0
    endin

</CsInstruments>

<CsScore>
f1 0 16384 10 1
i198 0 10000
e
</CsScore>
</CsoundSynthesizer>