<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o int.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

icount init 0
loop:
  inum = icount / 3
  inm  = int(inum)
  prints "integer (%f/3) = %f\\n", icount, inm
loop_lt icount, 1, 10, loop

endin
</CsInstruments>
<CsScore>

i 1 0 0
e
</CsScore>
</CsoundSynthesizer>
