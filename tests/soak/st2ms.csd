<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o st2ms.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; by  Menno Knevel - 2021
; sample = ambient sound of waterfall + animals (monkey)

instr 1 
al, ar    diskin2    "stereoJungle.wav", 1          ; stereo sample
am, as st2ms  al,ar
fout "MSJungleEncoded.wav", -1, am *.6, as *.6      ; write MS encoded audio file to disk
outs am *.6, as *.6                                 ; & reduce volume a bit

endin

instr 2 
al, ar    diskin2    "MSJungleEncoded.wav", 1       ; get back in the MS encoded sample
am, as ms2st  al,ar, p4
outs am, as                               

endin

</CsInstruments>
<CsScore>

i1 0 6.7          ; encode signal, write to disk

i2 10 6.7 .5      ; read encoded file from disk and decode 1:1
i2 17 6.7 .7      ; decode but with more width

e
</CsScore>
</CsoundSynthesizer>
