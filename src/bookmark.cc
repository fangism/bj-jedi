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
#if BITMASK_ACTION_OPTIONS
		player_options(action_mask::all)
#else
		may_double(true), may_split(true), may_surrender(true)
#endif
		{
}
bookmark::bookmark(const size_t dr, const hand& h, const deck_state& c, 
#if BITMASK_ACTION_OPTIONS
		const action_mask& m
#else
		const bool d, const bool p, const bool r
#endif
		) :
		dealer_reveal(dr), player_hand(h), cards(c), 
#if BITMASK_ACTION_OPTIONS
		player_options(m)
#else
		may_double(d), may_split(p), may_surrender(r)
#endif
		{
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
#if BITMASK_ACTION_OPTIONS
	o << "options: double:" << yn(player_options.can_double_down());
	o << ", split:" << yn(player_options.can_split());
	o << ", surrender:" << yn(player_options.can_surrender()) << endl;
#else
	o << "options: double:" << yn(may_double);
	o << ", split:" << yn(may_split);
	o << ", surrender:" << yn(may_surrender) << endl;
#endif
}

//=============================================================================
}	// end namespace blackjack

