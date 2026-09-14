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
  xtratim .008
  if p1 == 1 then
    aa madsr .008, .008, .5, p5, p4, p5
    kk madsr .008, .008, .5, p5, p4, p5
  else
    aa mxadsr .008, .008, .5, p5, p4, p5
    kk mxadsr .008, .008, .5, p5, p4, p5
  endif
  ka downsamp aa
  idelay = int(p4 * kr + .5)
  iphase = int(.008 * kr + .5)
  irelease = int(p5 * kr + .5)
  kcount init 0
  kafter init 0
  kstart init 0
  kratio init 0
  krel release
  if kcount < idelay then
    kexpected = 0
  elseif kcount < idelay + iphase then
    kpos = (kcount - idelay) / iphase
    kexpected = p1 == 1 ? kpos : .001 * pow(1000, kpos)
  elseif kcount < idelay + 2 * iphase then
    kpos = (kcount - idelay - iphase) / iphase
    kexpected = p1 == 1 ? 1 - .5 * kpos : pow(.5, kpos)
  else
    kexpected = .5
  endif
  if krel == 1 then
    if kafter == 0 then
      kstart = kexpected
      if kstart > .001 then
        kratio = .001 / kstart
      else
        kratio = .001
      endif
    endif
    if kafter >= irelease then
      kexpected = 0
    elseif p1 == 1 then
      kexpected = kstart * (1 - kafter / irelease)
    else
      kexpected = kstart * pow(kratio, kafter / irelease)
    endif
    if kafter == int(.006 * kr + .5) then
      gkchecks += 1
    endif
    kafter += 1
  endif
  if !(abs(ka-kexpected) < .00001 && abs(kk-kexpected) < .00001) then
    printks "early ADSR note-off: type=%g block=%g audio=%g control=%g expected=%g\n", 0, p1, kcount, ka, kk, kexpected
    exitnowk(-1)
  endif
  kcount += 1
endin

instr 99
  if i(gkchecks) != 12 then
    prints "not all early ADSR releases completed\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Note-off during delay, attack, or decay; normal and instantaneous release.
i 1 0 0.004 0.008 0
i 1 0 0.004 0.008 0.004
i 1 0 0.004 0 0
i 1 0 0.004 0 0.004
i 1 0 0.012 0 0
i 1 0 0.012 0 0.004
i 2 0 0.004 0.008 0
i 2 0 0.004 0.008 0.004
i 2 0 0.004 0 0
i 2 0 0.004 0 0.004
i 2 0 0.012 0 0
i 2 0 0.012 0 0.004
i 99 .03 .001
</CsScore>
</CsoundSynthesizer>
