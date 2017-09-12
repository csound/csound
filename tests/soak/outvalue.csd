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
; written by Alex Hofmann

instr 1	;move fader

kMoveUp  linseg 0, 3, 1, 1, 1, 0.5, 0
outvalue "movefader", kMoveUp
endin

</CsInstruments>
<CsScore>
i 1 0 5
e
</CsScore>
</CsoundSynthesizer>

