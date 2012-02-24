// "hand.cc"

#include <iostream>
#include "hand.hh"
#include "blackjack.hh"

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

//=============================================================================
// class hand method definitions

void
hand::initial_card_player(const size_t p1) {
	// the string stores the card names, not card indices
	assert(p1 < card_values);
	cards.clear();
	cards.push_back(card_name[p1]);
	state = play->initial_card_player(p1);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
hand::initial_card_dealer(const size_t p1) {
	// the string stores the card names, not card indices
	assert(p1 < card_values);
	cards.clear();
	cards.push_back(card_name[p1]);
	state = play->initial_card_dealer(p1);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
hand::hit_player(const size_t p2) {
	assert(p2 < card_values);
	cards.push_back(card_name[p2]);
	state = play->hit_player(state, p2);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
hand::hit_dealer(const size_t p2) {
	assert(p2 < card_values);
	cards.push_back(card_name[p2]);
	state = play->hit_dealer(state, p2);
	if (cards.size() == 2 && state == goal) {
		state = dealer_blackjack;
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Assigns initial state based on player's first two cards.
	\param nat true if 21 should be considered natural blackjack
 */
void
hand::deal_player(const size_t p1, const size_t p2, const bool nat) {
	cards.clear();
	cards.push_back(card_name[p1]);
	cards.push_back(card_name[p2]);
	state = play->deal_player(p1, p2, nat);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
hand::double_down(const size_t p2) {
	hit_player(p2);
	action = DOUBLED_DOWN;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool
hand::splittable(void) const {
	return (cards.size() == 2) && (cards[0] == cards[1]);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if 0
/**
	Split back to single-card.
	Does not hit.
 */
void
hand::presplit(void) {
	const size_t p1 = card_index(cards[0]);
	initial_card_player(p1);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\param p2 new up-card for player.
	TODO: check for one-card on split-aces
 */
void
hand::split(const size_t p2) {
	if (splittable()) {
		presplit();
		hit_player(p2);
		// 21 here does not count as blackjack
	}
}
#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
hand::dump_player(ostream& o) const {
	o << "player: " << cards << " (" << play->player_hit[state].name << ")";
	switch (action) {
	case DOUBLED_DOWN: o << " doubled-down"; break;
	case SURRENDERED: o << " surrendered"; break;
	default: break;
	}
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
hand::dump_dealer(ostream& o) const {
	o << "dealer: " << cards << " (" << play->dealer_hit[state].name << ")";
	return o;
}

//=============================================================================
}	// end namespace blackjack

