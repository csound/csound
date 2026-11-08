<CsTest>
description = "string and generic channel bindings follow name and value changes"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 1

struct Value value:k
seed:Value init 0
chnseti seed, "object-first"
chnseti seed, "object-second"

instr 1
  SName init "text-first"
  kCycle timeinstk
  if kCycle == 1 then
    chnsetks "first", "text-first"
    chnsetks "second", "text-second"
  elseif kCycle == 2 then
    SName strcpyk "text-second"
  elseif kCycle == 3 then
    SName strcpyk "text-first"
  endif

  ; Read before writing so the two caches cannot hide a wrong destination.
  SRead chngetks SName
  if kCycle == 1 && strcmpk(SRead, "first") != 0 || \
     kCycle == 2 && strcmpk(SRead, "second") != 0 || \
     kCycle >= 3 && strcmpk(SRead, "updated") != 0 then
    printks "string read used the wrong channel on cycle %d\n", 0, kCycle
    exitnowk(-1)
  endif
  chnsetks "updated", SName
  SFirst chngetks "text-first"
  SSecond chngetks "text-second"
  if strcmpk(SFirst, "updated") != 0 || \
     kCycle >= 2 && strcmpk(SSecond, "updated") != 0 then
    printks "string write used the wrong channel on cycle %d\n", 0, kCycle
    exitnowk(-1)
  endif
endin

instr 2
  SName init "object-first"
  kCycle timeinstk
  if kCycle == 2 then
    SName strcpyk "object-second"
  elseif kCycle == 3 then
    SName strcpyk "object-first"
  endif

  source:Value init 0
  source.value = kCycle + 10
  chnsetk source, SName
  read:Value chngetk SName
  first:Value chngetk "object-first"
  second:Value chngetk "object-second"
  kDestination = (kCycle == 2 ? second.value : first.value)
  if read.value != source.value || kDestination != source.value then
    printks "generic channel used the wrong binding on cycle %d\n", 0, kCycle
    exitnowk(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.004
i 2 0 0.004
i 1 0.01 0.004
i 2 0.01 0.004
</CsScore>
</CsoundSynthesizer>
