<CsTest>
description = "a UDO's local cycles that do not run still keep its clock and the clocks of the UDOs, opcode objects and subinstruments it calls"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1

gkOuter init 0
gkInner init 0
gkOuterTime init 0
gkInnerTime init 0
gkGrand init 0
gkGrandTime init 0
gkNotes init 0
giStart init 0

; Each callee counts its lastcycle and keeps its clock.
opcode InnerNew():k
  gkInner += lastcycle()
  gkInnerTime = timek()
  xout(gkInnerTime)
endop

opcode InnerClassic, k, 0
  gkInner += lastcycle()
  gkInnerTime = timek()
  xout gkInnerTime
endop

opcode Grand():k
  gkGrand += lastcycle()
  gkGrandTime = timek()
  xout(gkGrandTime)
endop

; This one calls one more, which must keep its time too.
opcode InnerOver, k, 0
  oversample 2, 9
  gkInner += lastcycle()
  gkInnerTime = timek()
  kG = Grand()
  xout gkInnerTime
endop

opcode GrandLocal4():k
  setksmps 4
  gkGrand += lastcycle()
  gkGrandTime = timek()
  xout(gkGrandTime)
endop

; So does this one, which runs blocks of its own.
opcode InnerLocal, k, 0
  setksmps 16
  gkInner += lastcycle()
  gkInnerTime = timek()
  kG = GrandLocal4()
  xout gkInnerTime
endop

opcode InnerLocal4, k, 0
  setksmps 4
  gkInner += lastcycle()
  gkInnerTime = timek()
  xout gkInnerTime
endop

opcode InnerOne, k, 0
  setksmps 1
  gkInner += lastcycle()
  gkInnerTime = timek()
  xout gkInnerTime
endop

opcode InnerUnder, k, 0
  undersample 2, 4
  gkInner += lastcycle()
  gkInnerTime = timek()
  xout gkInnerTime
endop

; Run as opcode objects.
opcode InnerObject(x:k):k
  gkInner += lastcycle()
  gkInnerTime = timek()
  xout(gkInnerTime)
endop

opcode InnerObjects(x:k):void
  gkInner += lastcycle()
  gkInnerTime = timek()
endop

opcode InnerObjectLocal(x:k):k
  setksmps 16
  gkInner += lastcycle()
  gkInnerTime = timek()
  xout(gkInnerTime)
endop

opcode InnerObjectOver(x:k):k
  oversample 2, 9
  gkInner += lastcycle()
  gkInnerTime = timek()
  xout(gkInnerTime)
endop

; Its body could run the object it runs as, in a branch never taken.
opcode InnerSelf(o:Opcode, x:k):k
  if x == 1 then
    kSelf:k = perf(o, o, x)
  endif
  gkInner += lastcycle()
  gkInnerTime = timek()
  xout(gkInnerTime)
endop

; Runs the object its caller set up.
opcode Runner(o:Opcode):k
  kR:k = perf(o, 0)
  xout(kR)
endop

; Sets up the object its caller runs.
opcode Prepare(o:Opcode):void
  kI:k = init(o, 0)
endop

; Calls itself, down to the one that counts.
opcode Deep(depth:i):k
  if depth > 0 then
    kD:k = Deep(depth - 1)
  else
    gkInner += lastcycle()
    gkInnerTime = timek()
  endif
  xout(gkInnerTime)
endop

; It calls one more too.
instr Sub
  gkInner += lastcycle()
  gkInnerTime = timek()
  kG = Grand()
endin

; It sets up an object and runs it.
instr SubObject
  inside:Opcode = create(opcoderef("InnerObject"))
  kI:k = init(inside, 0)
  kO:k = perf(inside, 0)
endin

