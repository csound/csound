<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs=1
nchnls=1
ksmps=64

; Load a MIDI file with its own tempo map.
gimf midifileopen "rain.mid", 1

instr 98
  ; Override the file tempo before playback starts.
  midifiletempo 60, gimf
  midifileplay gimf
endin

instr 99
  ; Confirm miditempo reports the overridden tempo during playback.
  itp miditempo gimf
  if int(itp) != 60 then
    print itp
    exitnow(-1)
  endif
endin

schedule(98, 0, 0)
schedule(99, 0.05, 0)

</CsInstruments>
<CsScore>
f0 1
</CsScore>
</CsoundSynthesizer>
