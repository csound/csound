
all: CSDPlayer.jar CsoundEditor.jar

CSDPlayer.jar: CSDPlayer.java CSDPlayer.mf
	-rm -r *.class csnd6/*
	-rmdir csnd6
	javac -classpath ../../csnd6.jar $<
	jar xf ../../csnd6.jar csnd6
	jar cfm $@ CSDPlayer.mf csnd6 *.class
	jar i $@
	-rm -r *.class csnd6/*
	-rmdir csnd6

CsoundEditor.jar: CsoundEditor.java CsoundEditor.mf
	-rm -r *.class csnd6/*
	-rmdir csnd6
	javac -classpath ../../csnd6.jar $<
	jar xf ../../csnd6.jar csnd6
	jar cfm $@ CsoundEditor.mf csnd6 *.class
	jar i $@
	-rm -r *.class csnd6/*
	-rmdir csnd6

clean:
	-rm *.jar *.class csnd6/*
	-rmdir csnd6

