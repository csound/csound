<CsTest>
description = "MIDI file transport stops only the selected port's notes"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-F -n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 1
nchnls = 1

giBase midifileopen "port_transport.mid", 0
giSeek midifileopen "port_transport.mid", 1
giRewind midifileopen "port_transport.mid", 4
giMute midifileopen "port_transport.mid", 16
giPause midifileopen "port_transport.mid", 63

; Each file plays notes on its first and last channel.
instr 1
endin
instr 16
endin
instr 17
endin
instr 32
endin
instr 65
endin
instr 80
endin
instr 257
endin
instr 272
endin
instr 1009
endin
instr 1024
endin

instr 2000
  midifileplay giBase
  midifileplay giSeek
  midifileplay giRewind
  midifileplay giMute
  midifileplay giPause
endin

instr 2001
  midifilepos 2, giSeek
  midifilerewind giRewind
  midifilemute giMute
  midifilepause giPause
endin

instr 2002
  kFirst active p4
  kLast active p4 + 15
  if kFirst != p5 || kLast != p5 then
    printf "Port check failed: channel %g, first=%g, last=%g, expected=%g\n", 1, p4, kFirst, kLast, p5
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
i 2000 0 .001
i 2002 .6 .001 1 1
i 2002 .6 .001 17 1
i 2002 .6 .001 65 1
i 2002 .6 .001 257 1
i 2002 .6 .001 1009 1
i 2001 .7 .001
i 2002 .71 .001 1 1
i 2002 .71 .001 17 0
i 2002 .71 .001 65 0
i 2002 .71 .001 257 0
i 2002 .71 .001 1009 0
e .72
</CsScore>
<CsFileB filename="port_transport.mid">
TVRoZAAAAAYAAAABAeBNVHJrAAAAFoNgkDxkAJ9AZINggDwAAI9AAAD/LwA=
</CsFileB>
</CsoundSynthesizer>
