<CsTest>
description = "bformdec2's binaural setups stop at the first HRTF file they cannot load, instead of trying it again for every speaker"

[expect]
exit = "nonzero"
stderr_regex = ['INIT ERROR in instr 1: note deleted \(1 init errors\)',
          'INIT ERROR in instr 2: note deleted \(1 init errors\)']
output = ["reached the end of the run"]
output_regex = ['(?s)^(?!.*cannot load no/such/21l\.dat.*cannot load no/such/21l\.dat).*cannot load no/such/21l\.dat',
          '(?s)^(?!.*cannot load no/such/31r\.dat.*cannot load no/such/31r\.dat).*cannot load no/such/31r\.dat']
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

; Setup 21 decodes through 8 virtual speakers and 31 through 20, each loading
; the two HRTF files. One that could not be loaded was tried again, and its
; error given again, for every speaker: 8 and 20 init errors for one note.

instr 1
  aIn[] init 4
  aOut[] init 2
  aOut bformdec2 21, aIn, 0, 0, 0, 0, "no/such/21l.dat", "hrtf-44100-right.dat"
endin

instr 2
  aIn[] init 4
  aOut[] init 2
  aOut bformdec2 31, aIn, 0, 0, 0, 0, "hrtf-44100-left.dat", "no/such/31r.dat"
endin

instr 90
  prints "reached the end of the run\n"
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
i 2 0.02 0.01
i 90 0.05 0.01
</CsScore>
</CsoundSynthesizer>
