docdir=$(prefix)/share/doc/@PACKAGE@

EXTRA_DIST = Doxyfile.in liboggz

all: doxygen-build.stamp

if HAVE_DOXYGEN
doxygen-build.stamp: Doxyfile $(top_srcdir)/include/oggz/*.h
	doxygen
	touch doxygen-build.stamp
else
doxygen-build.stamp:
	echo "*** Warning: Doxygen not found; documentation will not be built."
	touch doxygen-build.stamp
endif

install-data-local: doxygen-build.stamp
	$(mkinstalldirs) $(docdir)
	if test -d liboggz; then \
	  for dir in liboggz/*; do \
	    if test -d $$dir; then \
	      b=`basename $$dir`; \
	      $(mkinstalldirs) $(docdir)/$$b; \
	      (cd $$dir && $(INSTALL_DATA) * $(docdir)/$$b) \
	    fi \
	  done \
	fi

uninstall-local:
	rm -rf $(docdir)

maintainer-clean-local:
	if test -d liboggz; then rm -rf liboggz; fi
