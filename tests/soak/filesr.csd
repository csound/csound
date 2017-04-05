<CsoundSynthesizer> 
<CsOptions> 
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o filesr.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

;after an example from Jonathan Murphy

instr 1 
;load sound into an ftable
  Sfile	    strcpy    "beats.wav" 
  ilen	    filelen   Sfile 
  isr	    filesr    Sfile 
  isamps    =  ilen * isr 
;adjust the length of the table to be a power of two closest
;to the actual size of the sound 
  isize	    init      1 
loop: 
  isize	    =  isize * 2 
  if (isize < isamps) igoto loop 
  itab	    ftgen     0, 0, isize, 1, Sfile, 0, 0, 0 
prints  "sample rate = %f, size = %f\n", isr, isize ;prints them

endin 
</CsInstruments> 
<CsScore> 
i1 0 2
e 
</CsScore> 
</CsoundSynthesizer> 