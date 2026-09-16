<CsTest>
description = "Program and controller readers agree with MIDI message fields and filters"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 -F reader_fields.mid
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 1
nchnls = 1
massign 0, 0
gkMessages init 0
giRuns init 0

instr 1
  giRuns += 1
  kStatus, kChannel, kData1, kData2 midiin
  kProgram, kProgramChannel pgmchn
  kProgram3, kProgramChannel3 pgmchn 3
  kValue, kNumber, kControlChannel ctlchn
  kValue3, kNumber3, kControlChannel3 ctlchn 3, 7
  kValue7, kNumber7, kControlChannel7 ctlchn 0, 7

  kExpectedProgram = (kStatus == 192 ? kData1 + 1 : -1)
  kExpectedChannel = (kStatus == 192 ? kChannel : 0)
  if kProgram != kExpectedProgram || kProgramChannel != kExpectedChannel then
    exitnowk -1
  endif
  if kChannel != 3 then
    kExpectedProgram = -1
    kExpectedChannel = 0
  endif
  if kProgram3 != kExpectedProgram || kProgramChannel3 != kExpectedChannel then
    exitnowk -1
  endif

  kExpectedValue = (kStatus == 176 ? kData2 : -1)
  kExpectedNumber = (kStatus == 176 ? kData1 : -1)
  kExpectedChannel = (kStatus == 176 ? kChannel : 0)
  if kValue != kExpectedValue || kNumber != kExpectedNumber || kControlChannel != kExpectedChannel then
    exitnowk -1
  endif
  if kData1 != 7 then
    kExpectedValue = -1
    kExpectedNumber = -1
    kExpectedChannel = 0
  endif
  if kValue7 != kExpectedValue || kNumber7 != kExpectedNumber || kControlChannel7 != kExpectedChannel then
    exitnowk -1
  endif
  if kChannel != 3 then
    kExpectedValue = -1
    kExpectedNumber = -1
    kExpectedChannel = 0
  endif
  if kValue3 != kExpectedValue || kNumber3 != kExpectedNumber || kControlChannel3 != kExpectedChannel then
    exitnowk -1
  endif
  if kStatus != 0 then
    gkMessages += 1
  endif
endin

instr 2
  if i(gkMessages) != 11 || giRuns != 2 then
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .13
; A reused reader starts at the current buffer position, with no old events.
i 1 .2 .01
i 2 .22 .001
</CsScore>
<CsFileB filename="reader_fields.mid">
TVRoZAAAAAYAAAABAfRNVHJrAAAALQrAAArCCQrPfwqyB2QKsggHCrAHAAq/B38KsgBCCrJ/AQqSPGQKgjwACv8vAA==
</CsFileB>
</CsoundSynthesizer>
