Csnd_Init =loadlib("c:/utah/home/mkg/projects/csound5/_csnd.dll","Csnd_Init") 
Csnd_Init()
csound = csnd.CppCsound()
print("csound = ", csound)
csound:Compile('-o', 'dac', '../examples/trapped.csd')
do
	repeat
		result = csound:PerformBuffer()
	until result ~= 0
end