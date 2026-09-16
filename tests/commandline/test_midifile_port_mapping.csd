<CsTest>
description = "MIDI file channel mapping at init and control rates"

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
nchnls = 1

; An absent file should match the other transport opcodes' no-op.
midifilerewind 999
giEmpty midifileopen "empty_port_test.mid"
iEmptyLength midifilelen giEmpty
iEmptyEvents midifilevents giEmpty
if abs(iEmptyLength - .5) > .001 || iEmptyEvents != 0 then
  exitnow -1
endif

instr Check
  iPort = p4
  iFile midifileopen "port_test.mid", iPort
  iLength midifilelen iFile
  ; Include the silence between the last note-off and end of track.
  if abs(iLength - 1) > .001 then
    exitnow -1
  endif
  iEvent = 0
  while iEvent < 4 do
    iStatus, iChannel, iData1, iData2, iTime midifilein iEvent, iFile
    iExpected = (iEvent % 2 == 0 ? 1 : 16) + 16 * iPort
    if iChannel != iExpected || iStatus != (iEvent < 2 ? 144 : 128) then
      print iPort, iEvent, iStatus, iChannel
      exitnow -1
    endif
    iEvent += 1
  od
  kEvent init 0
  kStatus, kChannel, kData1, kData2, kTime midifilein kEvent, iFile
  kExpected = (kEvent % 2 == 0 ? 1 : 16) + 16 * iPort
  if kChannel != kExpected || kStatus != (kEvent < 2 ? 144 : 128) then
    exitnowk -1
  endif
  kEvent = (kEvent + 1) % 4
endin
</CsInstruments>
<CsScore>
i "Check" 0 .01 0
i "Check" 0 .01 1
i "Check" 0 .01 3
i "Check" 0 .01 4
i "Check" 0 .01 16
i "Check" 0 .01 63
</CsScore>
<CsFileB filename="port_test.mid">
TVRoZAAAAAYAAAABAeBNVHJrAAAAFgCQPGQAn0Bkg2CAPAAAj0AAg2D/LwA=
</CsFileB>
<CsFileB filename="empty_port_test.mid">
TVRoZAAAAAYAAAABAeBNVHJrAAAABYNg/y8A
</CsFileB>
</CsoundSynthesizer>
