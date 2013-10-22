<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o nsamp.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
; slightly adapted example from Jonathan Murphy Dec 2006

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ifn   =  p4	; table number
ilen  =  nsamp(ifn)
prints "actual numbers of samples = %d\n", ilen
itrns =  1	; no transposition
ilps  =  0	; loop starts at index 0
ilpe  =  ilen	; ends at value returned by nsamp above
imode =  1	; loops forward
istrt =  0	; commence playback at index 0 samples
; lphasor provides index into f1 
alphs lphasor itrns, ilps, ilpe, imode, istrt
atab  tablei  alphs, ifn
      outs atab, atab

endin
</CsInstruments>
<CsScore>
f 1 0 262144 1 "kickroll.wav" 0 4 1	;stereo file in table, with lots of zeroes
f 2 0 262144 1 "fox.wav" 0 4 1		;mono file in table, with lots of zeroes

i1 0 10 1
i1 + 10 2
e

</CsScore>
</CsoundSynthesizer>