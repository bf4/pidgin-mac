#!/bin/zsh


matchfn() {
	basename $1 | egrep $2 >/dev/null && return 0
	return -1
}

matchdn() {
	dirname $f | egrep $2 >/dev/null && return 0
	return -1
}

rm -f pidgin-osx-bundle.diff 2>/dev/null

for f in pidgin-src/**/*; do
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

	origf=${f/pidgin-src\//pidgin-src-orig/}

	diff -N -U 3 $origf $f >>pidgin-osx-bundle.diff || echo $f
done
