sr = 44100
ksmps = 100
nchnls = 2

giengine  fluidEngine
isfnum    fluidLoad "07AcousticGuitar.sf2", giengine, 1
          fluidProgramSelect giengine, 1, isfnum, 0, 0

instr 1
          mididefault     60, p3
          midinoteonkey   p4, p5

  ikey    init            p4
  ivel    init            p5

          fluidNote       giengine, 1, ikey, ivel
endin

instr 99
  imvol   init            70000
  asigl, asigr fluidOut   giengine
          outs            asigl * imvol, asigr * imvol
endin