<CsoundSynthesizer>
<CsOptions>
-odac -m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2

instr 1
  ; Attempt to open a filename that should not exist to provoke a runtime/file I/O error
  a1 diskin2 "THIS_FILE_DOES_NOT_EXIST_12345.wav", 1, 0, 0
  outs a1, a1
endin
</CsInstruments>
<CsScore>
i1 0 0.1
</CsScore>
</CsoundSynthesizer>
