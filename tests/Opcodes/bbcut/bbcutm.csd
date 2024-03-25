<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac             ;;;RT audio out
; For Non-realtime ouput leave only the line below:
; -o bbcutm.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; additions by Menno Knevel 2021

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1     ; Play an audio file normally.

asource soundin "drumsMlp.wav"
outs asource, asource

endin

instr 2     ; Cut-up an audio file

ibps = 4
isubdiv = 8
ibarlength = 4
iphrasebars = 1
inumrepeats = p4
asource diskin2 "drumsMlp.wav", 1, 0 ,1
a1 bbcutm asource, ibps, isubdiv, ibarlength, iphrasebars, inumrepeats
outs a1, a1

endin

</CsInstruments>
<CsScore>

i 1 0 2
;       repeats
i 2 3 8     2
i 2 12 8    6
e
</CsScore>
</CsoundSynthesizer>
