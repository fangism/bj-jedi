#!/bin/sh

args=`getopt "s:" $*`
if test $? != 0
then
	echo "Usage: $0 [ -s srcdir ] [files...]"
	exit 2
fi
set -- $args

# assumed source directory
srcdir=`dirname $0`
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
	case $f in
	*.bj* ) e=$b.bj-exp ;;
	*.* ) e=$b.expect ;;
	esac
	echo "cp $f $srcdir/$e"
	cp $f $srcdir/$e
done

