docdir=$(prefix)/share/doc/@PACKAGE@

EXTRA_DIST = Doxyfile.in \
	forcefeed.fig forcefeed.eps forcefeed.png \
	hungry.fig hungry.eps hungry.png \
	oggzdump.1.sgml oggzdiff.1.sgml \
	oggzdump.1 oggzdiff.1

man_MANS = oggzdump.1 oggzdiff.1

doc_DATA = doxygen-build.stamp

html: oggzdump.1.html

if HAVE_DOCBOOKTOMAN
%.1: %.1.sgml
	docbook-to-man $< > $@
else
%.1: %.1.sgml
endif

if HAVE_DOCBOOK2HTML
%.1.html: %.1.sgml
	-docbook2html $<
	mv index.html $@
endif

if HAVE_DOXYGEN
doxygen-build.stamp: Doxyfile $(top_srcdir)/include/oggz/*.h $(top_srcdir)/src/examples/*.c
	doxygen
	touch doxygen-build.stamp
else
doxygen-build.stamp:
	echo "*** Warning: Doxygen not found; documentation will not be built."
	touch doxygen-build.stamp
endif

dist_docdir = $(distdir)/liboggz

dist-hook:
	if test -d liboggz; then \
	  mkdir $(dist_docdir); \
	  for dir in liboggz/*; do \
            if test -d $$dir; then \
              b=`basename $$dir`; \
              mkdir $(dist_docdir)/$$b; \
	      for f in $$dir/*; do \
                cp -p $$f $(dist_docdir)/$$b; \
              done \
            fi \
          done \
	fi


install-data-local: doxygen-build.stamp
	$(mkinstalldirs) $(DESTDIR)$(docdir)
	if test -d liboggz; then \
	  for dir in liboggz/*; do \
	    if test -d $$dir; then \
	      b=`basename $$dir`; \
	      $(mkinstalldirs) $(DESTDIR)$(docdir)/$$b; \
	      for f in $$dir/*; do \
		$(INSTALL_DATA) $$f $(DESTDIR)$(docdir)/$$b; \
              done \
	    fi \
	  done \
	fi

uninstall-local:
	rm -rf $(DESTDIR)$(docdir)

clean-local:
	if test -d liboggz; then rm -rf liboggz; fi
	if test -f doxygen-build.stamp; then rm -f doxygen-build.stamp; fi
