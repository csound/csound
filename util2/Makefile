#								UTIL2 makef
# macros defined in toplevel csound/makefile
#	CFLAGS INCLUDE LIB DEST

M = "DEFINES=$(DEFINES)"

MAKE = make $M

all: utils2

utils2:	sndinfos scale mixers envext extractor pvlook dnoise srconv


sndinfos:
	cd sndinfo; $(MAKE) $@

scale:
	cd scale.dir; $(MAKE) $@

mixers:
	cd mixer; $(MAKE) $@

envext:
	cd envext; $(MAKE) $@

extractor:
	cd mixer; $(MAKE) $@

pvlook:
	cd pvlook.dir; $(MAKE) $@

dnoise:
	cd dnoise.dir; $(MAKE) $@

srconv:
	cd dnoise.dir; $(MAKE) $@

install:
	cd sndinfo; $(MAKE) install
	cd scale.dir; $(MAKE) install
	cd mixer; $(MAKE) install
	cd envext; $(MAKE) install
	cd pvlook.dir; $(MAKE) install
	cd dnoise.dir; $(MAKE) install

clean:
	cd sndinfo; $(MAKE) clean
	cd scale.dir; $(MAKE) clean
	cd mixer; $(MAKE) clean
	cd envext; $(MAKE) clean
	cd pvlook.dir; $(MAKE) clean
	cd dnoise.dir; $(MAKE) clean

rmbak:
	-rm *.bak *~
	cd sndinfo; $(MAKE) rmbak
	cd scale.dir; $(MAKE) rmbak
	cd mixer; $(MAKE) rmbak
	cd envext; $(MAKE)  rmbak
	cd pvlook.dir; $(MAKE)  rmbak
	cd dnoise.dir; $(MAKE)  rmbak
