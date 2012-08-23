/**
	\file "bookmark.hh"
	$Id: $
 */

#ifndef	__BOC2_BOOKMARK_HH__
#define	__BOC2_BOOKMARK_HH__

#include "hand.hh"
#include "deck_state.hh"
#include "player_action.hh"

namespace blackjack {

/**
	Decision situation bookmark.
	Doesn't support insurance offers.
 */
struct bookmark {
	size_t				dealer_reveal;
//	hand				dealer_hand;
	hand				player_hand;
	// for now, just save everything, only ::cards is needed
	deck_state			cards;

	bookmark();

	bookmark(const size_t, const hand&, const deck_state&);

	~bookmark();

	ostream&
	dump(ostream&) const;

};	// end struct bookmark

}	// end namespace blackjack

#endif	// __BOC2_BOOKMARK_HH__
