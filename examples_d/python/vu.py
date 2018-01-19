from Tkinter import *
import csnd6

class Application(Frame):
  
    def move(self,event):
        canvas = event.widget
        x = canvas.canvasx(event.x)
        y = canvas.canvasy(event.y)
        item = canvas.find_withtag("current")[0]
        canvas.coords(item, x+10, y+10, x-10, y-10)
        self.cs.SetChannel("pitch", 55+660*(x/self.size))
        self.cs.SetChannel("level", self.lvl)
     
    def click(self,event):
        self.lvl = self.cs.GetChannel("level")
        self.cs.SetChannel("clicked", 1)
        self.drawing = False

    def unclick(self,event):
        self.cs.SetChannel("clicked", 0)       
        self.drawing = True
                
    def createCanvas(self):
        self.size = 600
        self.canvas = Canvas(self,height=self.size,width=self.size,bg="violet")
        self.canvas.pack()
    
         
    def createShapes(self):
          item = self.canvas.create_oval(self.size/2-10,self.size/2-10,self.size/2+10,self.size/2+10,fill="black")
          self.items.append(item)
          self.notes.append(440)
          self.notes.append(item)
          self.canvas.tag_bind(item,"<ButtonPress>", self.click)
          self.canvas.tag_bind(item,"<B1-Motion>", self.move)
          self.canvas.tag_bind(item,"<ButtonRelease>", self.unclick)
          
        
    def meter(self):
        iw = 10
        for i in range(0, self.size, iw):
            self.vu.append(self.canvas.create_rectangle(i,self.size-40,i+iw,self.size,fill="grey"))

    def draw(self):
        level = self.cs.GetChannel("meter")
        posy =  self.size - self.cs.GetChannel("pitch")*self.size
        posx =  level/10 
        if posy > self.size-40: posy = self.size-60
        if posx > self.size: posx = self.size - 20
        if posx < 0 : posx = 20
        if posy < 0 : posy = 20
        if self.drawing:
         self.canvas.coords(self.items[0], posx, posy, posx+30, posy+30)
        cnt = 0
        red = (self.size/10)*0.8
        yellow = (self.size/10)*0.6
        for i in self.vu:
          if level > cnt*100:
           if cnt > red:
            self.canvas.itemconfigure(i, fill="red")
            self.canvas.itemconfigure(self.items[0], fill="red")
           elif cnt > yellow:
            self.canvas.itemconfigure(i, fill="yellow")
            self.canvas.itemconfigure(self.items[0], fill="yellow")
           else:
            self.canvas.itemconfigure(i, fill="blue")
            self.canvas.itemconfigure(self.items[0], fill="blue")
          else:
           self.canvas.itemconfigure(i, fill="grey")
          cnt  = cnt + 1
        self.master.after(50,self.draw)
                
    def quit(self):
        self.master.destroy()
        self.perf.Stop()
        self.perf.Join()

    def __init__(self,master=None):
        master.title("make the ball jump!")
        self.items = []
        self.notes = []
        Frame.__init__(self,master)
        self.pack()
        self.createCanvas()  
        self.createShapes()
        self.cs = csnd6.Csound()
        res = self.cs.Compile("vu.csd")
        self.cs.SetChannel("pitch", 1.0)
        self.cs.SetChannel("volume", 1.0)
        self.drawing = True
        self.vu = []
        self.meter()
        master.after(100, self.draw),
        self.master = master
        self.perf = csnd6.CsoundPerformanceThread(self.cs)
        self.perf.Play()
        self.master.protocol("WM_DELETE_WINDOW", self.quit)

app = Application(Tk())
app.mainloop()
