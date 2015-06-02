<CsoundSynthesizer>
<CsOptions>
-ndm0
</CsOptions>
<CsInstruments>
;example by joachim heintz 2013

  opcode ToAscii, S, S				;returns the ASCII numbers of the input string as string

Sin        xin 					;input string
ilen       strlen     Sin 			;its length
ipos       =          0 			;set counter to zero
Sres       =          "" 			;initialize output string
loop:                 				;for all characters in input string:
ichr       strchar    Sin, ipos 		;get its ascii code number
Snew       sprintf    "%d ", ichr 		;put this number into a new string
Sres       strcat     Sres, Snew 		;append this to the output string
           loop_lt    ipos, 1, ilen, loop 	;see comment for 'loop:'
           xout       Sres 			;return output string
  endop

  instr Characters

printf_i "\nCharacters:\n  given as single strings: %s%s%s%s%s%s\n", 1, "c", "s", "o", "u", "n", "d"
printf_i "  but can also be given as numbers: %c%c%c%c%c%c\n", 1, 99, 115, 111, 117, 110, 100 
Scsound ToAscii "csound"
printf_i "  in csound, the ASCII code of a character can be accessed with the opcode strchar.%s", 1, "\n"
printf_i "  the name 'csound' returns the numbers %s\n\n", 1, Scsound
  endin

</CsInstruments>
<CsScore>

i "Characters" 0 0
e
</CsScore>
</CsoundSynthesizer>
