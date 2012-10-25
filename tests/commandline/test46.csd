<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

k1 expon 440, p3/10, 880        ;begin gliss and continue
  printk .1, k1                ;prints frequency every tenth of a second
if (k1 >= sr/2) then        ;until Nyquist detected
  turnoff                      ;then quits
else
 asig oscil 1, k1, 1
      outs asig, asig
endif

endin

</CsInstruments>
<CsScore>
f 1 0 32768 10 1        ;sine

i 1 0 4                 ;plays for 4 seconds, but gets turned off after 2.3 seconds
e
</CsScore>
</CsoundSynthesizer>
