<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o dumpk3.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

giSine ftgen 0, 0, 2^10, 10, 1

instr 1 ;writes three control signals to a file
kfreq     randh     100, 1, 2, 1, 500 ;generates one random number between 400 and 600 per second
kdb       randh     12, 1, 2, 1, -12 ;amplitudes in dB between -24 and 0
kpan      randh     .5, 1, 2, 1, .5 ;panning between 0 and 1
          dumpk3    kfreq, kdb, kpan, "dumpk3.txt", 8, 1 ;writes the control signals
          prints    "WRITING:\n"
          printks   "kfreq = %f, kdb = %f, kpan = %f\n", 1, kfreq, kdb, kpan  ;prints them
endin

instr 2 ;reads the file written by instr 1
kf,kdb,kp readk3    "dumpk3.txt", 8, 1
          prints    "READING:\n"
          printks   "kfreq = %f, kdb = %f, kpan = %f\n", 1, kf, kdb, kp  ;prints again
kdb       lineto    kdb, .1 ;smoothing amp transition
kp        lineto    kp, .1 ;smoothing pan transition
aout      poscil    ampdb(kdb), kf, giSine
aL, aR    pan2      aout, kp
          outs      aL, aR
endin

</CsInstruments>
<CsScore>
i 1 0 5
i 2 5 5
e
</CsScore>
</CsoundSynthesizer>

