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
		dealer_reveal(cards::ACE), player_hand(), cards(variation()) {
}
bookmark::bookmark(const size_t dr, const hand& h, const deck_state& c) :
		dealer_reveal(dr), player_hand(h), cards(c) {
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
// TODO: move this to hand member function
	o << "options: double:" << yn(player_hand.player_options.can_double_down());
	o << ", split:" << yn(player_hand.player_options.can_split());
	o << ", surrender:" << yn(player_hand.player_options.can_surrender()) << endl;
}

//=============================================================================
}	// end namespace blackjack

