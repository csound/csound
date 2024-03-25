<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o event_i.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

seed 0
gisine ftgen 0, 0, 2^10, 10, 1

instr 1 ;master instrument

ininstr = 10 ;number of called instances
indx = 0
loop:
ipan  random 0, 1
ifreq random 100, 1000
iamp  = 1/ininstr
event_i "i", 10, 0, p3, iamp, ifreq, ipan
loop_lt indx, 1, ininstr, loop

endin

instr 10

      print p4, p5, p6
ipeak random 0, 1 ;where is the envelope peak
asig  poscil3 p4, p5, gisine
aenv  transeg 0, p3*ipeak, 6, 1, p3-p3*ipeak, -6, 0
aL,aR pan2 asig*aenv, p6
      outs aL, aR

endin

</CsInstruments>
<CsScore>
i1 0 10
i1 8 10
i1 16 15
e
</CsScore>
</CsoundSynthesizer>
