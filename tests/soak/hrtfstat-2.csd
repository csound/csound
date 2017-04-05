<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o hrtfstat-2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

iAz   = p4
iElev = p5

itim = ftlptim(1)
;    transeg a   dur   ty  b  dur    ty  c    dur   ty  d
kamp transeg 0, p3*.1, 0, .9, p3*.3, -3, .5,  p3*.3, -2, 0
ain  loscil3 kamp, 50, 1
aleft,aright hrtfstat ain, iAz, iElev, "hrtf-44100-left.dat","hrtf-44100-right.dat"
     outs aleft, aright

endin
</CsInstruments>
<CsScore>
f 1 0 0 1 "Church.wav" 0 0 0 ;Csound computes tablesize

;      Azim Elev
i1 0 7  90   0  ;to the right
i1 3 7 -90  -40 ;to the left and below
i1 6 7 180   90 ;behind and up
e 
</CsScore>
</CsoundSynthesizer>