<CsTest>
description = "UDO output copies preserve overlapping sources"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr=48000
ksmps=32
nchnls=1
0dbfs=1
giA init 3
giB init 8
gSA init "first"
gSB init "second"
giArrayA[] fillarray 1, 2
giArrayB[] fillarray 7, 9
gkA init 0
gkB init 0
gaA init 0
gaB init 0

opcode InitPair():(i,i,S,S,i[],i[])
  xout giA, giB, gSA, gSB, giArrayA, giArrayB
endop
opcode ClassicPair, ii, 0
  xout giA, giB
endop
opcode ControlPair():(k,k,a,a)
  xout gkA, gkB, gaA, gaB
endop
opcode ClassicControlPair, kkaa, 0
  xout gkA, gkB, gaA, gaB
endop
opcode LocalControlPair, kk, 0
  setksmps 1
  xout gkA, gkB
endop

instr 1
  kCount init 0
RESTART:
  giA = 3
  giB = 8
  gSA = "first"
  gSB = "second"
  giArrayA fillarray 1, 2
  giArrayB fillarray 7, 9
  giB, giA, gSB, gSA, giArrayB, giArrayA = InitPair()
  if giA != 8 || giB != 3 || strcmp(gSA, "second") != 0 || strcmp(gSB, "first") != 0 || giArrayA[0] != 7 || giArrayB[1] != 2 then
    prints "Wrong overlapping init outputs\n"
    exitnow -1
  endif
  giB, giA ClassicPair
  if giA != 3 || giB != 8 then
    prints "Wrong classic overlapping init outputs\n"
    exitnow -1
  endif
  rireturn
  kCount += 1
  if kCount == 3 then
    reinit RESTART
  endif
  gkA = kCount
  gkB = kCount + 10
  gaA = kCount
  gaB = kCount + 10
  gkB, gkA, gaB, gaA = ControlPair()
  kAudioA downsamp gaA
  kAudioB downsamp gaB
  if gkA != kCount + 10 || gkB != kCount || kAudioA != kCount + 10 || kAudioB != kCount then
    printks "Wrong overlapping control outputs\n", 0
    exitnowk -1
  endif
  gkB, gkA, gaB, gaA ClassicControlPair
  kClassicA downsamp gaA
  kClassicB downsamp gaB
  if gkA != kCount || gkB != kCount + 10 || kClassicA != kCount || kClassicB != kCount + 10 then
    printks "Wrong classic overlapping control outputs\n", 0
    exitnowk -1
  endif
  gkB, gkA LocalControlPair
  if gkA != kCount + 10 || gkB != kCount then
    printks "Wrong local-ksmps overlapping control outputs\n", 0
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
i1 0 .01
i1 .02 .01
</CsScore>
</CsoundSynthesizer>
