<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o spsend.wav -W ;;; for file output any platform
</CsOptions> 
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2	;stereo output
  
ga1 init 0
ga2 init 0

instr 1	;sends different amounts to reverb

irev = p6
asig diskin2 "fox.wav", 1
a1, a2, a3, a4 space asig, 0, 0, irev, p4, p5	;take position values from p4, p5
ar1, ar2, ar3, ar4 spsend			;send to reverb

ga1 = ga1+ar1
ga2 = ga2+ar2
    outs a1, a2

endin

instr 99 ; reverb instrument

a1 reverb2 ga1, 2.5, .5
a2 reverb2 ga2, 2.5, .5
   outs	a1, a2

ga1=0
ga2=0

endin

</CsInstruments>
<CsScore>
;WITH REVERB
;place the sound in the left speaker and near
i1 0 1 -1 1 .1
;place the sound in the right speaker and far
i1 1 1 45 45 .1
;place the sound equally between left and right and in the middle ground distance
i1 2 1 0 12 .1

;NO REVERB
;place the sound in the left speaker and near
i1 6 1 -1 1 0
;place the sound in the right speaker and far
i1 7 1 45 45 0
;place the sound equally between left and right and in the middle ground distance
i1 8 1 0 12 0

i 99 0 12	;keep reverb active all the time
e
</CsScore>
</CsoundSynthesizer>
