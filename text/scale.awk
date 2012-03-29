#!/usr/bin/awk -f
# "scale.awk"
# scale floating-point numbers
# options:
#	-v scalefactor
#	-v OFMT
#	-v startrow
#	-v startcol

BEGIN {
	if (!length(startrow)) {
		startrow = 2;
	}
	if (!length(startcol)) {
		startcol = 2;
	}
	if (!length(OFMT)) {
		OFMT = "%.2f";
	}
	OFS = "\t";
}

function scale(n) {
	if (length(scalefactor)) {
		return n *scalefactor;
	} else	return n;
}

{
if (NR >= startrow) {
	if (1 >= startcol) {
		printf(OFMT, scale($1));
	} else {
		printf(OFMT, $1);
	}
	for (i=2; i<=NF; ++i) {
	if (i >= startcol) {
		printf(OFS OFMT, scale($i));
	} else {
		printf(OFS OFMT, $i);
	}
	}
	print "";
} else {
	print $0;
}
}

