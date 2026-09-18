<CsTest>
description = "String ternaries follow control conditions with unchanged inputs"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 32
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
  Sshort init "x"
  Slong init "a longer unchanged string"
  kCycle init 0
  kCycle += 1
  kOdd = kCycle % 2
  Sliteral = (kOdd == 1 ? "odd" : "even")
  Scopy = Sliteral
  Svariable = (kOdd == 1 ? Sshort : Slong)
  Sempty = (kOdd == 1 ? "" : "full")
  Snested = (kCycle % 4 == 0 ? "four" : (kOdd == 1 ? "odd" : "even"))
  Sdynamic sprintfk "%d", kCycle
  Smixed = (kOdd == 1 ? Sdynamic : "held")

  if kOdd == 1 then
    if strcmpk(Sliteral, "odd") != 0 || strcmpk(Scopy, "odd") != 0 || strcmpk(Svariable, Sshort) != 0 || strcmpk(Sempty, "") != 0 || strcmpk(Smixed, Sdynamic) != 0 then
      printks "string ternary did not select its true branch on cycle %d\n", 0, kCycle
      exitnowk(-1)
    endif
  else
    if strcmpk(Sliteral, "even") != 0 || strcmpk(Scopy, "even") != 0 || strcmpk(Svariable, Slong) != 0 || strcmpk(Sempty, "full") != 0 || strcmpk(Smixed, "held") != 0 then
      printks "string ternary did not select its false branch on cycle %d\n", 0, kCycle
      exitnowk(-1)
    endif
  endif
  if kCycle % 4 == 0 then
    kNestedError strcmpk Snested, "four"
  else
    kNestedError strcmpk Snested, Sliteral
  endif
  if kNestedError != 0 then
    printks "nested string ternary returned stale text\n", 0
    exitnowk(-1)
  endif
  if kCycle == 8 then
    gkChecks += 1
    turnoff
  endif
endin

instr 2
  ; Keep init-time selection and downstream init-time copies unchanged.
  Sselected = (p4 > 0 ? "positive" : "other")
  Scopy = Sselected
  if p4 > 0 then
    iError strcmp Scopy, "positive"
  else
    iError strcmp Scopy, "other"
  endif
  if iError != 0 then
    prints "init-time string ternary changed\n"
    exitnow(-1)
  endif
  gkChecks += 1
  turnoff
endin

instr 99
  if i(gkChecks) != 3 then
    prints "string ternary checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .5
i 2 0 .1 1
i 2 .1 .1 0
i 99 .6 .01
</CsScore>
</CsoundSynthesizer>
