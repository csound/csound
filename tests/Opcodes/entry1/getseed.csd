<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o getseed.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; by Menno Knevel - 2021

seed 0    ; use seed from system clock

instr 1   ; different values every time - value is derived from system clock

krnd  randomh 100, 400, 1		
      printks2    "different values every time Value = %d\n ",  krnd	
aout  poscil 0.8, 440+krnd
      outs aout, aout
endin

instr 2   ; same values every time - captured by getseed

gised getseed                 ; capture the clock seed value and make it a global value
seed  gised                   ; so that it can be used in all instruments
print gised
krnd  randomh 100, 400, 1		
      printks2    "get seed value from time clock. Value = %d\n ",  krnd	
aout  poscil 0.8, 440+krnd
      outs aout, aout
endin

instr 3	; same values every time - captured by getseed

print gised                   ; check the global seed value
seed  gised                   ; let the seed get this value
krnd  randomh 100, 400, 1		
      printks2    "re-uses seed value of instr 2. Value = %d\n ",  krnd	
aout  poscil 0.8, 440+krnd
      outs aout, aout
endin


</CsInstruments>
<CsScore>

i1 0 3  ; 2 notes showing the different
i1 5 3

i2 10 3 ; 1 note & get the seed value

i3 15 3 ; 1 note and use that seed value again
e
</CsScore>
</CsoundSynthesizer>
