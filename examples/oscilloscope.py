#!/usr/bin/python
#####################################
#  oscilloscope example
#  
#   VL, 01/07

import csnd
from Tkinter import *
import display
import array

# window size, refresh interval and norm factor
window_size = 300
time_interval = 0.2
norm = 32768.0

# display callback
class Disp:

 def callb(self, dummy):
    sig = array.array('f')
    cs  = self.data[0]
    disp = self.data[1]
    chn = self.data[2]
    cs.ChanOAGet(chn.cast(), 1)
    for i in range(0,cs.GetKsmps()):
      sig.append(chn[i]/norm)
    disp.draw(sig,time_interval*cs.GetSr())

 def __init__(self,data):
      self.data = data


# create & compile instance
cs = csnd.Csound()
cs.Compile("am.csd")

# create the thread object
perf = csnd.CsoundPerformanceThread(cs)

# display object
master = Tk()
disp = display.Oscilloscope(master, window_size, perf.Stop, "green", "black")

# samples array
chn = csnd.floatArray(cs.GetKsmps())
dat = (cs,disp,chn)
tes = Disp(dat)

# set the callback
perf.SetProcessCallback(tes.callb, None)

# play
perf.Play()

# run the display
disp.mainloop()





