
<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>
;run this example in CsoundQt, a Csound editor that provides widgets
;make the Widgets-panel visible, by clicking the Widgets symbol in the menu or pressing (Alt+1).

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1
; written by Andres Cabrera
instr 1

kfreq invalue "freq" ; Quotes are needed here
asig  oscil 0.1, kfreq, 1
      outs asig, asig

endin
</CsInstruments>
<CsScore>
f 1 0 1024 10 1 ;sine
i 1 0 300 	;play for 300 seconds
e
</CsScore>
</CsoundSynthesizer>

