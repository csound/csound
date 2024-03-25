<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o cross2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
; after example from Kevin Conder
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1	;play audio file

aout soundin "fox.wav"
     outs aout, aout
endin

instr 2	;cross-synthesize

icps = p4
ifn  = p5		; Use the "ahhc.aiff" sound and "eeec.aiff"
ain1 oscil 0.6, p4, ifn
ain2 soundin "fox.wav"	; Use the "fox.wav" as modulator

  isize = 4096
  ioverlap = 2
  iwin = 3
  kbias init 1

aout cross2 ain1, ain2, isize, ioverlap, iwin, kbias
     outs aout, aout
endin

</CsInstruments>
<CsScore>
;audio files
f 1 0 128 1 "ahhc.aiff" 0 4 0
f 2 0 128 1 "eeec.aiff" 0 4 0

f 3 0 2048 20 2	;windowing function
 
i 1 0 3

i 2 3 3  50	1 ;"eeec.aiff"
i 2 + 3  50	2 ;"ahhc.aiff"
i 2 + 3  100	1 ;"eeec.aiff"
i 2 + 3  100	2 ;"ahhc.aiff"
i 2 + 3  250	1 ;"eeec.aiff"
i 2 + 3  250	2 ;"ahhc.aiff"
i 2 + 3  20	1 ;"eeec.aiff"
i 2 + 3  20	2 ;"ahhc.aiff"
e

</CsScore>
</CsoundSynthesizer>