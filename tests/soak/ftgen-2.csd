<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o ftgen-2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

  sr	    =  48000
  ksmps	    =  16
  nchnls    =  2

;Example by Jonathan Murphy 2007

  0dbfs	    =  1

    instr 1

  Sfile	    =    "beats.wav"

  ilen	    filelen   Sfile  ; Find length
  isr	    filesr    Sfile  ; Find sample rate

  isamps    =  ilen * isr  ; Total number of samples
  isize	    init      1

loop:
  isize	    =  isize * 2
; Loop until isize is greater than number of samples
if (isize < isamps) igoto loop

  itab	    ftgen     0, 0, isize, 1, Sfile, 0, 0, 0
	    print     isize
	    print     isamps

  turnoff
    endin

</CsInstruments>
<CsScore>
i1 0 10
e
</CsScore>
</CsoundSynthesizer>

