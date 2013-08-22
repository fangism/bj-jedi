// "bj/core/state/player_hand.cc"

#include <iostream>
#include <sstream>
#include "bj/core/state/player_hand.hh"
#include "bj/core/blackjack.hh"

#define	DEBUG_HAND				0

namespace blackjack {
#if DEBUG_HAND
using std::cout;
using std::endl;
#endif
using cards::card_name;
using cards::card_symbols;
using cards::card_value_map;
using cards::state_machine;

//=============================================================================
// class player_hand_base method definitions

/**
	This constructor automatically sets the action mask depending
	on whether or not the state is splittable.
 */
player_hand_base::player_hand_base(const player_state_type ps,
		const play_map& play) :
		state(ps),
		player_options(
			play.is_player_pair(ps) ?
			action_mask::all : action_mask::all_but_split) {
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
player_hand_base::hit(const play_map& play, const card_type c) {
	assert(c < card_symbols);
	state = play.hit_player(state, card_value_map[c]);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
player_hand_base::split(const play_map& play) {
	state = play.p_initial_card_map[pair_card()];
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
player_hand_base::split(const play_map& play, const card_type c) {
	assert(c < card_symbols);
	const card_type cv = card_value_map[c];
	state = play.split_player(state, cv);
	// assume re-splittable
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
player_hand_base::final_split(const play_map& play, const card_type c) {
	assert(c < card_symbols);
	const card_type cv = card_value_map[c];
	state = play.final_split_player(state, cv);
	// assume not re-splittable
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
player_hand_base::dump_state_only(ostream& o, const state_machine& sm) const {
	return o << sm[state].name;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
player_hand_base::dump(ostream& o, const state_machine& sm) const {
	o << sm[state].name << ", ";
//	return player_options.dump_verbose(o);
	return player_options.dump_debug(o);	// more compact
}

//=============================================================================
// class player_hand method definitions

void
player_hand::initial_card_player(const card_type p1) {
	// the string stores the card names, not card indices
	assert(p1 < card_symbols);
	cards.clear();
	cards.push_back(card_name[p1]);
	state = play->initial_card_player(card_value_map[p1]);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Appends card and updates allowable actions.
 */
void
player_hand::hit_player(const card_type p2) {
	player_hand_base::hit(*play, p2);
	cards.push_back(card_name[p2]);
	player_options &= play->post_hit_actions;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Assigns initial state based on player's first two cards.
	\param p1,p2 first two player's cards
	\param nat true if 21 should be considered natural blackjack
	\param dr dealer's reveal card
 */
void
player_hand::deal_player(const card_type p1, const card_type p2, const bool nat, 
		const card_type dr) {
	cards.clear();
	cards.push_back(card_name[p1]);
	cards.push_back(card_name[p2]);
	const card_type dv = card_value_map[dr];
	state = play->deal_player(card_value_map[p1], card_value_map[p2], nat);
#if DEBUG_HAND
	play->initial_actions_per_state[state].dump_debug(cout << "state : ") << endl;
	play->initial_actions_given_dealer[dv].dump_debug(cout << "dealer: ") << endl;
#endif
	player_options = play->initial_actions_per_state[state]
		& play->initial_actions_given_dealer[dv];
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
player_hand::double_down(const card_type p2) {
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
		const card_type s1, const card_type s2, const card_type dr) {
	const card_type split_card = pair_card();
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

//=============================================================================
}	// end namespace blackjack

