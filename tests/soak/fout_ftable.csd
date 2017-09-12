<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o fout_ftable.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
; By: Jonathan Murphy 2007

  gilen		=		131072
  gicps		=		sr/gilen
  gitab		ftgen		1, 0, gilen, 10, 1

                instr 1

  /******** write file to table ********/

  ain		diskin2		"beats.wav", 1, 0, 1
  aphs		phasor		gicps
  andx		=		aphs * gilen
		tablew		ain, andx, gitab

  /******** write table to file ********/

  aosc		table		aphs, gitab, 1
		out		aosc
		fout		"beats_copy.wav", 6, aosc

                endin

</CsInstruments>
<CsScore>
i1 0 2
e
</CsScore>
</CsoundSynthesizer>