; p4 of the caller: 0 a new-style UDO, 1 a classic one, 2 one that
; oversamples again, 3 a subinstrument, 4 a UDO with setksmps 16, 5 one that
; undersamples, 6 a UDO run as an opcode object, 7 two run as an array, 8 a
; UDO with setksmps 4, 9 a subinstrument run as an opcode object, 10 a UDO
; object run with perf, 11 one whose body could run itself, 12 one run by a
; UDO the caller calls, 13 a UDO with setksmps 1, 14 an object of a UDO with
; setksmps 16, 15 an object of one that oversamples, 16 an object set up by a
; UDO the caller calls, 17 a UDO calling itself three times, 18 one object
; run twice in each cycle, 19 an object in a subinstrument.
#define BODY #
  gkOuter += lastcycle()
  gkOuterTime = timek()
  if iKind == 0 then
    kN = InnerNew()
  elseif iKind == 1 then
    kN = InnerClassic()
  elseif iKind == 2 then
    kN = InnerOver()
  elseif iKind == 3 then
    aS subinstr "Sub"
    kN = 0
  elseif iKind == 4 then
    kN = InnerLocal()
  elseif iKind == 5 then
    kN = InnerUnder()
  elseif iKind == 6 then
    one:Opcode = create(opcoderef("InnerObject"))
    kN = run(one, 0)
  elseif iKind == 7 then
    two:Opcode[] = create(opcoderef("InnerObjects"), 2)
    kIn[] init 2
    run(two, kIn)
    kN = 0
  elseif iKind == 8 then
    kN = InnerLocal4()
  elseif iKind == 10 then
    obj:Opcode = create(opcoderef("InnerObject"))
    kInit:k = init(obj, 0)
    kN = perf(obj, 0)
  elseif iKind == 11 then
    me:Opcode = create(opcoderef("InnerSelf"))
    kSet:k = init(me, me, 0)
    kN = perf(me, me, 0)
  elseif iKind == 12 then
    held:Opcode = create(opcoderef("InnerObject"))
    kHeld:k = init(held, 0)
    kN = Runner(held)
  elseif iKind == 13 then
    kN = InnerOne()
  elseif iKind == 14 then
    local:Opcode = create(opcoderef("InnerObjectLocal"))
    kN = run(local, 0)
  elseif iKind == 15 then
    over:Opcode = create(opcoderef("InnerObjectOver"))
    kN = run(over, 0)
  elseif iKind == 16 then
    made:Opcode = create(opcoderef("InnerObject"))
    Prepare(made)
    kN = perf(made, 0)
  elseif iKind == 17 then
    kN = Deep(3)
  elseif iKind == 18 then
    giStart = timek:i()
    twice:Opcode = create(opcoderef("InnerObject"))
    kTwice:k = init(twice, 0)
    kN = perf(twice, 0)
    kAgain = Runner(twice)
  elseif iKind == 19 then
    aS subinstr "SubObject"
    kN = 0
  else
    sub:Opcode = create(opcoderef("subinstr"))
    aS = run(sub, "Sub")
    kN = 0
  endif
  xout kN
#

opcode OuterOver, k, i
  iKind xin
  oversample 4, 9
  $BODY
endop

opcode OuterLocal, k, i
  iKind xin
  setksmps 16
  $BODY
endop

opcode OuterOne, k, i
  iKind xin
  setksmps 1
  $BODY
endop

opcode OuterUnder, k, i
  iKind xin
  undersample 4, 4
  $BODY
endop

; p4: the callee, p5: the note's first sample in its block, p6: its length,
; p7: 0 for the caller with oversample 4, 1 for setksmps 16, 2 for
; setksmps 1, 3 for undersample 4. Its first local cycles hold none of the
; note and do not run.
instr Check
  gkOuter init 0
  gkInner init 0
  gkGrand init 0
  if p7 == 0 then
    kOut OuterOver p4
  elseif p7 == 1 then
    kOut OuterLocal p4
  elseif p7 == 2 then
    kOut OuterOne p4
  else
    kOut OuterUnder p4
  endif
  ; The callees that oversample again run two cycles for each of their
  ; caller's, and one with a ksmps of its own its blocks in each of its
  ; caller's cycles, less those in the last one after the note's end; so
  ; does the setksmps 4 callee of the one with setksmps 16.
  iOwn = p4 == 4 || p4 == 14 ? 16 : p4 == 8 ? 4 : p4 == 13 ? 1 : 0
  iLocal = p7 == 0 ? ksmps : p7 == 2 ? 1 : 16
  iEnd = p7 == 0 ? (p5 + p6) * 4 : p7 == 3 ? ceil((p5 + p6) / 4) : p5 + p6
  iTimes = p4 == 2 || p4 == 15 ? 2 : iOwn > 0 ? iLocal / iOwn : 1
  iAfter = iOwn > 0 ? floor(((iLocal - iEnd % iLocal) % iLocal) / iOwn) : 0
  iGrandTimes = p4 == 4 ? 4 : 1
  iGrandAfter = p4 == 4 ? floor(((16 - iEnd % 16) % 16) / 4) : 0
  ; The object run twice in each cycle counts two from its caller's clock
  ; at init, and its lastcycle() comes halfway: not checked.
  iTimes = p4 == 18 ? 2 : iTimes
  iAfter = p4 == 18 ? giStart : iAfter
  if lastcycle() == 1 then
    ; There are two objects in 7.
    if gkOuter != 1 || (p4 != 18 && gkInner != (p4 == 7 ? 2 : 1)) || \
       gkInnerTime != gkOuterTime * iTimes - iAfter || \
       ((p4 == 2 || p4 == 3 || p4 == 4 || p4 == 9) && \
        (gkGrand != 1 || \
         gkGrandTime != gkInnerTime * iGrandTimes - iGrandAfter)) then
      printks "caller %d, callee %d, note %d+%d: lastcycle %d times at \
clock %d, in the callee %d times at clock %d, in its callee %d times at \
clock %d\n", 0, p7, p4, p5, p6, gkOuter, gkOuterTime, gkInner, \
              gkInnerTime, gkGrand, gkGrandTime
      exitnowk -1
    endif
    gkNotes += 1
  endif
