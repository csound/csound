print (_VERSION)
Csnd_Init = loadlib("_csnd.dll","Csnd_Init")
print(Csnd_Init)
Csnd_Init()
csound = csnd.Csound()
print("csound = ", csound)
csound:Compile('-o', 'dac', 'examples/trapped.csd')
do
	repeat
		result = csound:PerformBuffer()
	until result ~= 0
end