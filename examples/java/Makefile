
all: CSDPlayer.jar CsoundEditor.jar

CSDPlayer.jar: CSDPlayer.java CSDPlayer.mf
	-rm *.class csnd/*
	-rmdir csnd
	javac -classpath ../../bin/csnd.jar $<
	jar xf ../../bin/csnd.jar csnd
	jar cfm $@ CSDPlayer.mf csnd *.class
	jar i $@
	-rm *.class csnd/*
	-rmdir csnd

CsoundEditor.jar: CsoundEditor.java CsoundEditor.mf
	-rm *.class csnd/*
	-rmdir csnd
	javac -classpath ../../bin/csnd.jar $<
	jar xf ../../bin/csnd.jar csnd
	jar cfm $@ CsoundEditor.mf csnd *.class
	jar i $@
	-rm *.class csnd/*
	-rmdir csnd

clean:
	-rm *.jar *.class csnd/*
	-rmdir csnd

