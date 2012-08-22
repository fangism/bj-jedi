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
		dealer_reveal(cards::ACE), player_hand(), cards(variation())
#if !HAND_PLAYER_OPTIONS
		, player_options(action_mask::all)
#endif
		{
}
bookmark::bookmark(const size_t dr, const hand& h, const deck_state& c
#if !HAND_PLAYER_OPTIONS
		, const action_mask& m
#endif
		) :
		dealer_reveal(dr), player_hand(h), cards(c)
#if !HAND_PLAYER_OPTIONS
		, player_options(m)
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
// TODO: move this to hand member function
#if HAND_PLAYER_OPTIONS
#define	HAND	player_hand.
#else
#define	HAND
#endif
	o << "options: double:" << yn(HAND player_options.can_double_down());
	o << ", split:" << yn(HAND player_options.can_split());
	o << ", surrender:" << yn(HAND player_options.can_surrender()) << endl;
}

//=============================================================================
}	// end namespace blackjack

