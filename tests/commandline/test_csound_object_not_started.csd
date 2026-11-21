<CsTest>
description = "A Csound object that did not start is not performed, and its channels are not read or written, on a reused instance too"
[expect]
exit = 4
stderr = ["Csound perf: the Csound object has not started", "Csound inch: the Csound object has not started", "Csound outch: the Csound object has not started"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 64
nchnls = 1
; p4: 0 perf, 1 inch, 2 outch
; p5: 1 a child that starts, 0 one whose start fails, 2 one never started
; Each note ends before the next, so the next takes its instance, and the
; Csound object left in it.
instr 1
  child:Csound = create()
  iResult = setoption(child, "-n")
  if p5 == 1 then
    iResult = compilestr(child, {{
      sr = 1024
      ksmps = 16
      nchnls = 1
    }})
  else
    iResult = compilestr(child, {{
      sr = 1024
      ksmps = 16
      nchnls = 1
      iMissing = ftlen(99)
    }})
  endif
  if p5 != 2 then
    iResult = start(child)
  endif
  if p4 == 0 then
    kResult = perf(child)
  elseif p4 == 1 then
    aOutput = inch(child, 1)
  else
    aInput init 0
    outch child, 1, aInput
  endif
  delete child
endin
</CsInstruments>
<CsScore>
i 1 0    .125 0 1
i 1 .25  .125 0 0
i 1 .5   .125 1 1
i 1 .75  .125 1 0
i 1 1    .125 2 1
i 1 1.25 .125 2 0
i 1 1.5  .125 0 2
e
</CsScore>
</CsoundSynthesizer>
