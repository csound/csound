from Tkinter import *
import time
import math
import array

class Oscilloscope(Frame):
                
    def createCanvas(self):
        self.canvas = Canvas(self,height=self.size,width=self.size,bg=self.bg)
        self.canvas.pack()

    def draw(self,what,samples):
        for i in what:
          self.data.append(i);
        s = len(self.data)
        if s >= samples:
            count = 0
            incr = (s/self.size)
            j = 0
            while(j < s):
              if(count >= self.size): break
              self.canvas.delete(self.item[count])
              y = self.data[j]*self.size/2 + self.size/2
              self.item[count] = self.canvas.create_line(count, self.prev, count+1, y, fill=self.line)
              self.prev = y       
              count += 1
              j += incr
            del self.data[0:s]
      
    def quit(self):
        self.close();
        time.sleep(0.1)
        self.master.destroy()

    def __init__(self,master, size, end, line="black", bg="white"):
        master.title("Oscilloscope")
        self.size = size
        self.close = end
        self.line = line
        self.bg = bg
        self.data = array.array('f')
        Frame.__init__(self,master)
        self.pack()
        self.createCanvas()
        self.item = array.array('i')
        for i in range(0, self.size):
           self.item.append(0)
        self.prev = 0
        self.n = 0
        self.master = master
        self.master.protocol("WM_DELETE_WINDOW", self.quit)
