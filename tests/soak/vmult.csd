<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o cigoto.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>


sr=44100
kr=4410
ksmps=10
nchnls=2

	instr 1 ;table playback
ar lposcil 1, 1, 0, 262144, 1
outs ar,ar
	endin

	instr 2
vcopy 2, 1, 40000 ;copy vector from sample to empty table
vmult 5, 10000, 262144 ;scale noise to make it audible
vcopy 1, 5, 40000 ;put noise into sample
turnoff
	endin

	instr 3
vcopy 1, 2, 40000 ;put original information back in
turnoff
	endin

</CsInstruments>
<CsScore>
f1  0 262144   -1 "beats.wav" 0 4 0
f2  0 262144   2  0

f5  0 262144   21  3 30000

i1 0 4
i2 3 1

s
i1 0 4
i3 3 1
s

i1 0 4

</CsScore>
</CsoundSynthesizer>
