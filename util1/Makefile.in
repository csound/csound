#								UTIL1 Makefile
# macros defined in toplevel csound/Makefile
#	CFLAGS INCLUDE LIB DEST

M = "DEFINES=$(DEFINES)"

MAKE = make $M

SORTS = scsort extract

utils1:	scots cscores sortexs

scots:
	cd scot; $(MAKE) $@

cscores:
	cd cscore; $(MAKE) $@

sortexs $(SORTS):
	cd sortex; $(MAKE) $@

install:
	cd scot; $(MAKE) install
	cd cscore; $(MAKE) install
	cd sortex; $(MAKE) install

clean:
	cd scot; $(MAKE) clean
	cd cscore; $(MAKE) clean
	cd sortex; $(MAKE) clean

rmbak:
	-rm *.bak *~
	cd scot; $(MAKE) rmbak
	cd cscore; $(MAKE) rmbak
	cd sortex; $(MAKE) rmbak
