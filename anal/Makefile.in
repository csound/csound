#								ANAL makef
# macros defined in toplevel csound/makefile
#	CFLAGS INCLUDE LIB DEST

MAKE =	make

ADSYNS = hetro
LPCS = lpanal
PVOCS = pvanal
CVL = cvanal

anals:	adsyns lpcs pvocs cvl

adsyns $(ADSYNS):
	cd adsyn; $(MAKE) $@

lpcs $(LPCS):
	cd lpc; $(MAKE) $@

pvocs $(PVOCS):
	cd pvoc; $(MAKE) $@

cvl $(CVL):
	cd convol; $(MAKE) $@

install:
	cd adsyn; $(MAKE) install
	cd lpc; $(MAKE) install
	cd pvoc; $(MAKE) install
	cd convol; $(MAKE) install

clean:
	cd adsyn; $(MAKE) clean
	cd lpc; $(MAKE) clean
	cd pvoc; $(MAKE) clean
	cd convol; $(MAKE) clean

rmbak:
	-rm *.bak *~
	cd adsyn; $(MAKE) rmbak
	cd lpc; $(MAKE) rmbak
	cd pvoc; $(MAKE) rmbak
	cd convol; $(MAKE) rmbak
