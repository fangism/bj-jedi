// "bj/core/outcome.cc"

#include <iostream>
#include <iterator>
#include <algorithm>

#include "bj/core/outcome.hh"
#include "bj/core/blackjack.hh"
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
dump_player_final_outcome_vector(const player_final_outcome_vector& v, ostream& o) {
	const player_final_outcome_vector::const_iterator b(v.begin()), e(v.end());
	player_final_outcome_vector::const_iterator i(b);
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

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\param header true if table header is wanted
 */
ostream&
dump_dealer_final_vector(ostream& o, const dealer_final_vector& d,
		const bool header) {
	if (header) {
		play_map::dealer_final_table_header(o) << endl;
	}
	std::ostream_iterator<probability_type> osi(o, "\t");
	std::copy(d.begin(), d.end(), osi);
	return o << endl;
}

//=============================================================================
}	// end namespace blackjack

