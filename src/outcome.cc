// "outcome.cc"

#include <iostream>
#include "outcome.hh"
#include "util/array.tcc"

namespace blackjack {
using std::endl;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
operator << (ostream& o, const outcome r) {
	// matches the enum outcome ordering
	static const char* wlp[] = { "win", "push", "lose" };
#if 0
	switch (r) {
	case WIN: o << "win"; break;
	case LOSE: o << "lose"; break;
	case PUSH: o << "push"; break;
	default: o << '?'; break;
	}
#else
	o << wlp[size_t(r)];
#endif
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
ostream&
operator << (ostream& o, const outcome_odds& r) {
	o << r.win() << '|' << r.push() << '|' << r.lose();
#if 0
	// confirm sum
	o << '=' << r.win() +r.push() +r.lose();
#endif
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
dump_outcome_vector(const outcome_vector& v, ostream& o) {
	const outcome_vector::const_iterator b(v.begin()), e(v.end());
	outcome_vector::const_iterator i(b);
	o << "win";
	for (; i!=e; ++i) {
		o << '\t' << i->win();
	}
	o << endl;
	o << "push";
	for (i=b; i!=e; ++i) {
		o << '\t' << i->push();
	}
	o << endl;
	o << "lose";
	for (i=b; i!=e; ++i) {
		o << '\t' << i->lose();
	}
	return o << endl;
}

//=============================================================================
}	// end namespace blackjack

