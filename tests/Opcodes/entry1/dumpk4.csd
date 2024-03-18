<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o dumpk4.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

giSine ftgen 0, 0, 2^10, 10, 1

instr 1 ;writes four control signals to a file
kcf       randh     950, 1, 2, 1, 1050 ;generates one random number between 100 and 2000 per second
kq        randh     10, 1, 2, 1, 11 ;generates another random number between 1 and 21 per second
kdb       randh     9, 1, 2, 1, -15 ;amplitudes in dB between -24 and -6
kpan      randh     .5, 1, 2, 1, .5 ;panning between 0 and 1
          dumpk4    kcf, kq, kdb, kpan, "dumpk4.txt", 8, 1 ;writes the control signals
          prints    "WRITING:\n"
          printks   "kcf = %f, kq = %f, kdb = %f, kpan = %f\n", 1, kcf, kq, kdb, kpan  ;prints them
endin

instr 2 ;reads the file written by instr 1
kcf,kq,kdb,kp readk4 "dumpk4.txt", 8, 1
          prints    "READING:\n"
          printks   "kcf = %f, kq = %f, kdb = %f, kpan = %f\n", 1, kcf, kq, kdb, kp  ;prints values
kdb       lineto    kdb, .1 ;smoothing amp transition
kp        lineto    kp, .1 ;smoothing pan transition
anoise    rand      ampdb(kdb), 2, 1
kbw       =         kcf/kq ;bandwidth of resonant filter
abp       reson     anoise, kcf, kbw
aout      balance   abp, anoise
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
