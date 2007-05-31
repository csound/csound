<CsoundSynthesizer>
<CsOptions>
csound -opyrun.wav temp.orc temp.sco
</CsOptions>
<CsInstruments>
sr=44100
kr=4410
ksmps=10
nchnls=1

pyinit

pyruni "import random"

instr 1
        ; This message is stored in the main namespace
        ; and is the same for every instance
        pyruni  "message = 'a global random number: %f' % random.random()"
        pyrun   "print message"

        ; This message is stored in the private namespace
        ; and is different for different instances
        pylruni "message = 'a private random number: %f' % random.random()"
        pylrun  "print message"

endin
</CsInstruments>
<CsScore>
i1 0 0.1

</CsScore>
</CsoundSynthesizer>
