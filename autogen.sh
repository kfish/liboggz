#!/bin/sh

# find out the automake version number, in format 1.major.minor
# (i don't think version 2 will be coming anytime soon :)  (stolen
# from videolan's vlc bootstrap strip)
automake_version="none"
if automake-1.7 --version >/dev/null 2>&1; then
  automake_version="-1.7"
elif automake-1.6 --version >/dev/null 2>&1; then
  automake_version="-1.6"
elif automake-1.5 --version >/dev/null 2>&1; then
  automake_version="-1.5"
elif automake --version > /dev/null 2>&1; then
  automake_version=""
  case "`automake --version | sed -e '1s/[^0-9]*//' -e q`" in
    0|0.*|1|1.[01234]|1.[01234][-.]*) automake_version="none" ;;
    1.5*)                             automake_version="-1.5" ;;
    1.6*)                             automake_version="-1.6" ;;
    1.7*)                             automake_version="-1.7" ;;
    1.8*)                             automake_version="-1.8" ;;
    1.9*)                             automake_version="-1.9" ;;
  esac
fi

if test "x${automake_version}" = "xnone"; then
  set +x
  echo "you need automake version 1.5 or later"
  exit 1
fi



automake_version_major=`echo "$automake_version" | cut -d. -f2`
automake_version_minor=`echo "$automake_version" | cut -d. -f3`

# need at least automake >= 1.5
if test "$automake_version_major" -lt "5"; then
  echo "$0: this project requires automake >= 1.5.  Please upgrade your version of automake to at least 1.5"
  exit 1
fi

# make autotools directory if it doesn't already exist
[ -d "autotools" ] || mkdir "autotools"

# need libltdl?
if grep -q AC_LIBLTDL_CONVENIENCE configure.*; then
  LIBTOOLIZE=${LIBTOOLIZE:-libtoolize}
  "$LIBTOOLIZE" --ltdl --copy --force
fi

AUTORECONF=${AUTORECONF:-autoreconf}
"$AUTORECONF" --install "$@"

