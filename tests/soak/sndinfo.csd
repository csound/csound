<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   
; For Non-realtime ouput leave only the line below:
; -o sndinfo.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 48000  ; sample rate of drumsMlp.wav = 44100
ksmps = 32  ; and will be resampled to 48000
nchnls = 2
0dbfs  = 1

; by Menno Knevel 2021

; resample in the highest qualty, name the new sample 'beats48.wav'
gires system_i 1,{{ src_conv -r48000 -Q5 drumsMlp.wav -o beats48.wav }}

instr 1
ires system_i 1,{{ sndinfo -i beats48.wav }} ; check sample rate
aout    diskin2 "beats48.wav", 1
outs    aout, aout  
endin

</CsInstruments>
<CsScore>
i1 0 2              

e
</CsScore>
</CsoundSynthesizer>