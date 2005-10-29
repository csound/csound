my_init=loadlib("c:/utah/home/mkg/projects/csound5/_csnd.dll","Csnd_Init") -- for Unix/Linux
assert(my_init) -- name sure its not nil
my_init()       -- call the init fn of the lib
cs = csnd.csoundCreate(nil)
print(cs)