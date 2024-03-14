<CsoundSynthesizer>

<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o FLpanel-sensekey.wav -W ;;; for file output any platform
</CsOptions>

<CsInstruments>
; Example by Johnathan Murphy

  sr	    =  44100
  ksmps	    =  128
  nchnls    =  2


	; ikbdcapture flag set to 1
  ikey	    init      1 		
 
	    FLpanel   "sensekey", 740, 340, 100, 250, 2, ikey
  gkasc, giasc	FLbutBank	2, 16, 8, 700, 300, 20, 20, -1
	    FLpanelEnd
	    FLrun

    instr 1

  kkey	    sensekey
  kprint    changed   kkey
	    FLsetVal  kprint, kkey, giasc

    endin

</CsInstruments>

<CsScore>
i1 0 60
e
</CsScore>

</CsoundSynthesizer>