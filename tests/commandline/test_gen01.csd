<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

0dbfs = 1

;; Raw Input
;
; Writing raw bytes from this file to wavetables.
; This way, we do not have to keep track of a separate file to read.
;
table_number@global:i = init:i(0)
;
table_number = ftgen(0, 0, 0, 1, "test_gen01.csd", 0, -1, 0)
table_number = ftgen(0, 0, 0, 1, "test_gen01.csd", 0, -2, 0)
table_number = ftgen(0, 0, 0, 1, "test_gen01.csd", 0, -3, 0)
table_number = ftgen(0, 0, 0, 1, "test_gen01.csd", 0, -4, 0)
table_number = ftgen(0, 0, 0, 1, "test_gen01.csd", 0, -5, 0)
table_number = ftgen(0, 0, 0, 1, "test_gen01.csd", 0, -6, 0)
table_number = ftgen(0, 0, 0, 1, "test_gen01.csd", 0, -7, 0)
table_number = ftgen(0, 0, 0, 1, "test_gen01.csd", 0, -8, 0)
table_number = ftgen(0, 0, 0, 1, "test_gen01.csd", 0, -9, 0)


</CsInstruments>
<CsScore>
e 0
</CsScore>
</CsoundSynthesizer>
