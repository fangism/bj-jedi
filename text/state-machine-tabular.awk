#!/usr/bin/awk -f
# translates a state-machine dump to a table for LaTeX

# load
{
	id = $1;
	_name = $2;
	gsub("\\\"", "", _name);
	gsub(":", "", _name);
	name[id] = _name;
	edges[id] = $3;
}

END {
	for (i=0; i<=id; ++i) {
		printf("%s", name[i]);
		ntoks=split(edges[i], toks, ",");
		if (ntoks) {
		for (j=1; j<ntoks; ++j) {
			printf("&%s", name[toks[j]]);
		}
		} else {
			printf("& & & & & & & & & &");
		}
		print " \\\\ \\hline";
	}
}

