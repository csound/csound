<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
; -odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
-n -Fmidichn_advanced.mid
;Don't write audio ouput to disk and use the file midichn_advanced.mid as MIDI input
</CsOptions>
<CsInstruments>

  sr	    =  48000
  ksmps	    =  16
  nchnls    =  2

  ;Example by Jonathan Murphy 2007

	    ; assign all midi events to instr 1000
	    massign   0, 1000
	    pgmassign	0, 1000

    instr 1000

  ktim	timeinsts
	
  kst, kch, kd1, kd2  midiin
if (kst != 0) then
;  p4 = MIDI event type   p5 = channel   p6= data1    p7= data2
	    fprintks  "MIDI2cs.sco", "i1\\t%f\\t%f\\t%d\\t%d\\t%d\\t%d\\n", ktim, 1/kr, kst, kch, kd1, kd2
endif

    endin


</CsInstruments>
<CsScore>
i1000 0 10000
e
</CsScore>
</CsoundSynthesizer>