<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o strcatk.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

  sr	    =  48000
  ksmps	    =  16
  nchnls    =  2
  0dbfs	    =  1

; Example by Jonathan Murphy 2007

    instr 1

  S1	    =  "1"
  S2	    =  " + 1"
  ktrig	    init      0
  kval	    init      2
if (ktrig == 1) then
  S1	    strcatk   S1, S2
  kval	    =  kval + 1
endif
  String    sprintfk  "%s = %d", S1, kval
	    puts      String, kval
  ktrig	    metro     1

    endin

</CsInstruments>
<CsScore>
i1 0 10
e
</CsScore>
</CsoundSynthesizer>
