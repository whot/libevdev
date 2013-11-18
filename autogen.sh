#!/bin/sh

test -n "$srcdir" || srcdir=`dirname "$0"`
test -n "$srcdir" || srcdir=.

olddir=`pwd`

cd "$srcdir"
test -d m4 || mkdir m4
autoreconf -fvi || exit $?

cd "$olddir"
test -n "$NOCONFIGURE" || $srcdir/configure "$@"
