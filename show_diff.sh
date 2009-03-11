#!/bin/zsh

if [ "$1" = "" ]; then
	echo "usage: $0 source-dir"
	exit -1
fi

srcdir="$1"
origsrcdir="$1-orig"

if [ ! -d "$origsrcdir" ]; then
	echo "$origsrcdir not found"
	exit -1
fi


matchfn() {
	basename $1 | egrep $2 >/dev/null && return 0
	return -1
}

matchdn() {
	dirname $f | egrep $2 >/dev/null && return 0
	return -1
}

rm -f $srcdir.diff 2>/dev/null

for f in $srcdir/**/*; do
	[ -d $f ] && continue
	matchfn $f "^configure.*$" && continue
	matchfn $f "^\.DS_Store$" && continue
	matchfn $f "^.*\.log$" && continue
	matchfn $f "^Makefile\..*$" && continue
	matchfn $f "^.*\.orig$" && continue
	matchfn $f "^.*\.rej$" && continue
	matchfn $f "^\.svn/.*$" && continue

#	matchdn $f "/libpurple/plugins" ] && continue
#	matchdn $f "/libpurple/protocols" ] && continue

	origf=${f/$srcdir\//$origsrcdir/}

	diff -N -U 3 $origf $f >>$srcdir.diff || echo $f
	[ ! -e "$origf" ] && echo "NEW: $f"
done
