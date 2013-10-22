<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d
; For Non-realtime ouput leave only the line below:
; -o schedkwhennamed.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

  sr	    =  48000
  ksmps	    =  16
  nchnls    =  2
  0dbfs	    =  1

; Example by Jonathan Murphy 2007

  gSinstr2  =  "printer"

    instr 1

  ktrig	    metro     1
if (ktrig == 1) then
  ;Call instrument "printer" once per second
	    schedkwhennamed   ktrig, 0, 1, gSinstr2, 0, 1

endif

    endin

    instr printer

  ktime	    timeinsts
	    printk2   ktime

    endin

</CsInstruments>
<CsScore>
i1 0 10
e
</CsScore>
</CsoundSynthesizer>

