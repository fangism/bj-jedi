/**
	\file "bj-splitgen.cc"
	Generates dot graphs of split calculations.
 */

#include <iostream>
#include "hand.hh"
#include "util/string.tcc"

using std::cout;
using std::ostream;
using std::endl;
using util::strings::string_to_num;
using blackjack::split_state;

static
void
usage(ostream& o) {
	o << "usage: bj-splitgen N\n"
	"where N (>=1) is the maximum number of hands allowed by splitting.\n"
	"Produces digraph of split states used in computing expected value."
	<< endl;
}

int
main(int argc, char* argv[]) {
	int N;
	if (argc == 2) {
		if (string_to_num(argv[1], N) || N<1) {
			usage(cout);
			return 1;
		}
	} else {
		usage(cout);
		return 2;
	}
	split_state ss;
	ss.splits_remaining = N-1;
	ss.paired_hands = 1;
	ss.unpaired_hands = 0;
	cout << "digraph G {" << endl;
	cout << "# allow split up to " << N << " hands." << endl;
	ss.generate_graph(cout);
	cout << "}" << endl;
	return 0;
}
