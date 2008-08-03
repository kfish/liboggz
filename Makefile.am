#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
#  PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE ORGANISATION OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
## Process this file with automake to produce Makefile.in

DISTCHECK_CONFIGURE_FLAGS = --enable-gcc-werror

SUBDIRS = doc include src

# pkg-config
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = oggz.pc

# Extra files to distribute in the source tarball.
EXTRA_DIST = oggz.pc.in oggz-uninstalled.pc.in \
	apache-install.sh \
	bash-completion/oggz \
	$(SYMBIAN_extra_dist) $(WIN32_extra_dist)

# Extra files to distribute for the Symbian build.
SYMBIAN_extra_dist = \
	README.symbian \
	symbian/bld.inf \
	symbian/config.h \
	symbian/liboggz.mmp

# Extra files to distribute for the Win32 build.
# Note that with Automake 1.9 it is possible to use wildcards instead of
# listing out files explicitly, ie.
#	win32/*.[ch] \
#	win32/*.def \
#	win32/*.dsp \
#	win32/*.dsw \
#	win32/VS200[35]/*.sln \
#	win32/VS200[35]/liboggz/*.vcproj \
# And with earlier versions, it is possible to use $(wildcard ...) if using
# GNU make.
# However, we list them out here for the sake of compatibility with earlier
# versions of Automake, and for portability with other vendors' make.
WIN32_extra_dist = \
	README.win32 \
	win32/Makefile \
	win32/attgetopt.c \
	win32/config.h \
	win32/getopt.h \
	win32/strsep.c \
	win32/liboggz.def \
	win32/liboggz.dsp \
	win32/oggzdump.dsp \
	win32/oggzed.dsp \
	win32/oggzinfo.dsp \
	win32/oggzmerge.dsp \
	win32/liboggz.dsw \
	win32/VS2003/liboggz.sln \
	win32/VS2005/liboggz.sln \
	win32/VS2003/liboggz/libogg_importer.vcproj \
	win32/VS2003/liboggz/liboggz.vcproj \
	win32/VS2003/liboggz/oggzdump.vcproj \
	win32/VS2003/liboggz/oggzinfo.vcproj \
	win32/VS2003/liboggz/oggzmerge.vcproj \
	win32/VS2003/liboggz/oggzrip.vcproj \
	win32/VS2003/liboggz/oggzscan.vcproj \
	win32/VS2003/liboggz/oggzvalidate.vcproj \
	win32/VS2005/liboggz/libogg_importer.vcproj \
	win32/VS2005/liboggz/liboggz.vcproj \
	win32/VS2005/liboggz/oggzdump.vcproj \
	win32/VS2005/liboggz/oggzinfo.vcproj \
	win32/VS2005/liboggz/oggzmerge.vcproj \
	win32/VS2005/liboggz/oggzrip.vcproj \
	win32/VS2005/liboggz/oggzscan.vcproj \
	win32/VS2005/liboggz/oggzvalidate.vcproj \
	win32/liboggz_bin_installer/liboggz_bin_installer.vdproj
