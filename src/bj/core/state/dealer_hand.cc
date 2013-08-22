// "bj/core/state/dealer_hand.cc"

#include <iostream>
#include "bj/core/state/dealer_hand.hh"
#include "bj/core/blackjack.hh"

#define	DEBUG_HAND				0

namespace blackjack {
using cards::card_name;
using cards::card_symbols;
using cards::card_value_map;
using cards::state_machine;

//=============================================================================
// class dealer_hand_base method definitions

void
dealer_hand_base::set_upcard(const card_type c) {
	assert(c < card_symbols);
	const card_type d = card_value_map[c];
	state = play_map::d_initial_card_map[d];
	first_card = true;
	// peek_state untouched
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
dealer_hand_base::dump(ostream& o, const state_machine& sm) const {
	o << sm[state].name << ' ';
	switch (peek_state) {
	case PEEKED_NO_10: o << "peek!10"; break;
	case PEEKED_NO_ACE: o << "peek!A "; break;
	case NO_PEEK:
	default: o << "no-peek"; break;
	}
	return o;
}

//=============================================================================
/**
	\param p1 reveal card can be A,2-9,T-K, will be translated to value
 */
void
dealer_hand::initial_card_dealer(const card_type p1) {
	// the string stores the card names, not card indices
	assert(p1 < card_symbols);
	const card_type reveal_value = card_value_map[p1];
	reveal = p1;
//	reveal = card_value_map[p1];
	cards.clear();
	cards.push_back(card_name[p1]);
	state = play->initial_card_dealer(reveal_value);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
dealer_hand::hit_dealer(const card_type p2) {
	assert(p2 < card_symbols);
	reveal_hole_card();
	cards.push_back(card_name[p2]);
	state = play->hit_dealer(state, card_value_map[p2]);
	if ((cards.size() == 2) && (state == goal)) {
		state = dealer_blackjack;
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
dealer_hand::dump_dealer(ostream& o) const {
	o << "dealer: " << cards << " (" << play->dealer_hit[state].name << ")";
	return o;
}

//=============================================================================
}	// end namespace blackjack

