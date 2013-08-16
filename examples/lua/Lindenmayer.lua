-- Copyright (c) 2002, 2003 by Michael Gogins. All rights reserved.
-- Tutorial demonstrating a MusicModel composition based on a Lindenmayer system.

require "luaCsnd6"
require "luaCsoundAC"

filename = 'Lindenmayer.lua'
model = luaCsoundAC.MusicModel()
lindenmayer = luaCsoundAC.Lindenmayer()
lindenmayer:setAxiom("Ts=425 b")
lindenmayer:setAngle(2.0 * math.pi / 0.5)
lindenmayer:addRule("b", " b [ Ts*16 Ti-1 a N b ] Tt+1 Tk-3.01 a N b Tt+3 N Tt+1.5 Tk+2.5 b [ Ti+1 a b ] N")
lindenmayer:addRule("a", " N Tt+1.251 Tk+1 N [ Tk+2 b ] Tk+4.1 N Tk-3 Tt-1 [ Tt+1 Tk-4 [ a ] N ] N ")
lindenmayer:setIterationCount(5)
lindenmayer:generate()
random = luaCsoundAC.Random()
random:createDistribution("uniform_real")
random:setElement(7, 11, 1)
rescale = luaCsoundAC.Rescale()
rescale:setRescale( 0, true, true,  0,     240)
rescale:setRescale( 1, true, true,  2,       4)
rescale:setRescale( 3, true, true,  2,       8)
rescale:setRescale( 4, true, true,  36,      60)
rescale:setRescale( 5, true, true,  20,      10)
rescale:setRescale( 7, true, true,  -0.9875,    1.875)
scales =  {'Dm', 'Fm', 'AM', 'G7', 'Em', 'FM'}
pcsForScales = {}
for i,scale in ipairs(scales) do
    scalenumber = luaCsoundAC.Conversions_nameToM(scale)
    pcsForScales[scale] = scalenumber
    print(scale .. "="  .. scalenumber)
end
random:addChild(lindenmayer)
rescale:addChild(random)
model:addChild(rescale)
model:generate()
score = model:getScore()
print('Events in generated score: '..score:size())
model:setConformPitches(true)
csound = model:getCppSound()
csound:load('../CsoundAC.csd')
csound:setCommand("csound -m3 -RWZdfo " .. filename .. ".wav ")
csound:setFilename(filename)
duration = score:getDuration()		
print('Duration: '.. duration)
score:arrange(0, 7)
score:arrange(1, 5)
score:arrange(2, 13)
score:arrange(3, 10)
score:arrange(4, 14)
score:arrange(5, 7)
score:arrange(6, 15)
score:arrange(7, 19)
model:createCsoundScore([[
; EFFECTS MATRIX

; Chorus to Reverb
i 1 0 0 200 210 0.0
; Chorus to Output
i 1 0 0 200 220 0.05
; Reverb to Output
i 1 0 0 210 220 2.0

; SOUNDFONTS OUTPUT

; Insno Start   Dur     Key 	Amplitude
i 190 	0       -1      0	64.

; MASTER EFFECT CONTROLS

; Chorus.
; Insno	Start	Dur	Delay	Divisor of Delay
i 200   0       -1      10      30

; Reverb.
; Insno	Start	Dur	Level	Feedback	Cutoff
i 210   0       -1      0.81    0.0  		16000

; Master output.
; Insno	Start	Dur	Fadein	Fadeout
i 220   0       -1      0.1     0.1

]])
print(csound:getCommand())
csound:perform()

















































