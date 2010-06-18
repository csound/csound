
all: CSDPlayer.jar CsoundEditor.jar

CSDPlayer.jar: CSDPlayer.java CSDPlayer.mf
	-rm -r *.class csnd/*
	-rmdir csnd
	javac -classpath ../../csnd.jar $<
	jar xf ../../csnd.jar csnd
	jar cfm $@ CSDPlayer.mf csnd *.class
	jar i $@
	-rm -r *.class csnd/*
	-rmdir csnd

CsoundEditor.jar: CsoundEditor.java CsoundEditor.mf
	-rm -r *.class csnd/*
	-rmdir csnd
	javac -classpath ../../csnd.jar $<
	jar xf ../../csnd.jar csnd
	jar cfm $@ CsoundEditor.mf csnd *.class
	jar i $@
	-rm -r *.class csnd/*
	-rmdir csnd

clean:
	-rm *.jar *.class csnd/*
	-rmdir csnd

