<CsTest>
description = "midiarp stops triggering after a zero-velocity note-on releases the key"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 -F arp_release.mid
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 1
nchnls = 1
massign 0, 0
gkTriggers init 0
gkReleased init 0

instr 1
  kStatus, kChannel, kKey, kVelocity midiin
  kHeld init 0
  if kStatus == 144 then
    kHeld = (kVelocity != 0 ? 1 : 0)
    if kVelocity == 0 then
      gkReleased = 1
    endif
  endif
  kNote, kTrigger midiarp 20, 1
  if kTrigger != 0 then
    if kHeld == 0 || kNote != 60 then
      exitnowk -1
    endif
    gkTriggers += 1
  endif
endin

instr 2
  if i(gkTriggers) != 2 || i(gkReleased) != 1 then
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .4
i 2 .4 .001
</CsScore>
<CsFileB filename="arp_release.mid">
TVRoZAAAAAYAAAABAfRNVHJrAAAADQqQPGRakDwAgxD/LwA=
</CsFileB>
</CsoundSynthesizer>
