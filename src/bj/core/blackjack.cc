// "bj/core/blackjack.cc"

#include <iostream>
#include <sstream>
#include <algorithm>

#include "bj/core/blackjack.hh"

#define	ENABLE_STACKTRACE			0
#include "util/stacktrace.hh"

/**
	Debug switch: print tables as they are computed.
 */
#define		DUMP_DURING_EVALUATE		0

namespace blackjack {
using std::ostringstream;
using std::cout;
using std::endl;
using std::fill;
using cards::card_name;
using cards::ACE;

static const std::ios_base::Init __init;

//=============================================================================
// Blackjack specific routines

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/// mapping of initial card (dealer or player) to state indices

/**
	index: card from deck
	value: player state index
	ACE is mapped to the lone ace state.
 */
const player_state_type
play_map::p_initial_card_map[card_values] =
	{ player_bust+1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
const dealer_state_type
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
const char play_map::player_final_states[][p_final_states] = {
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
const char play_map::dealer_final_states[][d_final_states] = {
	"17",
	"18",
	"19",
	"20",
	"21",
	"BJ",
	"bust",
	"push"	// switch variation only
};

vector<state_type>
play_map::__reverse_topo_order;

const vector<state_type>&
play_map::reverse_topo_order(play_map::__reverse_topo_order);

const int
play_map::init_reverse_topo_order =
	play_map::initialize_reverse_topo_order();

//-----------------------------------------------------------------------------
// class play_map method definitions
play_map::play_map(const variation& v) : var(v),
		dealer_hit(), player_hit(), player_resplit(), last_split(),
		post_hit_actions(action_mask::stand_hit),
		// some variations allow surrender-after-hit!
		post_double_down_actions(action_mask::stand),
		// some variations allow surrender-after-double-down!
		post_split_actions(action_mask::stand_hit)
		{
	reset();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Resets tables based on current set of variations, var.
 */
void
play_map::reset(void) {
	set_dealer_policy();
	compute_player_hit_state();
	compute_player_split_state();
	initialize_outcome_matrix();
	initialize_action_masks();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
play_map::initialize_action_masks(void) {
	if (var.double_after_split)
		post_split_actions += DOUBLE;
#if IMPROVED_SPLIT_ANALYSIS
	if (var.max_split_hands > 2)
#else
	if (var.resplit)
#endif
		post_split_actions += SPLIT;
	// for now, no surrender after split...
	// don't handle split-aces variations here
	// TODO: once-card-on-split-aces
	action_mask default_init = action_mask::stand_hit;
	if (var.some_double())
		default_init += DOUBLE;
	if (var.surrender_late)
		default_init += SURRENDER;
	action_mask default_init_split(default_init);
#if IMPROVED_SPLIT_ANALYSIS
	if (var.max_split_hands > 1)
#else
	if (var.split)
#endif
		default_init_split += SPLIT;
	fill(initial_actions_per_state.begin(),
		initial_actions_per_state.end(), default_init);
	// splittable states
	fill(&initial_actions_per_state[pair_offset],
		&initial_actions_per_state[pair_offset+card_values],
		default_init_split);
	// terminal states: stand only
	player_state_type i = 0;
	for ( ; i<p_action_states; ++i) {
		if (player_hit[i].is_terminal()) {
			initial_actions_per_state[i] = action_mask::stand;
		}
	}
	fill(initial_actions_given_dealer.begin(),
		initial_actions_given_dealer.end(), default_init +SPLIT);
	// restrict double-downs, e.g. only on player 10,11
	// 'other' includes A,x soft-hands
	if (var.double_H9) {
		initial_actions_per_state[9] += DOUBLE;
	} else {
		initial_actions_per_state[9] -= DOUBLE;
	}
	if (var.double_H10) {
		initial_actions_per_state[10] += DOUBLE;
		initial_actions_per_state[pair_offset +5] += DOUBLE;
	} else {
		initial_actions_per_state[10] -= DOUBLE;
		initial_actions_per_state[pair_offset +5] -= DOUBLE;
	}
	if (var.double_H11) {
		initial_actions_per_state[11] += DOUBLE;
	} else {
		initial_actions_per_state[11] -= DOUBLE;
	}
	// TODO: restrictions given dealer reveal
	// restrict surrenders, e.g. only vs. dealer A,10
#if DUMP_DURING_EVALUATE
	i = 0;
	for ( ; i<p_action_states; ++i) {
		initial_actions_per_state[i].dump_debug(
			cout << "initial state " << i << ": ") << endl;
	}
#endif
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
player_state_type
play_map::initial_card_player(const card_type p1) const {
	assert(p1 < card_values);
	return p_initial_card_map[p1];
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
dealer_state_type
play_map::initial_card_dealer(const card_type p1) const {
	assert(p1 < card_values);
	return d_initial_card_map[p1];
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool
play_map::is_player_pair(const player_state_type state) const {
	return state >= pair_offset && state < pair_offset +card_values;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool
play_map::is_player_terminal(const player_state_type state) const {
	return player_hit[state].is_terminal();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool
play_map::is_dealer_terminal(const dealer_state_type state) const {
	return dealer_hit[state].is_terminal();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
player_state_type
play_map::hit_player(const player_state_type state, const card_type c) const {
	assert(c < card_values);
	const state_machine::node& current(player_hit[state]);
	if (!current.is_terminal()) {
		return current[c];
	} else {
		return state;
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	For now, assume re-splittable.
	\param c is card enum from 0-9.
 */
player_state_type
play_map::split_player(const player_state_type state, const card_type c) const {
	assert(c < card_values);
	const player_state_type po = state -pair_offset;
	assert(po < card_values);
	const state_machine::node& current(player_resplit[po]);
	if (!current.is_terminal()) {
		return current[c];
	} else {
		return state;
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	For now, assume not re-splittable.
	\param c is card enum from 0-9.
 */
player_state_type
play_map::final_split_player(const player_state_type state,
		const card_type c) const {
	assert(c < card_values);
	const player_state_type po = state -pair_offset;
	assert(po < card_values);
	const state_machine::node& current(last_split[po]);
	if (!current.is_terminal()) {
		return current[c];
	} else {
		return state;
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
dealer_state_type
play_map::hit_dealer(const dealer_state_type state, const card_type c) const {
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
player_state_type
play_map::deal_player(const card_type p1, const card_type p2,
		const bool nat) const {
	STACKTRACE_VERBOSE;
	if (p1 == p2) {
		return pair_offset +p1;
	}
	player_state_type state = hit_player(initial_card_player(p1), p2);
	if (nat && (state == goal)) {
		return player_blackjack;
	}
	return state;
}

//-----------------------------------------------------------------------------
/**
	Translates player states to compact representation of final states.  
	\param i is player *action* state index.
	\return an index [0,p_final_states) into an array of player final states.
 */
player_state_type
play_map::player_final_state_map(const player_state_type i) {
	assert(i < p_action_states);
	if (i < stop) {
		return 0;	// represents <= 16
	} else if (i <= goal) {
		return i -stop +1;	// represents 17..21
	} else if (i == player_blackjack) {
		// represents blackjack state
		return p_final_states -2;
	} else if (i == player_bust) {
		// represents bust state
		return p_final_states -1;
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
	\param p player state, before mapping to final_state index
	\param d dealer state
	\return win-lose-push outcome
 */
const outcome&
play_map::lookup_outcome(const player_state_type p,
		const dealer_state_type d) const {
	return outcome_matrix[player_final_state_map(p)][d -stop];
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Given a spread of dealer final states and outcome_array (win/lose/push)
	compute the overall outcome odds for a given player (final) state.
	\param ps is a player_final_state in [0, p_final_states)
 */
void
play_map::compute_player_final_outcome(const player_state_type ps,
		const dealer_final_vector& dfv,
		outcome_odds& o) const {
	const outcome_array_type& v(outcome_matrix[ps]);
	dealer_state_type d;
	for (d=0; d < d_final_states; ++d) {     // dealer's final state
		const probability_type& p(dfv[d]);
		o.prob(v[d]) += p;              // adds to win/push/lose
	}       // end for d_final_states
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Given a spread of dealer final states, compute outcome odds
	for all player final states.
 */
void
play_map::compute_player_final_outcome_vector(const dealer_final_vector& dfv,
		player_final_outcome_vector& ps) const {
	fill(ps.begin(), ps.end(), outcome_odds());	// clear first
	player_state_type k;
	// player blackjack and bust is separate
	for (k=0; k < p_final_states; ++k) {    // player's final state
		outcome_odds& o(ps[k]);
		compute_player_final_outcome(k, dfv, o);
	}       // end for p_final_states
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
	const dealer_state_type soft_max = var.H17 ? 7 : 6;	// 6-16, 7-17
	// 0-21 are hard values (no ace)
	// 22 is blackjack
	// 23 is bust
	// 24 is push
	// 25-31 are soft 11 through soft 17
	dealer_hit.resize(d_action_states);
	// first handle hard values (no ACE)
	dealer_state_type i;
	card_type c;
	for (i=0; i<stop; ++i) {	// hit through hard 16
		ostringstream oss;
		oss << i;
		dealer_hit.name_state(i, oss.str());
	for (c=1; c<card_values; ++c) {
		const dealer_state_type t = i +c +1;	// sum value
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
		const dealer_state_type t = i+1;		// A = 1
		const dealer_state_type tA = t+card_values;	// A = 11
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
	const dealer_state_type offset = dealer_push;
	for (i=soft_min; i<=soft_max; ++i) {
		ostringstream oss;
		oss << "A";
		if (i>soft_min) { oss << "," << i-1; }
		dealer_hit.name_state(i+offset, oss.str());
	for (c=0; c<card_values; ++c) {		// A = 1 only
		const dealer_state_type t = i+c;
		const dealer_state_type tA = t +card_values +1;
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
	static const player_state_type soft_max = 10;
	// 0-21 are hard values (no ace)
	// 22-30 are soft 12 through soft 20
	// 31 is bust
	player_hit.resize(p_action_states);
	// first handle hard values (no ACE)
	player_state_type i;
	card_type c;
	for (i=0; i<goal; ++i) {	// hit through hard 16
		ostringstream oss;
		oss << i;
		player_hit.name_state(i, oss.str());
	for (c=1; c<card_values; ++c) {
		const player_state_type t = i +c +1;	// sum value
		if (t > goal) {			// bust
			player_hit.add_edge(i, player_bust, c, card_values);
		} else {
			player_hit.add_edge(i, t, c, card_values);
		}
	}
		// for c=0 (ace)
		const player_state_type t = i+1;		// A = 1
		const player_state_type tA = t+card_values;	// A = 11
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
	const player_state_type offset = player_bust+1;
	// soft-values: A, A1, A2, ...
	for (i=0; i<=card_values; ++i) {
		ostringstream oss;
		oss << "A";
		if (i) { oss << "," << i; }
		player_hit.name_state(i+offset, oss.str());
	for (c=0; c<card_values; ++c) {		// A = 1 only
		const player_state_type t = i+c+1;
		const player_state_type tA = t +card_values +1;
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
		const player_state_type equiv =
			(i ? (i+1)<<1 : i+player_soft +1);
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
	player_state_type i;
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
void
play_map::initialize_outcome_matrix(void) {
	player_state_type k;
	// player blackjack and bust is separate
	for (k=0; k < p_final_states -2; ++k) {	// player's final state
	dealer_state_type d;
		outcome_array_type& v(outcome_matrix[k]);
	// -1: blackjack, push, and bust states separate
	for (d=0; d < d_final_states -3; ++d) {	// dealer's final state
		outcome& o(v[d]);
		int diff = int(k -d -1);
		if (diff > 0) {
			o = WIN;
		} else if (!diff) {
			if (var.ties_win) {
				o = WIN;
			} else if (var.ties_lose) {
				o = LOSE;
			} else {
				o = PUSH;
			}
		} else {
			o = LOSE;
		}
	}
		v[d] = LOSE;	// dealer blackjack
		v[d+1] = WIN;	// dealer busts
		v[d+2] = PUSH;	// dealer pushes
	}
	outcome_array_type& pbj(outcome_matrix[k]);	// player blackjack
	outcome_array_type& px(outcome_matrix[k+1]);	// player busts
	fill(pbj.begin(), pbj.end(), WIN);
	pbj[d_final_states -3] = PUSH;			// both blackjack
	fill(px.begin(), px.end(), LOSE);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
        Just a reverse topological sort of reachable states
 */
int
play_map::initialize_reverse_topo_order(void) {
	// reordering for-loop: player states
	vector<state_type>& reorder(__reverse_topo_order);
	reorder.reserve(p_action_states);
	// backwards, starting from high hard value states
	state_type i;
	for (i=goal-1; i>goal-card_values; --i) {
		reorder.push_back(i);
	}
	// backwards from soft states (Aces)
	for (i=pair_offset-1; i>=player_soft; --i) {
		reorder.push_back(i);
	}
	// backwards from low hard values
	for (i=goal-card_values; i<p_action_states; --i) {
		reorder.push_back(i);
	}
	// evaluate split states at their face value
	for (i=p_action_states-1; i>=pair_offset; --i) {
		reorder.push_back(i);
	}
#if 0
	copy(reorder.begin(), reorder.end(),
		ostream_iterator<state_type>(cout, ","));
	cout << endl;
#endif
	return 1;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
play_map::dealer_final_table_header(ostream& o) {
	dealer_state_type d = 0;
	for (; d<d_final_states; ++d) {
		o << '\t' << dealer_final_states[d];
	}
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
play_map::player_final_table_header(ostream& o) {
	player_state_type j = 0;
	for (; j<p_final_states; ++j) {
		o << '\t' << player_final_states[j];
	}
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
play_map::dump_final_outcomes(ostream& o) const {
	static const char header[] = "P\\D";
	o << "Dealer vs. player final state outcomes." << endl;
	dealer_final_table_header(o << header) << endl;
	player_state_type k;
	for (k=0; k<p_final_states; ++k) {
		o << player_final_states[k];
		const outcome_array_type& v(outcome_matrix[k]);
	dealer_state_type d;
	for (d=0; d<d_final_states; ++d) {
		o << '\t' << v[d];
	}
		o << endl;
	}
	return o;
}

//=============================================================================
}	// end namespace blackjack

