#!/usr/bin/awk -f
# "transpose.awk"
# transpose a matrix

BEGIN {
	max_NF = 1;
	OFS = "\t";
}

{
	for (i=1; i<=NF; ++i) {
		matrix[i,NR] = $i;
	}
	max_NR = NR;
	if (NF > max_NF) {
		max_NF = NF;
	}
}

END {
	for (i=1; i<=max_NF; ++i) {
		printf("%s", matrix[i,1]);
		for (j=2; j<=max_NR; ++j) {
			printf("%s", OFS matrix[i,j]);
		}
		print "";
	}
}