endin

; An object whose body calls a UDO with setksmps 16 that could run that
; object again, in a branch never taken: the blocks the UDO skips before
; the note's start leave the object's clock alone.
opcode Below(o:Opcode, x:k):k
  setksmps 16
  if x == 1 then
    kR:k = perf(o, o, x)
  endif
  gkInner += lastcycle()
  xout(0)
endop

opcode Above(o:Opcode, x:k):k
  gkOuter += lastcycle()
  gkOuterTime = timek()
  kB = Below(o, x)
  xout(gkOuterTime)
endop

instr Ancestor
  gkOuter init 0
  gkInner init 0
  above:Opcode = create(opcoderef("Above"))
  kInit:k = init(above, above, 0)
  kN = perf(above, above, 0)
  if lastcycle() == 1 then
    if gkOuter != 1 || gkInner != 1 || gkOuterTime != timek() then
      printks "object above, note 40+100: lastcycle %d times at clock %d, \
the instrument's %d, in the callee %d times\n", 0, gkOuter, \
              gkOuterTime, timek(), gkInner
      exitnowk -1
    endif
    gkNotes += 1
  endif
endin

instr CheckResults
  if i(gkNotes) != 64 then
    prints "Expected 64 notes checked, got %g\n", i(gkNotes)
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; A note from sample 40 for 100 samples, and from 16 for 144: at factor 4
; the first two local cycles hold none of the note, and the first one; with
; setksmps 16 the first two local blocks, the first one, and from sample 8
; none, and with setksmps 1 the first 40.
; From sample 61, undersample 4 rounds the start past its first block.
#define NOTE(K'O'D'T'C) #i "Check" [(64 * $T + $O + 0.5) / 48000] [($D + 0.25) / 48000] $K $O $D $C#
$NOTE(0'40'100'10'0)
$NOTE(1'40'100'20'0)
$NOTE(2'40'100'30'0)
$NOTE(3'40'100'40'0)
$NOTE(4'40'100'50'0)
$NOTE(5'40'100'60'0)
$NOTE(6'40'100'70'0)
$NOTE(7'40'100'80'0)
$NOTE(0'16'144'90'0)
$NOTE(2'16'144'100'0)
$NOTE(0'40'100'110'1)
$NOTE(1'40'100'120'1)
$NOTE(2'40'100'130'1)
$NOTE(3'40'100'140'1)
$NOTE(4'40'100'150'1)
$NOTE(5'40'100'160'1)
$NOTE(6'40'100'170'1)
$NOTE(7'40'100'180'1)
$NOTE(0'40'100'190'2)
$NOTE(1'40'100'200'2)
$NOTE(2'40'100'210'2)
$NOTE(3'40'100'220'2)
$NOTE(6'40'100'230'2)
$NOTE(7'40'100'240'2)
$NOTE(9'40'100'250'0)
$NOTE(8'40'100'260'1)
$NOTE(9'40'100'270'1)
$NOTE(0'61'100'280'3)
$NOTE(2'61'100'290'3)
$NOTE(3'61'100'300'3)
$NOTE(6'61'100'310'3)
$NOTE(7'61'100'320'3)
$NOTE(8'61'100'330'3)
$NOTE(9'61'100'340'3)
$NOTE(10'40'100'350'0)
$NOTE(10'40'100'360'1)
$NOTE(10'61'100'370'3)
$NOTE(8'40'100'380'0)
$NOTE(11'40'100'390'0)
$NOTE(11'40'100'400'1)
$NOTE(12'40'100'420'0)
$NOTE(12'40'100'430'1)
$NOTE(13'40'100'440'0)
$NOTE(9'40'100'450'2)
$NOTE(10'40'100'460'2)
$NOTE(11'40'100'470'2)
$NOTE(12'61'100'480'3)
$NOTE(11'61'100'490'3)
$NOTE(15'40'100'500'0)
$NOTE(14'40'100'510'0)
$NOTE(13'40'100'520'1)
$NOTE(13'61'100'530'3)
$NOTE(12'40'100'540'2)
$NOTE(16'40'100'550'0)
$NOTE(17'40'100'560'0)
$NOTE(16'40'100'570'1)
$NOTE(17'40'100'580'1)
$NOTE(0'16'144'590'1)
$NOTE(0'8'100'600'1)
$NOTE(18'40'100'610'0)
$NOTE(19'40'100'620'0)
$NOTE(18'40'100'630'1)
$NOTE(19'40'100'640'1)
i "Ancestor" [(64 * 410 + 40.5) / 48000] [100.25 / 48000]
i "CheckResults" 0.9 0
e 0.91
</CsScore>
</CsoundSynthesizer>
