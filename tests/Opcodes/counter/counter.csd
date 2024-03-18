<CsoundSynthesizer>
<CsOptions>
-odac -Mhw:1,0,0
</CsOptions>

<CsInstruments>
 gicnt cntCreate 1     ; a toggle
 gicntNote cntCreate 1 ; ignore note-off message to turn MIDI notes into toggles


instr 1

kkey sensekey

inote notnum

if (kkey == 97) then
 k1 count gicnt
 if k1==0 then
  event "i", 2, 0, -1
 else
  event "d", 2, 0, -1
 endif
endif

print  inote
if (inote == 60) then
 i2 count_i gicntNote
 print i2
 if i2==0 then
  event_i "i", 3, 0, -1
 else
  event_i "d", 3, 0, -1
 endif
endif
endin

instr 2
asig oscil 10000, 440
out asig
endin

instr 3
asig oscil 5000, 880
out asig
endin

</CsInstruments>


<CsScore>
i1 0 z
e

</CsScore>

</CsoundSynthesizer>
