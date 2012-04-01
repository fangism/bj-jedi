#!/bin/sh

args=`getopt "s:" $*`
if test $? != 0
then
	echo "Usage: $0 [ -s srcdir ] [files...]"
	exit 2
fi
set -- $args

# assumed source directory
srcdir=.
for i
do
	case "$i"
	in
		-s) srcdir="$2" shift; shift;;
		--) shift; break;;
	esac
done

for f
do
	b=`echo $f | cut -d. -f1`
	e=$b.bj-exp
	echo "cp $f $srcdir/$e"
	cp $f $srcdir/$e
done

