// "hand.cc"

#include <iostream>
#include "hand.hh"
#include "blackjack.hh"

#define	DEBUG_HAND				0

namespace blackjack {
using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using cards::ACE;
using cards::TEN;
using cards::card_name;
using cards::standard_deck_distribution;
using cards::card_index;
using cards::card_symbols;
using cards::card_value_map;

//=============================================================================
// class player_hand method definitions

void
player_hand::initial_card_player(const size_t p1) {
	// the string stores the card names, not card indices
	assert(p1 < card_symbols);
	cards.clear();
	cards.push_back(card_name[p1]);
	state = play->initial_card_player(card_value_map[p1]);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
dealer_hand::initial_card_dealer(const size_t p1) {
	// the string stores the card names, not card indices
	assert(p1 < card_symbols);
	cards.clear();
	cards.push_back(card_name[p1]);
	state = play->initial_card_dealer(card_value_map[p1]);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
player_hand::hit_player(const size_t p2) {
	assert(p2 < card_symbols);
	cards.push_back(card_name[p2]);
	state = play->hit_player(state, card_value_map[p2]);
	player_options &= play->post_hit_actions;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
dealer_hand::hit_dealer(const size_t p2) {
	assert(p2 < card_symbols);
	cards.push_back(card_name[p2]);
	state = play->hit_dealer(state, card_value_map[p2]);
	if ((cards.size() == 2) && (state == goal)) {
		state = dealer_blackjack;
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Assigns initial state based on player's first two cards.
	\param p1,p2 first two player's cards
	\param nat true if 21 should be considered natural blackjack
	\param dr dealer's reveal card
 */
void
player_hand::deal_player(const size_t p1, const size_t p2, const bool nat, 
		const size_t dr) {
	cards.clear();
	cards.push_back(card_name[p1]);
	cards.push_back(card_name[p2]);
	state = play->deal_player(card_value_map[p1], card_value_map[p2], nat);
#if DEBUG_HAND
	play->initial_actions_per_state[state].dump_debug(cout << "state : ") << endl;
	play->initial_actions_given_dealer[dr].dump_debug(cout << "dealer: ") << endl;
#endif
	player_options = play->initial_actions_per_state[state]
		& play->initial_actions_given_dealer[dr];
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
player_hand::double_down(const size_t p2) {
	hit_player(p2);
	action = DOUBLED_DOWN;
	player_options &= play->post_double_down_actions;
	// yes, in rare cases, surrender is allowed after double-down
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	TODO: support splitting un-paired hands!
	TODO: split-21s-are-naturals?
	Splits player's hand into two, using initial cards.  
	\param s1 second card for first hand
	\param s2 second card for second hand
	\param dr dealer's reveal card
 */
void
player_hand::split(player_hand& nh,
		const size_t s1, const size_t s2, const size_t dr) {
	const size_t split_card = state - pair_offset;
	// 21 should not be considered blackjack when splitting (variation)?
	deal_player(split_card, s1, false, dr);
	player_options &= play->post_split_actions;
	nh.deal_player(split_card, s2, false, dr);
	nh.player_options &= play->post_split_actions;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if 0
bool
player_hand::splittable(void) const {
	// TODO: allow variants that split nonmatching cards!
	return (cards.size() == 2) && (cards[0] == cards[1]);
}
#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
player_hand::dump_player(ostream& o) const {
	o << "player: " << cards << " (" << play->player_hit[state].name << ")";
	switch (action) {
	case DOUBLED_DOWN: o << " doubled-down"; break;
	case SURRENDERED: o << " surrendered"; break;
	default: break;
	}
	// TODO: show player_options.dump()
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
dealer_hand::dump_dealer(ostream& o) const {
	o << "dealer: " << cards << " (" << play->dealer_hit[state].name << ")";
	return o;
}

//=============================================================================
}	// end namespace blackjack

