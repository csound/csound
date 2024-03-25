<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o readk-2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

giSine ftgen 0, 0, 2^10, 10, 1

instr 1 ;writes a control signal to a file
kfreq randh  100, 1, 2, 1, 500 ;generates one random number between 400 and 600 per second
      dumpk  kfreq, "dumpk.txt", 8, 1 ;writes the control signal
      printk 1, kfreq 				 ;prints it
endin

instr 2 ;reads the file written by instr 1
kfreq readk  "dumpk.txt", 8, 1
      printk 1, kfreq ;prints it
aout  poscil .2, kfreq, giSine
      outs   aout, aout
endin

</CsInstruments>
<CsScore>
i 1 0 5
i 2 5 5
e
</CsScore>
</CsoundSynthesizer>