instr 1
k1 chnget "amp"
k2 chnget "freq"
a1 vco2 p4*port(k1,0.01), p5*port(k2,0.02)
   out a1

endin