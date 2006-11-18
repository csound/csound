from Tkinter import *
import csnd


class Application(Frame):
  
    def move(self,event):
        canvas = event.widget
        x = canvas.canvasx(event.x)
        y = canvas.canvasy(event.y)
        item = canvas.find_withtag("current")[0]
        canvas.coords(item, x+10, y+10, x-10, y-10)
        self.cs.SetChannel("pitch", 2.0*x/self.size)
        self.cs.SetChannel("volume", 2.0*(self.size-y)/self.size)
     
    def play(self,event):

        note = event.widget.find_withtag("current")[0]
        for i in range(1,len(self.notes),2):
            if note == self.notes[i]:
                val = self.notes[i-1]
                self.canvas.itemconfigure(i, fill="red")
                self.perf.InputMessage("i1 0 -1 %f %f" % (5000, val))

    def stop(self,event):
        self.on = False
        note = event.widget.find_withtag("current")[0]
        for i in range(1,len(self.notes),2):
            if note == self.notes[i]:
                val = self.notes[i-1]
                self.canvas.itemconfigure(i, fill="black")
                self.perf.InputMessage("i-1 0 1 %f" % val)
        
                
    def createCanvas(self):
        self.size = 600
        self.canvas = Canvas(self,height=self.size,width=self.size,bg="violet")
        self.canvas.pack()
    
         
    def createShapes(self):
          item = self.canvas.create_oval(self.size/2-10,self.size/2-10,self.size/2+10,self.size/2+10,fill="black")
          self.items.append(item)
          self.notes.append(440)
          self.notes.append(item)
          self.canvas.tag_bind(item,"<ButtonPress>", self.play)
          self.canvas.tag_bind(item,"<B1-Motion>", self.move)
          self.canvas.tag_bind(item,"<ButtonRelease>", self.stop)

        
    def meter(self):
        iw = 10
        for i in range(0, self.size, iw):
            self.vu.append(self.canvas.create_rectangle(i,self.size-40,i+iw,self.size,fill="grey"))

    def draw(self):
        level = self.cs.GetChannel("meter")
        cnt = 0
        red = (self.size/10)*0.8
        yellow = (self.size/10)*0.6
        for i in self.vu:
          if level > cnt*100:
           if cnt > red:
            self.canvas.itemconfigure(i, fill="red")
           elif cnt > yellow:
            self.canvas.itemconfigure(i, fill="yellow")
           else:
            self.canvas.itemconfigure(i, fill="blue")
          else:
           self.canvas.itemconfigure(i, fill="grey")
          cnt  = cnt + 1
        self.master.after(50,self.draw)
                
    def quit(self):
        self.master.destroy()
        self.perf.Stop()
        self.perf.Join()

    def __init__(self,master=None):
        master.title("Csound + Tkinter: just click and play")
        self.items = []
        self.notes = []
        Frame.__init__(self,master)
        self.pack()
        self.createCanvas()  
        self.createShapes()
        self.cs = csnd.Csound()
        res = self.cs.Compile("shapes.csd")
        self.cs.SetChannel("pitch", 1.0)
        self.cs.SetChannel("volume", 1.0)
        self.vu = []
        self.meter()
        master.after(100, self.draw),
        self.master = master
        self.perf = csnd.CsoundPerformanceThread(self.cs)
        self.perf.Play()
        self.master.protocol("WM_DELETE_WINDOW", self.quit)

app = Application(Tk())
app.mainloop()
