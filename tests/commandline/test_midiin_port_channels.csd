<CsTest>
description = "midiin, pgmchn and ctlchn read the channels of every MIDI port"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 -F midiin_ports_empty.mid
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 1
nchnls = 1

massign 0, 0

gkNote init 0
gkKey init 0
gkVelocity init 0
gkProgram init 0
gkProgramChannel init 0
gkControl init 0
gkControlChannel init 0

; Port 63's channels are 1009 to 1024: past 255, which a byte cannot hold.
instr Read
  kStatus, kChannel, kData1, kData2 midiin
  if kStatus == 144 then
    gkNote = kChannel
    gkKey = kData1
    gkVelocity = kData2
  endif
  kProgram, kProgramChannel pgmchn 1024
  if kProgram != -1 then
    gkProgram = kProgram
    gkProgramChannel = kProgramChannel
  endif
  kControl, kNumber, kControlChannel ctlchn 1009, 7
  if kControl != -1 then
    gkControl = kControl
    gkControlChannel = kControlChannel
  endif
endin

instr Play
  iFile midifileopen "midiin_ports.mid", 63
  midifileplay iFile
endin

instr Check
  if i(gkNote) != 1009 || i(gkKey) != 60 || i(gkVelocity) != 100 || \
     i(gkProgram) != 6 || i(gkProgramChannel) != 1024 || \
     i(gkControl) != 100 || i(gkControlChannel) != 1009 then
    prints "key %d at %d on %d; program %d on %d; controller 7 at %d on %d\n", \
           i(gkKey), i(gkVelocity), i(gkNote), i(gkProgram), i(gkProgramChannel), \
           i(gkControl), i(gkControlChannel)
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; A key on channel 1009, program 6 on 1024, and controller 7 at 100 on 1009.
i "Read" 0 1
i "Play" 0 .01
i "Check" 1 0
</CsScore>
<CsFileB filename="midiin_ports.mid">
TVRoZAAAAAYAAAABAeBNVHJrAAAAFACQPGQAzwUAsAdkg2CAPAAA/y8A
</CsFileB>
<CsFileB filename="midiin_ports_empty.mid">
TVRoZAAAAAYAAAABAeBNVHJrAAAABYNg/y8A
</CsFileB>
</CsoundSynthesizer>
