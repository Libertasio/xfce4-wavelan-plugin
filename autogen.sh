#!/bin/sh
#
# $Id$
#
# Copyright (c) 2003-2005
#         The Xfce development team. All rights reserved.
#
# Written for Xfce by Benedikt Meurer <benny@xfce.org>.
#

(type xdt-autogen) >/dev/null 2>&1 || {
  cat >&2 <<EOF
autogen.sh: You don't seem to have the Xfce development tools installed on
            your system, which are required to build this software.
            Please install the xfce4-dev-tools package first, it is available
            from http://www.xfce.org/.
EOF
  exit 1
}

# substitute revision and date
revision=`LC_ALL=C svn info $0 | awk '/^Revision: / {printf "%05d\n", $2}'`
sed -e "s/@DATE@/`date +%Y%m%d`/g" -e "s/@REVISION@/${revision}/g" \
  < "configure.in.in" > "configure.in"

exec xdt-autogen $@

# vi:set ts=2 sw=2 et ai:
