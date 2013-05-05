# This file demonstrates starting Csound without any orc/sco/csd.  
# After configuring Csound, the program start Csound for rendering,
# then uses csoundCompileOrc to add new a new instrument and call
# the event_i opcode to run it.

import csnd6
import time

cs = csnd6.csoundCreate(None)
csnd6.csoundSetOption(cs,"-odac")
csnd6.csoundStart(cs)
perf = csnd6.CsoundPerformanceThread(cs)
perf.Play()

csnd6.csoundCompileOrc(cs, '''
event_i "i",1,0.1,1,1000,500
instr 1
k1 expon 1,p3,0.001
a2 oscili k1*p4,p5,-1
out a2
endin ''')

time.sleep(5)
perf.Stop()
