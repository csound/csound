sr=44100
kr=4410
ksmps=10
nchnls=1

instr 1

	k1	rand		1000
		pyassign	"a", k1
		pyrun		"print a"
endin
