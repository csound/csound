<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o avarset.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
sr=44100
ksmps=1
nchnls=2

	instr 1	; Sine Wave
ifreq = (p4 > 15 ? p4 : cpspch(p4))
iamp = ampdb(p5)

kenv adsr 0.1, 0.05, .9, 0.2

aout init 0
ksampnum init 0

kcount = 0

iperiod = sr / ifreq

i2pi = 3.14159 * 2

loopStart:

kphase = (ksampnum % iperiod) / iperiod

knewval = sin(kphase * i2pi)

	vaset knewval, kcount,aout

	ksampnum = ksampnum + 1

loop_lt kcount, 1, ksmps, loopStart

aout = aout * iamp * kenv

outs aout, aout
	endin

</CsInstruments>

<CsScore>

i1	0.0	2 440 80
e

</CsScore>

</CsoundSynthesizer>