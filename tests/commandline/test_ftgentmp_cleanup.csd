<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1

giFirst init 0
giLast init 0

instr 1
Make:
  ifno ftgentmp p4, 0, 512, 10, 1
  if p4 == 0 then
    if giFirst == 0 then
      giFirst = ifno
    endif
    giLast = ifno
  endif
  rireturn
  kCycle init 0
  kCycle += 1
  if p4 == 0 && kCycle < 4 then
    reinit Make
  endif
endin

instr 2
  iFirst ftexists giFirst
  iLast ftexists giLast
  if iFirst != 1 || iLast != 1 || giLast <= giFirst then
    prints "Temporary tables must survive until note end\n"
    exitnow -1
  endif
endin

instr 3
  iNumber = giFirst
  while iNumber <= giLast do
    iReused ftgentmp 0, 0, 512, 10, 1
    if iReused != iNumber then
      prints "Temporary table survived note end\n"
      exitnow -1
    endif
    iNumber += 1
  od
  iExplicit ftexists 500
  if iExplicit != 1 then
    prints "Explicit table must persist\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .04 0
i 2 .02 .001
i 1 .06 .02 500
i 3 .1 .001
e
</CsScore>
</CsoundSynthesizer>
