// "outcome.cc"

#include <iostream>
#include "outcome.hh"
#include "util/array.tcc"
#if 0
#include <cassert>
#include <algorithm>
#include <functional>
#include <numeric>		// for accumulate
#include <limits>		// for numeric_limits
#include <iomanip>
#include <iterator>
#include <cmath>		// for fabs
#include "variation.hh"
#include "blackjack.hh"
#include "util/probability.tcc"
// for commands
#include "util/string.tcc"
#include "util/command.tcc"
#include "util/value_saver.hh"
#include "util/iosfmt_saver.hh"
#include "util/member_select_iterator.hh"
#endif

namespace blackjack {
#if 0
using std::ios_base;
using std::vector;
using std::fill;
using std::copy;
using std::for_each;
using std::transform;
using std::inner_product;
using std::accumulate;
using std::mem_fun_ref;
using std::ostream_iterator;
using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::multimap;
using std::make_pair;
using std::setw;
using std::bind2nd;
using util::normalize;
using util::precision_saver;
using cards::ACE;
using cards::TEN;
using cards::card_name;
using cards::standard_deck_distribution;
using cards::reveal_print_ordering;
using cards::card_index;
#endif

using util::value_saver;
using util::Command;
using util::CommandStatus;
using util::string_list;
using util::strings::string_to_num;
using util::member_select_iterator;
using util::var_member_select_iterator;

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

