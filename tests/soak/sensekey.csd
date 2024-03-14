<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac -d   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
</CsOptions>
<CsInstruments>

; By Stefano Cucchi - 2020

; Initialize the global variables.
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1


instr 1

; Instr 1 trigger instr 2: every time you press any key, instr 2 produce a note.
; The pitch of the tone produced by instr 2 is equal to the ASCII code of the key pressed.

gkNumber, gkPress sensekey 
 
if changed(gkPress) == 1 then
      if (gkPress == 1) then
          event "i", 2, 0, 0.3
      endif 
endif      
endin

instr 2
  
iCps init i(gkNumber)
print iCps
asig oscili 0.2, iCps, 1 
kenvelope linseg 0, 0.1, 1, 0.1, 1, 0.1, 0
outs asig*kenvelope, asig*kenvelope
endin

</CsInstruments>
<CsScore>
f 1 0 4096 10 1 1 0.3 0 0.2 0.3 0.5

i 1 0 2000
e
</CsScore>
</CsoundSynthesizer>
