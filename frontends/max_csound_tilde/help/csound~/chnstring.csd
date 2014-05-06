<CsoundSynthesizer>
<CsInstruments>
sr      = 44100
ksmps   = 8
nchnls  = 2

chn_S "my_string", 3 ; 3 = 1 (input) + 2 (output)

instr 1
	Sfoo = "Hello World"     ; Init Sfoo.
	chnset Sfoo, "my_string" ; Set "my_string" = Sfoo (i-time only).
endin

instr 2
	; i-time only (i.e. only sets "my_string" once per instrument instance).
	chnset "Aaaahhhhchoo!", "my_string" 
endin

instr 3
	Sfoo chnget "my_string" ; Store value "my_string" in Sfoo (i-time only).
	puts Sfoo, 1            ; Print value of Sfoo to Max window (i-time only).
endin


</CsInstruments>
<CsScore>
f0 86400
;i1 0 86400
e
</CsScore>
</CsoundSynthesizer>







