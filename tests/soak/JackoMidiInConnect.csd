<CsoundSynthesizer>
<CsOptions>
-M0 -+rtmidi=null 
</CsOptions>
<CsInstruments>

sr     = 48000
ksmps  = 128
nchnls = 2
0dbfs  = 1

; by Menno Knevel - 2023
; The example shows how to connect a Midi port (Edirol) to the Midi In of Instrument 1

JackoInit   "default", "csound6"		       	        ; Csound as a Jack client
JackoMidiInConnect "UM-3:midi/playback_2", "midiinEDIROL" 	; create 1 Midi in port

instr 1	; get notes to the EDIROL Midi port	
ifreq   cpsmidi
iamp	ampmidi .7
aout    vco2    iamp, ifreq
outs aout, aout
endin

</CsInstruments>
<CsScore>

i1 1 30
e 
</CsScore>
</CsoundSynthesizer>
