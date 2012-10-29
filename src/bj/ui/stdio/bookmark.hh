/**
	\file "bj/ui/stdio/bookmark.hh"
	$Id: $
 */

#ifndef	__BJ_UI_STDIO_BOOKMARK_HH__
#define	__BJ_UI_STDIO_BOOKMARK_HH__

#include "bj/core/hand.hh"
#include "bj/core/deck_state.hh"
#include "bj/core/player_action.hh"

namespace cards {
class counter_base;
}

namespace blackjack {
using cards::card_type;
class play_map;

/**
	Decision situation bookmark.
	Doesn't support insurance offers.
 */
struct bookmark {
	card_type			dealer_reveal;
//	hand				d_hand;
//	player_hand			p_hand;
	player_hand_base		p_hand;
	// for now, just save everything, only ::cards is needed
	perceived_deck_state		cards;
//	deck_state			cards;

	bookmark();

	bookmark(const card_type, const player_hand&, const perceived_deck_state&);

	~bookmark();

	ostream&
	dump(ostream&, const play_map&) const;

	ostream&
	dump(ostream&, const play_map&, 
		const char*, const cards::counter_base&) const;

};	// end struct bookmark

}	// end namespace blackjack

#endif	// __BJ_UI_STDIO_BOOKMARK_HH__
