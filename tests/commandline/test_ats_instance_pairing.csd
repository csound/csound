<CsTest>
description = "ATS consumers follow readers in their own instance, including runtime switches"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 16
nchnls = 1
0dbfs = 1
giSine ftgen 1, 0, 4096, 10, 1
strset 101, "ats-instance.ats"
gkChecks init 0

; The fixture has constant partials at 100, 200, 400 and 800 Hz,
; with amplitudes .1, .2, .4 and .8. Its generator is beside this CSD.
opcode ChildReader, 0, 0
  atsbufread 0, 4, "ats-instance.ats", 4
endop

instr Child
  ATSbufread 0, 8, 101, 4
  out a(0)
endin

instr Parent
  kCycle init 0
  atsbufread 0, 1, "ats-instance.ats", 4
  ; Nested readers run between the parent's reader and its consumers.
  ChildReader
  aChild subinstr "Child"
  kFirstFreq, kFirstAmp atspartialtap 1
  kFirstInterp atsinterpread 150
  if abs(kFirstFreq - 100) > .001 || abs(kFirstAmp - .1) > .00001 || abs(kFirstInterp - .15) > .00001 then
    printks "A child replaced the parent's ATS reader\n", 0
    exitnowk -1
  endif

  ; The second group changes reader every 16 cycles. Consumers must follow
  ; the reader that ran this cycle, not the last one initialized.
  kHalfSpeed = (int(kCycle / 16) % 2)
  if kHalfSpeed == 0 then
    ATSbufread 0, 1, 101, 4
  else
    atsbufread 0, .5, "ats-instance.ats", 4
  endif
  if kCycle == 40 then
    reinit CONSUMERS
  endif
CONSUMERS:
  kFreq, kAmp ATSpartialtap 1
  kInterp ATSinterpread 150
  ; Cross-synthesis takes its amplitude from the current buffer.
  aCross atscross 0, 1, "ats-instance.ats", giSine, 0, 1, 1
  ; The reference takes amplitude from its file, ignoring buffered amplitude.
  ; Using the same oscillator also keeps phase reset behavior identical.
  aReference atscross 0, 1, "ats-instance.ats", giSine, 1, 0, 1
  rireturn
  kExpectedFreq = (kHalfSpeed == 0 ? 100 : 50)
  kExpectedInterp = (kHalfSpeed == 0 ? .15 : .3)
  if abs(kFreq - kExpectedFreq) > .001 || abs(kAmp - .1) > .00001 || abs(kInterp - kExpectedInterp) > .00001 then
    printks "ATS consumers did not follow the active local reader\n", 0
    exitnowk -1
  endif
  ; Halving the buffered frequencies doubles its amplitude at 100 Hz.
  ; Allow the existing one-block amplitude ramp after a reader switch.
  kGain = (kHalfSpeed == 0 ? 1 : 2)
  kError max_k abs(aCross - kGain*aReference), 1, 1
  if kCycle % 16 != 0 && kCycle != 40 && !(kError < .00001) then
    printks "atscross used the wrong buffered envelope\n", 0
    exitnowk -1
  endif
  kLevel rms aCross
  if kCycle == 30 || kCycle == 70 then
    if !(kLevel > .001) then
      printks "atscross produced silence\n", 0
      exitnowk -1
    endif
    gkChecks += 1
  endif
  kCycle += 1
endin

instr LateReader
  kCycle init 0
  ; atscross may initialize before its reader. Wait one cycle so the reader
  ; has filled its buffer before synthesis starts. Use the old numeric form.
  if kCycle > 0 then
    aCross ATScross 0, 1, 101, giSine, 0, 1, 1
    kLevel rms aCross
    if kCycle == 30 then
      if !(kLevel > .001) then
        printks "ATScross did not find its later reader\n", 0
        exitnowk -1
      endif
      gkChecks += 1
    endif
  endif
  ATSbufread 0, 1, 101, 4
  kCycle += 1
endin

instr CheckCompletion
  if i(gkChecks) != 7 then
    prints "Not every parent reached both ATS checks\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Overlap two parents, then reuse an ended parent and its children.
i "Parent" 0 .2
i "Parent" .04 .2
i "Parent" .3 .2
i "LateReader" .55 .1
i "CheckCompletion" .7 0
e
</CsScore>
</CsoundSynthesizer>
