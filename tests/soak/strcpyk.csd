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

seed 0

instr 1 
;get one element of the input string whenever the metro
;triggers, and call a subinstrument to play the file

Smember   strget    p4
istrlen   strlen    Smember
kprint    init      0
ktrig     metro     .6 

if ktrig == 1 then                              ;whenever the trigger gives signal
 kel       random    0, 3.9999                  ;choose a random element (0, 1 or 2)
 kel       =         int(kel)
 Scopy     strcpyk   Smember                    ;make a copy for leaving Smember intact 
 kndx      =         0                          ;set the initial index for reading substrings
 kcount    =         0                          ;set counter for searching the element
 
 loop:                                          ;start looping over the elements in Smember
 kdelim    strindexk Scopy, ":"
  
  if kdelim > 0 then                            ;as long as ":" occurs in Scopy, do:
   if kel == kcount then                        ;if this is the element to get
     Sfile     strsubk   Scopy, kndx, kdelim    ;read it as substring
     kprint = kprint+1
          kgoto     call                        ;and jump out
   else                                         ;if not
    Scopy     strsubk   Scopy, kdelim+1, istrlen ;cut off this element from Scopy
   endif 
    kcount   =  kcount+1                        ;if no element has been found,go back to loop 
          kgoto     loop                        ;and look for the next element
  else                                          ;if there is no delimiter left, the rest is the searched element
   Sfile     strcpyk   Scopy
  endif
 call:                                          ;print the result, call the subinstrument and play the file
          printf    "kel = %d, file = '%s'\n", ktrig+kprint, kel, Sfile
 S_call   sprintfk  {{i 2 0 1 "%s"}}, Sfile
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

i 1 0 30 "marimba.aif:fox.wav:drumsMlp.wav:flute.aiff"
e
</CsScore>
</CsoundSynthesizer>
