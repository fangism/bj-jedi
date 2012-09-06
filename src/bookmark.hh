/**
	\file "bookmark.hh"
	$Id: $
 */

#ifndef	__BOC2_BOOKMARK_HH__
#define	__BOC2_BOOKMARK_HH__

#include "hand.hh"
#include "deck_state.hh"
#include "player_action.hh"

namespace cards {
class counter_base;
}

namespace blackjack {

/**
	Decision situation bookmark.
	Doesn't support insurance offers.
 */
struct bookmark {
	size_t				dealer_reveal;
//	hand				d_hand;
	player_hand			p_hand;
//	player_hand_base		p_hand;
	// for now, just save everything, only ::cards is needed
	perceived_deck_state		cards;
//	deck_state			cards;

	bookmark();

	bookmark(const size_t, const player_hand&, const perceived_deck_state&);

	~bookmark();

	ostream&
	dump(ostream&) const;

	ostream&
	dump(ostream&, const char*, const cards::counter_base&) const;

};	// end struct bookmark

}	// end namespace blackjack

#endif	// __BOC2_BOOKMARK_HH__
