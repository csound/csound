<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs=1
nchnls=1

gimf midifileopen "seek_tempo.mid", 1

instr 1
  ; Seeking before the first tempo event should keep the implied default tempo.
  midifilepos 0.25, gimf
  itempo miditempo gimf
  if int(itempo + 0.5) != 120 then
    print itempo
    exitnow(-1)
  endif

  ; Seeking after the tempo change should restore that later tempo.
  midifilepos 1.0, gimf
  itempo miditempo gimf
  if int(itempo + 0.5) != 60 then
    print itempo
    exitnow(-1)
  endif
endin

schedule(1, 0, 0)

</CsInstruments>
<CsScore>
f0 0.1
</CsScore>
</CsoundSynthesizer>
