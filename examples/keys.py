from Tkinter import *
import csnd
import math
import time

playpos = 0.55
recpos = 0.45

def Mod12(val):
    while val >= 12:
        val = val - 12
    while val < 0:
        val = val + 12
    return val

class Application(Frame):
  
    def translate(self,val):
        res = 440.0*math.pow(2, (val-69.0)/12.0)
        return res

    def play(self,event):
        base = 60  - ((self.keys/2)/7)*12
        note = event.widget.find_withtag("current")[0]
        for i in range(1,len(self.notes),2):
            if note == self.notes[i]:
                val = self.notes[i-1]    
                freq = self.translate(val+base)
                self.perf.InputMessage("i1.1 0 -1 5000 %f" % freq)
                if(self.recording):
                    self.clockstart()
                    self.st = time.time()
                    ti = time.time() - self.time
                    self.cur = [ti,0, freq]
                            
    def stop(self,event):
        base = 60  - ((self.keys/2)/7)*12
        note = event.widget.find_withtag("current")[0]
        for i in range(1,len(self.notes),2):
            if note == self.notes[i]:
                val = self.notes[i-1]    
                freq = self.translate(val+base)
                self.perf.InputMessage("i-1.1 0 1 5000 %f" % freq)
                if(self.recording):              
                    dur = time.time() - self.st
                    self.cur[1] = dur
                    self.notesrec.append(self.cur)
                
    def playstop(self,event):
     if not self.recording:
      if not self.playing:
        self.playing = True
        self.canvas.itemconfigure(self.pbutt, fill="red")
        self.playback(event)
      else:
        self.playing = False
        self.canvas.itemconfigure(self.pbutt, fill="black")

    def playback(self, event):
        if not self.recording and self.playing:
         for i in self.notesrec:
            self.perf.InputMessage("i1 %f %f 5000 %f" % (i[0], i[1], i[2]))
         if self.rectime > 0.0:
            self.master.after(int(self.rectime*1000), self.playback, event)
         else:
            self.playstop(event)
        else:
          self.canvas.itemconfigure(self.pbutt, fill="black")

    def clockstart(self):
        if self.time == 0.0:
            self.time = time.time()      

    def clockreset(self):
         self.rectime = time.time() - self.time
         self.time = 0.0
        
    def  record(self,event):
        if not self.playing:
         if not self.recording:
            del self.notesrec
            self.notesrec = []
            self.canvas.itemconfigure(self.rbutt, fill="red")
            self.recording = True        
         else:
            self.clockreset()
            self.recording = False
            self.canvas.itemconfigure(self.rbutt, fill="black")
            
         
    def createCanvas(self):
        self.height = 100
        self.keys =  49
        self.w = 12
        self.size = self.keys*self.w
        self.canvas = Canvas(self,height=self.height + 50,width=self.size,bg="white")
        self.canvas.pack()
         
    def createKeys(self):
        j = 0
        m = self.w/4
        for i in range(0,self.size, self.w):
          item = self.canvas.create_rectangle(i,0,i+self.w,self.height*0.8,fill="yellow")
          self.items.append(item)
          self.notes.append(j)
          self.notes.append(item)
          self.canvas.tag_bind(item,"<ButtonPress>", self.play)
          self.canvas.tag_bind(item,"<ButtonRelease>", self.stop)

          if (Mod12(j) == 4 or Mod12(j) == 11):  j += 1
          else:
             j += 2
        j = 0
        for i in range(0,self.size, self.w):
          if (Mod12(j) == 4 or Mod12(j) == 11):  j += 1
          else:
             item = self.canvas.create_rectangle(i+self.w-m,0,i+self.w+m,self.height*0.5,fill="black")
             self.items.append(item)
             self.notes.append(j+1)
             self.notes.append(item)
             self.canvas.tag_bind(item,"<ButtonPress>", self.play)
             self.canvas.tag_bind(item,"<ButtonRelease>", self.stop)
             j += 2
             
        pos = self.size*playpos
        self.pbutt = self.canvas.create_polygon(pos,self.height,pos,self.height+30,pos+30,self.height+15, fill="black")
        self.canvas.tag_bind(self.pbutt,"<ButtonPress>", self.playstop)
        pos = self.size*recpos
        self.rbutt = self.canvas.create_oval(pos,self.height,pos+30,self.height+30, fill="black")
        self.canvas.tag_bind(self.rbutt,"<ButtonPress>", self.record)

    def quit(self):
        self.master.destroy()
        self.perf.Stop()
        self.perf.Join()
             
    def __init__(self,master=None):
        self.time = 0.0
        self.rectime = 0.0
        self.items = []
        self.notes = []
        self.notesrec = []
        self.recording = False
        self.playing = False
        Frame.__init__(self,master)
        self.pack()
        self.createCanvas()  
        self.createKeys()
        self.cs = csnd.Csound()
        res = self.cs.Compile("keys.csd")
        self.perf = csnd.CsoundPerformanceThread(self.cs)
        self.perf.Play()
        self.master = master
        self.master.bind("<space>", self.record)
        self.master.bind("<Return>", self.playstop)
        self.master.protocol("WM_DELETE_WINDOW", self.quit)
        master.title("Keysplay")

app = Application(Tk())
app.mainloop()

