<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
#ifndef TEST_KSMPS
#define TEST_KSMPS #8#
#endif
sr = 8000
ksmps = $TEST_KSMPS
nchnls = 1
0dbfs = 1
gkchecks init 0

instr 1, 2
  ; A second envelope requests a longer shared release than the main irel.
  aLong madsr .001, .001, .25, .006
  if p1 == 1 then
    aa madsr .001, .001, .5, .004, 0, p4
    kk madsr .001, .001, .5, .004, 0, p4
  else
    aa mxadsr .001, .001, .5, .004, 0, p4
    kk mxadsr .001, .001, .5, .004, 0, p4
  endif
  ka downsamp aa
  iduration = p4 < 0 ? .006 : p4
  icount = int(iduration * kr + .5)
  iphase = int(.001 * kr + .5)
  krel release
  kcount init 0
  kafter init 0
  if kcount < iphase then
    kexpected = p1 == 1 ? kcount / iphase : .001 * pow(1000, kcount / iphase)
  elseif kcount < 2 * iphase then
    kexpected = p1 == 1 ? 1 - .5 * (kcount - iphase) / iphase : pow(.5, (kcount - iphase) / iphase)
  else
    kexpected = .5
  endif
  if krel == 1 then
    if kafter >= icount then
      kexpected = 0
    else
      kexpected = p1 == 1 ? .5 * (1 - kafter / icount) : .5 * pow(.002, kafter / icount)
    endif
    ; This also proves a long override extends the note's lifetime.
    if kafter == max(icount - 1, 0) then
      gkchecks += 1
    endif
    kafter += 1
  endif
  if !(abs(ka-kexpected) < .00001 && abs(kk-kexpected) < .00001) then
    printks "madsr ireltim=%g: audio=%g control=%g expected=%g\n", 0, p4, ka, kk, kexpected
    exitnowk(-1)
  endif
  kcount += 1
endin

instr 99
  if i(gkchecks) != 8 then
    prints "not all madsr release overrides ran to completion\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .02 -1
i 1 0 .02 0
i 1 0 .02 .002
i 1 0 .02 .008
i 2 0 .02 -1
i 2 0 .02 0
i 2 0 .02 .002
i 2 0 .02 .008
i 99 .04 .001
</CsScore>
</CsoundSynthesizer>
