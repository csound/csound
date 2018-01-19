import Tkinter
import csnd6

class Application(Tkinter.Frame):
  
    def move(self,event):
        canvas = event.widget
        inst = 0
        x = canvas.canvasx(event.x)
        y = canvas.canvasy(event.y)
        circle = canvas.find_withtag("current")[0]
        canvas.coords(circle, x+10, y+10, x-10, y-10)
        for item in self.notes:
            if circle == item[1]:
              inst = item[0]
              break
        chn = "pitch-%d" % inst
        self.cs.SetChannel(chn, 2.0*x/self.size)
        chn = "volume-%d" % inst
        self.cs.SetChannel("volume", 2.0*(self.size-y)/self.size)
     
    def play(self,event):
        note = event.widget.find_withtag("current")[0]
        for item in self.notes:
            if note == item[1]:
                canvas = event.widget
                x = canvas.canvasx(event.x)
                y = canvas.canvasy(event.y)
                val = item[0]
                chn = "pitch-%d" % val
                self.cs.SetChannel(chn, 2.0*x/self.size)
                chn = "volume-%d" % val
                self.cs.SetChannel(chn,
                                   2.0*(self.size-y)/self.size)
                self.canvas.itemconfigure(note, fill="red")
                self.perf.InputMessage("i1.%d 0 -1 5000 440" %
                                       int(val))
                break

    def keyPlay(self,event):
        note = int(event.char)
        for item in self.notes:
         if(note == item[0]):
            circle = item[1]   
            x,y,x1,y1 = self.canvas.coords(circle)
            chn = "pitch-%d" % note
            self.cs.SetChannel(chn, 2.0*x/self.size)
            chn = "volume-%d" % note
            self.cs.SetChannel(chn, 2.0*(self.size-y)/self.size)
            self.canvas.itemconfigure(circle, fill="red")
            self.perf.InputMessage("i1.%d 0 -1 5000 440" %
                                       int(note))
            break

    def keyStop(self,event):
        note = int(event.char)
        for item in self.notes:
         if(note == item[0]):
            circle = item[1]   
            self.canvas.itemconfigure(circle, fill="black")
            self.perf.InputMessage("i-1.%d 0 -1 5000 440" %
                                       int(note))
            break

        
    def stop(self,event):
        note = event.widget.find_withtag("current")[0]
        for item in self.notes:
            if note == item[1]:
                val = item[0]
                self.canvas.itemconfigure(note, fill="black")
                self.perf.InputMessage("i-1.%d 0 1 440" % int(val))
                break
        
    def newNote(self,event):
        self.createCircle(self.size/2,self.size/2)
                          
    def createCanvas(self):
        self.size = 600
        self.canvas = Tkinter.Canvas(self,height=self.size,
                                     width=self.size,
                                     bg="violet")
        self.canvas.pack()
         
    def createCircle(self,x=0,y=0):
          circle = self.canvas.create_oval(x-10,
                                         y-10,
                                        x+10,
                                         y+10,
                                         fill="black")
          self.insts += 1
          self.notes.append((self.insts, circle))
          self.canvas.tag_bind(circle,"<ButtonPress>",
                               self.play)
          self.canvas.tag_bind(circle,"<B1-Motion>",
                               self.move)
          self.canvas.tag_bind(circle,"<ButtonRelease>",
                               self.stop)
          self.master.bind("%d" % self.insts,
                               self.keyPlay)
          self.master.bind("<KeyRelease-%d>" % self.insts,
                               self.keyStop)

    def createButton(self):
        self.newbutton = self.canvas.create_rectangle(1,1,
            26,26,fill="black")
        self.canvas.tag_bind(self.newbutton,"<ButtonPress>",
                               self.newNote)
        
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
        res = self.cs.Compile('shapes2.csd')
        if res == 0:
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
        self.notes = []
        self.insts = 0
        self.createButton()
        self.createMeter()
        if self.createEngine() is True:
          self.drawMeter()
          self.master.protocol('WM_DELETE_WINDOW',
                               self.quit)
          self.master.mainloop()
        else: self.master.quit()

Application(Tkinter.Tk())

