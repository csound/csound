<CsoundSynthesizer>
<CsOptions>
--nosound

</CsOptions>

<CsInstruments>

/* strstrip: strip whitespace from string

 Sout strstrip Sin [, Smode]

 Args
   Sin - string to strip whitespace from
   Smode - if not given, whitespace is stripped from left and right edges
           "l" - strip whitespace from left edge only
           "r" - strip whitespace from right edge only
 */

instr 1
  Sout = strstrip("  \t\n  foo bar   \t\n")
  ; Sout = strstrip("  center   ", "l")
  ; Sout = strstrip("  center   ", "r")
    
  prints "string: '%s'\n", Sout
  turnoff
endin

</CsInstruments>

<CsScore>

i1 0 1

</CsScore>
</CsoundSynthesizer>
