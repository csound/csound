from Tkinter import *

class TestApplication(Frame):
    def selectTest(self):
        selected = self.listBox.curselection()[0]
        self.textBox.insert(0, selected[5])

    def createWidgets(self):

        self.scrollbar = Scrollbar(self)
        self.scrollbar.grid(row=0, column=1, sticky="ns")

        self.listBox = Listbox(self, yscrollcommand=self.scrollbar.set)
        self.listBox.grid(row=0, column=0, sticky="ew")      

        self.scrollbar2 = Scrollbar(self)
        self.scrollbar2.grid(row=1, column=1, sticky="ns")
        
        self.textBox = Text(self, yscrollcommand=self.scrollbar2.set)
        self.textBox.grid(row=1, column=0, sticky="nsew")

        self.scrollbar.config(command=self.listBox.yview)
        self.scrollbar2.config(command=self.textBox.yview)
        
        self.current = None
        self.poll() # start polling the list
    
    def poll(self):
        now = self.listBox.curselection()
        if now != self.current:
            self.list_has_changed(now)
            self.current = now
        self.after(250, self.poll)

    def list_has_changed(self, selection):
        if len(selection) > 0:
            self.textBox.delete(1.0, END)
            self.textBox.insert(END, self.results[int(selection[0])][3])

    def setResults(self, results):
        self.results = results

        label = "%s (%s)"
        
        for i in results:            
            self.listBox.insert(END, label%(i[1], i[0]))

    def __init__(self, master=None):
        Frame.__init__(self, master)       
        self.pack()
        self.createWidgets()

if __name__ == "__main__":
    root = Tk()
    app = TestApplication(master=root)
    app.mainloop()
    root.destroy()
