<CsTest>
description = "Csound object channel access rejects invalid input and output channels"
[expect]
exit = "nonzero"
stderr = ["Csound inch: channel out of range", "Csound outch: channel out of range"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 64
nchnls = 1
instr 1
  child:Csound = create()
  iResult = setoption(child, "-n")
  iResult = compilestr(child, {{
    sr = 1024
    ksmps = 16
    nchnls = 2
    nchnls_i = 1
    instr 1
    endin
    schedule 1, 0, 1
  }})
  iResult = start(child)
  if p5 == 0 then
    aInput init 0
    outch child, p4, aInput
  else
    aOutput = inch(child, p4)
  endif
  delete child
endin
</CsInstruments>
<CsScore>
i 1 0 .125 0 0
i 1 .125 .125 2 0
i 1 .25 .125 0 1
i 1 .375 .125 3 1
i 1 .5 .125 1e30 0
i 1 .625 .125 1e30 1
e
</CsScore>
</CsoundSynthesizer>
