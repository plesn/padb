bin_SCRIPTS = padb
libexec_PROGRAMS = minfo
minfo_CFLAGS = -Wall
minfo_LDADD = -ldl
minfo_SOURCES = minfo.c mpi_interface.h
if HAVE_HWLOC
libexec_PROGRAMS += topoinfo
topoinfo_SOURCES = topoinfo.c
topoinfo_LDADD = -lhwloc
topoinfo_CFLAGS = -Wall
endif
EXTRA_DIST = padb

tidy:
	perltidy -b -ce -w -se padb

pc:	padb
	perlcritic --brutal --verbose "%l: (%s) %m\n" padb > .pc.tmp || true
	/bin/mv .pc.tmp pc

report: pc
	./report.pl pc | tee report
