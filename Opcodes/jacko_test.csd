<CsoundSynthesizer>
<CsOptions>
csound -m255 -RWfo jacko_test.wav
</CsOptions>
<CsInstruments>

sr = 48000
ksmps = 128
nchnls = 2
0dbfs = 1

JackInit "default", "csound"
JackAudioInConnect "aeolus:out.L", "leftin"
JackAudioInConnect "aeolus:out.R", "rightin"
JackMidiOutConnect "midiout", "aeolus:Midi/in"
; Note that Jack enables audio to be output to a regular
; Csound soundfile and, at the same time, to a sound 
; card in real time via the system client. 
JackAudioOutConnect "leftout", "system:playback_1"
JackAudioOutConnect "rightout", "system:playback_2"
JackInfo
JackFreewheel 1
JackOn

alwayson "jackin"

instr 1
ichannel = p1
itime = p2
iduration = p3
ikey = p4
ivelocity = p5
JackNoteOut "midiout", ichannel, ikey, ivelocity
print itime, iduration, ichannel, ikey, ivelocity
endin

instr jackin
aleft JackAudioIn "leftin"
aright JackAudioIn "rightin"
;JackAudioOut "leftout", aleft
;JackAudioOut "rightout", aright
outs  aright, aleft
endin

</CsInstruments>
<CsScore>
i 1 1 30 60 60
i 1 2 30 64 60
i 1 3 30 71 60
e 2
</CsScore>
</CsoundSynthesizer>

