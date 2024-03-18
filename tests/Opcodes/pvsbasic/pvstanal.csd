<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pvstanal.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

gifil     ftgen     0, 0, 0, 1, "fox.wav", 0, 0, 1

instr 1

fsig      pvstanal  p4, 1, p5, gifil, p6, p7
aout      pvsynth   fsig
          outs      aout, aout
endin

instr 2

kspeed    randi     2, 2, 2 ;speed randomly between -2 and 2
kpitch    randi     2, 2, 2 ;pitch between 2 octaves lower or higher
fsig      pvstanal  kspeed, 1, octave(kpitch), gifil
aout      pvsynth   fsig
          outs      aout, aout
endin

</CsInstruments>
<CsScore>
;           speed pch det wrap
i 1 0 2.757 1     1   0   0
i 1 3 .     2     1   0   0
i 1 6 .     2     1   0   1
i 1 9 .     1     .75
i 2 12 10 ;random scratching
e
</CsScore>
</CsoundSynthesizer>
