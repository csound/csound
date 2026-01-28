<CsoundSynthesizer>
<CsOptions>
-n -m0 
</CsOptions>
<CsInstruments>

instr 1
  ival = 1
  
  ; Standard spacing
  if (ival == 1) then
    print ival
  endif

  ; No space after if
  if(ival == 1) then
    print ival
  endif

  ; No space after if, space after paren
  if( ival == 1) then
    print ival
  endif

  ; nested parens, no space
  if((ival == 1)) then
    print ival
  endif

  ; elseif no space
  if (ival == 0) then
    print ival
  elseif(ival == 1) then
    print ival
  endif

  ; elseif no space, space after paren
  if (ival == 0) then
    print ival
  elseif( ival == 1) then
    print ival
  endif

  ; until no space
  i2 = 0
  until(i2 == 1) do
    i2 += 1
  od

  ; until no space, space after paren
  i2 = 0
  until( i2 == 1) do
    i2 += 1
  od

  ; while no space
  i2 = 0
  while(i2 < 1) do
    i2 += 1
  od

  ; while no space, space after paren
  i2 = 0
  while( i2 < 1) do
    i2 += 1
  od

endin

</CsInstruments>
<CsScore>
i 1 0 1
</CsScore>
</CsoundSynthesizer>
