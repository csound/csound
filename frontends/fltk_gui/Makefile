
PROGRAM = csound5gui
CXX = g++
CXXFLAGS = -Wall -W -ansi -pedantic -g -O -I. -I../../H -I../../interfaces
LIBS = -lfltk -L../.. -L/usr/X11R6/lib -lX11 -lcsound -lstdc++

# -----------------------------------------------------------------------------

FLUID_FL =	CsoundGUIConsole_FLTK.fl		\
		CsoundGUIMain_FLTK.fl			\
		CsoundGlobalSettingsPanel_FLTK.fl	\
		CsoundPerformanceSettingsPanel_FLTK.fl	\
		CsoundUtilitiesWindow_FLTK.fl

FLUID_SRCS = $(subst .fl,.cpp,$(FLUID_FL))
FLUID_HDRS = $(subst .fl,.hpp,$(FLUID_FL))

SRCS =	ConfigFile.cpp			\
	CsoundGUIConsole.cpp		\
	CsoundGUIMain.cpp		\
	CsoundGlobalSettings.cpp	\
	CsoundPerformance.cpp		\
	CsoundPerformanceSettings.cpp	\
	CsoundUtility.cpp		\
	../../interfaces/csPerfThread.cpp	\
	main.cpp			\
	$(FLUID_SRCS)

OBJS = $(subst .cpp,.o,$(SRCS))

all: $(FLUID_SRCS) $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.cpp: %.fl
	fluid -c $<

clean:
	-rm *.o $(OBJS) $(PROGRAM) $(FLUID_SRCS) $(FLUID_HDRS)

