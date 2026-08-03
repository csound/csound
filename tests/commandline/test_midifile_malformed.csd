<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
ksmps = 32
nchnls = 1

; the first malformed opens run against an empty MIDI file list
iOneByte midifileopen "malformed_midi_one_byte.mid"
iTruncated midifileopen "malformed_midi_truncated.mid"

; and this one against a non-empty list
iValid midifileopen "catherine.mid"
iBadHeader midifileopen "malformed_midi_bad_header.mid"

if iOneByte != -1 || iTruncated != -1 || iBadHeader != -1 || iValid < 0 then
  exitnow(-1)
endif
</CsInstruments>
<CsScore>
e 0.1
</CsScore>
</CsoundSynthesizer>
