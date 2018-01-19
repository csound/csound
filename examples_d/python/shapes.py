import Tkinter
import csnd6

class Application(Tkinter.Frame):
  
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
        self.canvas.itemconfigure(note, fill="red")
        self.perf.InputMessage("i1 0 -1 5000 440")

    def stop(self,event):
        note = event.widget.find_withtag("current")[0]
        self.canvas.itemconfigure(note, fill="black")
        self.perf.InputMessage("i-1 0 5000 440")
        
                
    def createCanvas(self):
        self.size = 600
        self.canvas = Tkinter.Canvas(self,height=self.size,
                                     width=self.size,
                                     bg="violet")
        self.canvas.pack()
         
    def createCircle(self):
          circle = self.canvas.create_oval(self.size/2-10,
                                         self.size/2-10,
                                         self.size/2+10,
                                         self.size/2+10,
                                         fill="black")

          self.canvas.tag_bind(circle,"<ButtonPress>",
                               self.play)
          self.canvas.tag_bind(circle,"<B1-Motion>",
                               self.move)
          self.canvas.tag_bind(circle,"<ButtonRelease>",
                               self.stop)
        
    def createMeter(self):
     iw = 10
     self.vu = []
     for i in range(0, self.size, iw):
      self.vu.append(self.canvas.create_rectangle(i,
            self.size-40,i+iw,self.size,fill="grey"))

    def drawMeter(self):
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
        self.master.after(50,self.drawMeter)
                
    def quit(self):
        self.perf.Stop()
        self.perf.Join()
        self.master.destroy()

    def createEngine(self):
        self.cs = csnd6.Csound()
        res = self.cs.Compile('shapes.csd')
        if res == 0:
         self.cs.SetChannel('pitch', 1.0)
         self.cs.SetChannel('volume', 1.0)
         self.perf = csnd6.CsoundPerformanceThread(self.cs)
         self.perf.Play()
         return True
        else:
         return False   
   
    def __init__(self,master=None):
        Tkinter.Frame.__init__(self,master)
        self.master.title('Csound + Tkinter: '
         'just click and play')
        self.master = master
        self.pack()
        self.createCanvas()  
        self.createCircle()
        self.createMeter()
        if self.createEngine() is True:
          self.drawMeter()
          self.master.protocol('WM_DELETE_WINDOW',
                               self.quit)
          self.master.mainloop()
        else: self.master.quit()

Application(Tkinter.Tk())

