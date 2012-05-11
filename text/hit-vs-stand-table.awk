#!/usr/bin/awk -f

/^[0-9]+/ || /^A/ || /^[2-9A],[2-9A]/ || /^BJ/ || /^bust/ {
	p_hand = $1;
}

/^hit/ || /^stand/ || /^split/ || /^double/ {
	if (p_hand != "BJ" && p_hand != "bust") {
	print p_hand "-" $0;
	if (p_hand == "21") {
		line = $0;
		gsub("0\\.[0-9]*", "-1.000", line);
		gsub("stand", "hit", line);
		print p_hand "-" line;
	}
	}
}
