<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o space_stereo.wav -W ;;; for file output any platform
</CsOptions> 
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2
  
ga1 init 0
ga2 init 0

instr 1

kx   = p4
ky   = p5
asig diskin2 "beats.wav", 1
a1, a2, a3, a4 space asig, 0, 0, .1, kx, ky	;take position values from p4, p5
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
;place the sound in the left speaker and near
i1 0 1 -1 1
;place the sound in the right speaker and far
i1 1 1 45 45
;place the sound equally between left and right and in the middle ground distance
i1 2 1 0 12

i 99 0 7	;keep reverb active
e
</CsScore>
</CsoundSynthesizer>
