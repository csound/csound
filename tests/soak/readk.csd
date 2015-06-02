<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o readk.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; Initialize the global variables.
sr = 44100
kr = 4410
ksmps = 10
nchnls = 1

0dbfs = 1
; By Andres Cabrera 2008

instr 1
; Read a number from the file every 0.5 seconds
  kfibo readk "fibonacci.txt", 7, 0.5
  kpitchclass = 8 +  ((kfibo % 12)/100)
  printk2 kpitchclass
  kcps = cpspch( kpitchclass )
  printk2 kcps
  a1 oscil 0.5, kcps, 1
  out a1
endin


</CsInstruments>
<CsScore>
f 1 0 1024 10 1
i 1 0 10
e


</CsScore>
</CsoundSynthesizer>
