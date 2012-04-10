/**
	\file "bookmark.cc"
	$Id: $
 */

#include <iostream>
#include "bookmark.hh"
#include "variation.hh"
#include "util/configure_option.hh"

namespace blackjack {
using cards::card_name;
using std::endl;
using util::yn;

//=============================================================================
// struct bookmark method definitions

bookmark::bookmark() :
		dealer_reveal(cards::ACE), player_hand(), cards(variation()), 
		may_double(true), may_split(true), may_surrender(true) {
}
bookmark::bookmark(const size_t dr, const hand& h, const deck_state& c, 
		const bool d, const bool p, const bool r) :
		dealer_reveal(dr), player_hand(h), cards(c), 
		may_double(d), may_split(p), may_surrender(r) {
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bookmark::~bookmark() { }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Similar to grader::dump_situation.
 */
ostream&
bookmark::dump(ostream& o) const {
	cards.show_count(o, false, false);
	// don't distinguish face cards, only show remaining, not used cards
	o << "dealer: " << card_name[dealer_reveal] << ", ";
	player_hand.dump_player(o) << endl;
	o << "options: double:" << yn(may_double);
	o << ", split:" << yn(may_split);
	o << ", surrender:" << yn(may_surrender) << endl;
}

//=============================================================================
}	// end namespace blackjack

