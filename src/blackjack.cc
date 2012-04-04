// "blackjack.cc"

#include <iostream>
#include <sstream>
#include "blackjack.hh"

/**
	Debug switch: print tables as they are computed.
 */
#define		DUMP_DURING_EVALUATE		0

namespace blackjack {
using std::ostringstream;
using std::cout;
using std::endl;
using cards::card_name;
using cards::ACE;

//=============================================================================
// Blackjack specific routines

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/// mapping of initial card (dealer or player) to state indices

/**
	index: card from deck
	value: player state index
	ACE is mapped to the lone ace state.
 */
const size_t
play_map::p_initial_card_map[card_values] =
	{ player_bust+1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
const size_t
play_map::d_initial_card_map[card_values] =
	{ dealer_push+1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/// follows enum player_choice in "enums.hh"
const char*
action_names[] = {
	"nil",
	"stand",
	"hit",
	"double-down",
	"split",
	"surrender",
	"bookmark",
	"count",
	"hint",
	"optimal"
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Final states applicable to player.
	TODO: this table should be redundant from player 
	state machine, which already contains strings.
 */
const char play_map::player_final_states[][player_states] = {
	"<=16",
	"17",
	"18",
	"19",
	"20",
	"21",
	"BJ",
	"bust"
};

/**
	Final states applicable to dealer.
	TODO: this table should be redundant from dealer
	state machine, which already contains strings.
 */
const char play_map::dealer_final_states[][dealer_states] = {
	"17",
	"18",
	"19",
	"20",
	"21",
	"BJ",
	"bust",
	"push"	// switch variation only
};

play_map::outcome_matrix_type		play_map::__outcome_matrix;

const play_map::outcome_matrix_type&
play_map::outcome_matrix = play_map::__outcome_matrix;

const int play_map::init_outcome_matrix = play_map::compute_final_outcomes();

//-----------------------------------------------------------------------------
// class play_map method definitions
play_map::play_map(const variation& v) : var(v),
		dealer_hit(), player_hit(), player_resplit(), last_split() {
	set_dealer_policy();
	compute_player_hit_state();
	compute_player_split_state();
	compute_final_outcomes();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t
play_map::initial_card_player(const size_t p1) const {
	assert(p1 < card_values);
	return p_initial_card_map[p1];
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t
play_map::initial_card_dealer(const size_t p1) const {
	assert(p1 < card_values);
	return d_initial_card_map[p1];
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool
play_map::is_player_terminal(const size_t state) const {
	return player_hit[state].is_terminal();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool
play_map::is_dealer_terminal(const size_t state) const {
	return dealer_hit[state].is_terminal();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t
play_map::hit_player(const size_t state, const size_t c) const {
	assert(c < card_values);
	const state_machine::node& current(player_hit[state]);
	if (!current.is_terminal()) {
		return current[c];
	} else {
		return state;
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t
play_map::hit_dealer(const size_t state, const size_t c) const {
	assert(c < card_values);
	const state_machine::node& current(dealer_hit[state]);
	if (!current.is_terminal()) {
		return current[c];
	} else {
		return state;
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Should result in splittable states.
	\param nat true if 21 should be considered natural blackjack,
		false if should just be considered 21.
 */
size_t
play_map::deal_player(const size_t p1, const size_t p2, const bool nat) const {
	if (p1 == p2) {
		return pair_offset +p1;
	}
	size_t state = hit_player(initial_card_player(p1), p2);
	if (nat && (state == goal)) {
		return player_blackjack;
	}
	return state;
}

//-----------------------------------------------------------------------------
/**
	Translates player states to compact representation of final states.  
	\param i is player *action* state index.
	\return an index [0,player_states) into an array of player final states.
 */
size_t
play_map::player_final_state_map(const size_t i) {
	assert(i < p_action_states);
	if (i < stop) {
		return 0;	// represents <= 16
	} else if (i <= goal) {
		return i -stop +1;	// represents 17..21
	} else if (i == player_blackjack) {
		// represents blackjack state
		return player_states -2;
	} else if (i == player_bust) {
		// represents bust state
		return player_states -1;
	} else if (i < pair_offset) {
		// is Ace state, take the higher value
		return player_final_state_map(i -player_bust +card_values);
	} else if (i == pair_offset) {
		// pair of Aces
		return player_final_state_map(player_soft +1);
	} else {
		// other pair
		return player_final_state_map((i -pair_offset +1) << 1);
	}
	// does not count split states
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Build the dealer's state machine, independent of probability.
	\param H17 If H17, then dealer must hit on a soft 17, else must stand.  
	\param push22, 22 is dealer push
 */
void
play_map::set_dealer_policy(void) {
	// enumerating edges:
	const size_t soft_max = var.H17 ? 7 : 6;	// 6-16, 7-17
	// 0-21 are hard values (no ace)
	// 22 is blackjack
	// 23 is bust
	// 24 is push
	// 25-31 are soft 11 through soft 17
	dealer_hit.resize(d_action_states);
	// first handle hard values (no ACE)
	size_t i, c;
	for (i=0; i<stop; ++i) {	// hit through hard 16
		ostringstream oss;
		oss << i;
		dealer_hit.name_state(i, oss.str());
	for (c=1; c<card_values; ++c) {
		const size_t t = i +c +1;	// sum value
		if (t > goal) {			// bust
			if ((t == goal+1) && var.push22) {
				dealer_hit.add_edge(i, dealer_push, c, card_values);
			} else {
				dealer_hit.add_edge(i, dealer_bust, c, card_values);
			}
		} else {
			dealer_hit.add_edge(i, t, c, card_values);
		}
	}
		// for c=0 (ace)
		const size_t t = i+1;		// A = 1
		const size_t tA = t+card_values;	// A = 11
		if (t >= soft_min && t <= soft_max) {
			dealer_hit.add_edge(i, t +dealer_soft -soft_min, 0, card_values);
		} else if (tA <= goal) {
			dealer_hit.add_edge(i, tA, 0, card_values);
		} else if (t <= goal) {
			dealer_hit.add_edge(i, t, 0, card_values);
		} else {
			dealer_hit.add_edge(i, dealer_bust, 0, card_values);
		}
	}
	for ( ; i<=goal; ++i) {
		ostringstream oss;
		oss << i;
		dealer_hit.name_state(i, oss.str());
	}
	// soft-values
	const size_t offset = dealer_push;
	for (i=soft_min; i<=soft_max; ++i) {
		ostringstream oss;
		oss << "A";
		if (i>soft_min) { oss << "," << i-1; }
		dealer_hit.name_state(i+offset, oss.str());
	for (c=0; c<card_values; ++c) {		// A = 1 only
		const size_t t = i+c;
		const size_t tA = t +card_values +1;
		if (tA > goal) {
			dealer_hit.add_edge(i+offset, t +1, c, card_values);
		} else if (tA > soft_max +card_values) {
			dealer_hit.add_edge(i+offset, tA, c, card_values);
		} else {
			dealer_hit.add_edge(i+offset, t +dealer_soft, c, card_values);
		}
	}
	}
	// might as well name the remaining Ace states
	for ( ; i+offset<d_action_states; ++i) {
		ostringstream oss;
		oss << "A," << i-1;
		dealer_hit.name_state(i+offset, oss.str());
	}
	dealer_hit.name_state(dealer_blackjack, "BJ");
	dealer_hit.name_state(dealer_bust, "bust");
	dealer_hit.name_state(dealer_push, "push");
	// the bust-state is terminal
	dealer_hit.check();
#if DUMP_DURING_EVALUATE
	dump_dealer_policy(cout);
#endif
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
play_map::dump_dealer_policy(ostream& o) const {
	o << "Dealer's action table:\n";
	return dealer_hit.dump(o);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Compute player hit states.  
	This is just an assessment of state transitions on hitting, 
	not any indication of whether hitting is a advisiable.  :)
	\pre depends on nothing
	\post computes player_hit state transition table.
 */
void
play_map::compute_player_hit_state(void) {
	static const size_t soft_max = 10;
	// 0-21 are hard values (no ace)
	// 22-30 are soft 12 through soft 20
	// 31 is bust
	player_hit.resize(p_action_states);
	// first handle hard values (no ACE)
	size_t i, c;
	for (i=0; i<goal; ++i) {	// hit through hard 16
		ostringstream oss;
		oss << i;
		player_hit.name_state(i, oss.str());
	for (c=1; c<card_values; ++c) {
		const size_t t = i +c +1;	// sum value
		if (t > goal) {			// bust
			player_hit.add_edge(i, player_bust, c, card_values);
		} else {
			player_hit.add_edge(i, t, c, card_values);
		}
	}
		// for c=0 (ace)
		const size_t t = i+1;		// A = 1
		const size_t tA = t+card_values;	// A = 11
		if (t >= soft_min && t <= soft_max) {
			player_hit.add_edge(i, t +player_soft -soft_min, 0, card_values);
		} else if (tA <= goal) {
			player_hit.add_edge(i, tA, 0, card_values);
		} else if (t <= goal) {
			player_hit.add_edge(i, t, 0, card_values);
		} else {
			player_hit.add_edge(i, player_bust, 0, card_values);
		}
	}
	for ( ; i<=goal; ++i) {
		ostringstream oss;
		oss << i;
		player_hit.name_state(i, oss.str());
	}
	// player doesn't have a push state like the dealer
	const size_t offset = player_bust+1;
	// soft-values: A, A1, A2, ...
	for (i=0; i<=card_values; ++i) {
		ostringstream oss;
		oss << "A";
		if (i) { oss << "," << i; }
		player_hit.name_state(i+offset, oss.str());
	for (c=0; c<card_values; ++c) {		// A = 1 only
		const size_t t = i+c+1;
		const size_t tA = t +card_values +1;
		if (tA > goal) {
			player_hit.add_edge(i+offset, t +1, c, card_values);
		} else if (tA > soft_max +card_values) {
			player_hit.add_edge(i+offset, tA, c, card_values);
		} else {
			player_hit.add_edge(i+offset, t +player_soft, c, card_values);
		}
	}
	}
	player_hit.name_state(player_blackjack, "BJ");
	player_hit.name_state(player_bust, "bust");
	// HERE: splits
	for (i=0; i<card_values; ++i) {
		ostringstream oss;
		const char cc = card_name[i];
		oss << cc << ',' << cc;
		player_hit.name_state(i+pair_offset, oss.str());
		// copy from non-paired equivalent value
		const size_t equiv = (i ? (i+1)<<1 : i+player_soft +1);
		// b/c cards are indexed A234...
		player_hit.copy_edge_set(equiv, i+pair_offset);
	}
	// the bust-state is terminal
	player_hit.check();
#if DUMP_DURING_EVALUATE
	dump_player_hit_state(cout) << endl;
#endif
}	// end compute_player_hit_state()

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
play_map::dump_player_hit_state(ostream& o) const {
	o << "Player's hit transition table:\n";
	return player_hit.dump(o);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Computes split transition tables (final and resplit).
	\pre player_hit already computed by compute_player_hit_state
	\post split-transition table based on hitting on the split card.
 */
void
play_map::compute_player_split_state(void) {
	last_split.resize(card_values);
	size_t i;
	for (i=ACE; i<card_values; ++i) {
		// TODO: just use string and +=
		ostringstream oss;
		const char c = card_name[i];
		oss << c << ',' << c;
		last_split.name_state(i, oss.str());
		last_split.copy_edge_set(player_hit,
			p_initial_card_map[i], i);
	}
	// copy table, then adjust
	player_resplit = last_split;
	i = var.resplit_aces ? ACE : ACE+1;
	// if resplitting aces forbidden, leave ACE row as-is
	for (; i<card_values; ++i) {
		player_resplit.add_edge(i, pair_offset+i, i, card_values);
	}
#if DUMP_DURING_EVALUATE
	dump_player_split_state(cout);
#endif
}	// end compute_player_split_state()

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
play_map::dump_player_split_state(ostream& o) const {
	o << "Player's split+hit (final) transition table:\n";
	last_split.dump(o) << endl;
	o << "Player's split+hit (resplit) transition table:\n";
	return player_resplit.dump(o) << endl;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int
play_map::compute_final_outcomes(void) {
	size_t k;
	// player blackjack and bust is separate
	for (k=0; k < player_states -2; ++k) {	// player's final state
	size_t d;
		outcome_array_type& v(__outcome_matrix[k]);
	// -1: blackjack, push, and bust states separate
	for (d=0; d < dealer_states -3; ++d) {	// dealer's final state
		outcome& o(v[d]);
		int diff = k -d -1;
		if (diff > 0) {
			o = WIN;
		} else if (!diff) {
			o = PUSH;
		} else {
			o = LOSE;
		}
	}
		v[d] = LOSE;	// dealer blackjack
		v[d+1] = WIN;	// dealer busts
		v[d+2] = PUSH;	// dealer pushes
	}
	outcome_array_type& pbj(__outcome_matrix[k]);	// player blackjack
	outcome_array_type& px(__outcome_matrix[k+1]);	// player busts
	std::fill(pbj.begin(), pbj.end(), WIN);
	pbj[dealer_states -3] = PUSH;			// both blackjack
	std::fill(px.begin(), px.end(), LOSE);
	return 1;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
play_map::dump_final_outcomes(ostream& o) {
	static const char header[] = "P\\D\t17\t18\t19\t20\t21\tBJ\tbust\tpush";
	o << "Dealer vs. player final state outcomes." << endl;
	o << header << endl;
	size_t k;
	for (k=0; k<player_states; ++k) {
		o << player_final_states[k];
		const outcome_array_type& v(outcome_matrix[k]);
	size_t d;
	for (d=0; d<player_states; ++d) {
		switch (v[d]) {
		case WIN: o << "\twin"; break;
		case LOSE: o << "\tlose"; break;
		case PUSH: o << "\tpush"; break;
		}
	}
		o << endl;
	}
	return o;
}

//=============================================================================
}	// end namespace blackjack

