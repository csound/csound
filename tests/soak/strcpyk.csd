<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o strcpyk.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

seed 32

instr 1 
;get one element of the input string whenever the metro
;triggers, and call a subinstrument to play the file

Smember   strget    p4
istrlen   strlen    Smember
kprint    init      0
ktrig     metro     .6 

 ;whenever the trigger gives signal
 if ktrig == 1 then
  ;choose a random element (0, 1 or 2)
kel       random    0, 3.9999
kel       =         int(kel)
 ;make a copy for leaving Smember intact
Scopy     strcpyk   Smember 
 ;set the initial index for reading substrings
kndx      =         0 
 ;set counter for searching the element
kcount    =         0
 ;start looping over the elements in Smember
loop:
kdelim    strindexk Scopy, ":"
  ;as long as ":" occurs in Scopy, do:
  if kdelim > 0 then
   ;if this is the element to get
   if kel == kcount then
    ;read it as substring
Sfile     strsubk   Scopy, kndx, kdelim
kprint = kprint+1
    ;and jump out
          kgoto     call
   ;if not
   else
    ;cut off this element from Scopy
Scopy     strsubk   Scopy, kdelim+1, istrlen
   endif
   ;if no element has been found,go back to loop 
   ;and look for the next element
kcount    =         kcount+1
          kgoto     loop
  ;if there is no delimiter left, the rest is the searched element
  else
Sfile     strcpyk   Scopy
  endif
call:
 ;print the result, call the subinstrument and play the file
          printf    "kel = %d, file = '%s'\n", ktrig+kprint, kel, Sfile
S_call    sprintfk  {{i 2 0 1 "%s"}}, Sfile
          scoreline S_call, ktrig
 endif

endin

instr 2 ;play
Sfile     strget    p4
ilen      filelen   Sfile
p3        =         ilen
asig      soundin   Sfile
          outs      asig, asig
endin
</CsInstruments>
<CsScore>

i 1 0 30 "mary.wav:fox.wav:beats.wav:flute.aiff"
e
</CsScore>
</CsoundSynthesizer>
