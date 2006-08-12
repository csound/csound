<CsoundSynthesizer>
<CsOptions>
--midi-key=4 --midi-velocity=5
</CsOptions>
<CsInstruments>

; Sample frames per second.

sr = 44100

; Sample frames per control period.

ksmps = 100

; Audio output channel.

nchnls = 1

; Output amplitudes will be normalized so 32767 is maximum.

0dbfs = 32767

; Allocate global sine wave table for oscillators.

giafno ftgen 1,          0,      65537,  10,     1 

instr 1
			; Note duration is -1 for MIDI (note stays on till turned off), or else comes from the score.
                        mididefault             -1, p3
			; p4 is MIDI key translated to linear octaves, p5 is MIDI velocity, or else comes from the score.
                        midinoteonoct           p4, p5
			; Print pfield values.
                        print                   p2, p3, p4, p5
			; Simple sine wave instrument.
a1                      oscili                  ampdb(p5), cpsoct(p4), giafno
                        out                     a1
endin

</CsInstruments>
</CsoundSynthesizer>
