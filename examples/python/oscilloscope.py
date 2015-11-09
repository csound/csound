#!/usr/bin/python
#####################################
#  oscilloscope example
#  
#   VL, 01/07

import csnd6
from Tkinter import *
import display
import array

# window size, refresh interval and norm factor
window_size = 300
time_interval = .2
norm = 32768.0

# display callback
class Disp:

 def callb(self, dummy):
    sig = array.array('d')
    cs  = self.data[0]
    disp = self.data[1]
    for i in range(0,cs.GetKsmps()):
      sig.append(cs.GetSpoutSample(i,0)/norm)
    disp.draw(sig,time_interval*cs.GetSr())
   
 def __init__(self,data):
      self.data = data


# create & compile instance
cs = csnd6.Csound()
cs.Compile("am.csd")

# create the thread object
perf = csnd6.CsoundPerformanceThread(cs)

# display object
master = Tk()
disp = display.Oscilloscope(master, window_size, perf.Stop, "green", "black")

dat = (cs,disp)
tes = Disp(dat)

# set the callback
perf.SetProcessCallback(tes.callb, None)

# play
perf.Play()

# run the display
disp.mainloop()





