<CsTest>
description = "A MIDI file on a port keeps its system messages from forging channel messages"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 -F port_system_empty.mid
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 1
nchnls = 1

massign 0, 0

gkFirst init 0
gkSecond init 0
gkOthers init 0

; A port byte follows a channel message's status alone: written after a
; clock, a song position or a song select, the parser read it as a status
; of its own.
instr Read
  kStatus = 1
  while kStatus != 0 do
    ; One midiin, read until it has nothing: a second keeps its own place.
    kStatus, kChannel, kData1, kData2 midiin
    if kStatus == 0 then
      kgoto done
    elseif kChannel == 17 && kStatus == 144 && kData1 == 60 && kData2 == 100 then
      gkFirst += 1
    elseif kChannel == 17 && kStatus == 144 && kData1 == 62 && kData2 == 100 then
      gkSecond += 1
    elseif kChannel != 17 then
      printks "a message on channel %d: status %d, data %d %d\n", 0, kChannel, kStatus, kData1, kData2
      gkOthers += 1
    endif
  od
done:
endin

instr Play
  iFile midifileopen "port_system.mid", 1
  midifileplay iFile
endin

instr Check
  if i(gkFirst) != 1 || i(gkSecond) != 1 || i(gkOthers) != 0 then
    print i(gkFirst), i(gkSecond), i(gkOthers)
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; A key on channel 1, a clock, a song position, a song select, another
; key, then their note-offs, played on port 1: channel 17.
i "Read" 0 1
i "Play" 0 .01
i "Check" 1 0
</CsScore>
<CsFileB filename="port_system.mid">
TVRoZAAAAAYAAAABAeBNVHJrAAAAHgCQPGQA+ADyAAAA8wEAkD5kg2CAPAAAgD4AAP8vAA==
</CsFileB>
<CsFileB filename="port_system_empty.mid">
TVRoZAAAAAYAAAABAeBNVHJrAAAABYNg/y8A
</CsFileB>
</CsoundSynthesizer>
