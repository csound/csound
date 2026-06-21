<CsoundSynthesizer>
<CsOptions>
-F -n
</CsOptions>
<CsInstruments>
0dbfs=1

gimf midifileopen "rain.mid", 1
print midifilelen(gimf)
print miditempo(gimf)

instr 2
midifilemute gimf
endin

instr 3
; Rewind should restore the file's initial tempo before playback resumes.
midifilerewind gimf
itempo miditempo gimf
if int(itempo) != 78 then
   print itempo
   exitnow(-1)
endif
midifileplay gimf
endin

instr 1,17,18,19,20,21,22,23,24,25,26,27,28,29,30
midifileplay gimf
iamp ampmidi 0.5
icps cpsmidi
asig vco2 iamp, icps
a2 madsr 0.001, 0.1, 0.5, 0.1
   out asig*a2*0.01
endin

schedule(17,1,0)
schedule(2,2,0)
schedule(2,3,0)
schedule(3,midifilelen(gimf)+1,0)
event_i("e", 0, 2*midifilelen(gimf)+2)

</CsInstruments>
<CsScore>
</CsScore>
</CsoundSynthesizer>
