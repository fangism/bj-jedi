// "blackjack.cc"

#include <cassert>
#include <algorithm>
#include <functional>
#include <numeric>		// for accumulate
#include <limits>		// for numeric_limits
#include <iostream>
#include <sstream>
#include <iterator>
#include <iomanip>
#include <map>
#include <cmath>		// for fabs
#include "blackjack.hh"
#include "util/array.tcc"
#include "util/probability.tcc"

/**
	Debug switch: print tables as they are computed.
 */
#define		DUMP_DURING_EVALUATE		0

namespace blackjack {

using std::fill;
using std::copy;
using std::for_each;
using std::transform;
using std::inner_product;
using std::accumulate;
using std::mem_fun_ref;
using std::ostringstream;
using std::ostream_iterator;
using std::cout;
using std::cerr;
using std::endl;
using std::multimap;
using std::make_pair;
using std::setw;
using std::setprecision;
using std::istringstream;
using util::normalize;

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

const char
strategy::action_key[] = "-SHDPR";
// - = nil (invalid)
// S = stand
// H = hit
// D = double
// P = split pair
// R = surrender (run!)

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
	TODO: this isn't even used...
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

//-----------------------------------------------------------------------------
// class variation method definitions

static
const char*
yn(const bool y) {
	return y ? "yes" : "no";
}

ostream&
variation::dump(ostream& o) const {
	o << "number of decks: " << num_decks << endl;
	o << "maximum penetration: " << maximum_penetration << endl;
	o << "dealer soft 17: " << (H17 ? "hits" : "stands") << endl;
	o << "player early surrender: " << yn(surrender_early) << endl;
	o << "player late surrender: " << yn(surrender_late) << endl;
	o << "dealer peek on 10: " << yn(peek_on_10) << endl;
	o << "dealer peek on A: " << yn(peek_on_Ace) << endl;
	o << "player loses ties: " << yn(ties_lose) << endl;
	o << "player wins ties: " << yn(ties_win) << endl;
	o << "player may double-down on 9: " << yn(double_H9) << endl;
	o << "player may double-down on 10: " << yn(double_H10) << endl;
	o << "player may double-down on 11: " << yn(double_H11) << endl;
	o << "player may double-down on other: " << yn(double_other) << endl;
	o << "player double after split: " << yn(double_after_split) << endl;
	o << "player may split: " << yn(split) << endl;
	o << "player may re-split: " << yn(resplit) << endl;
	o << "player receives only one card on each split ace: "
		<< yn(one_card_on_split_aces) << endl;
	o << "dealer 22 pushes against player: " << yn(push22) << endl;
	o << "blackjack pays: " << bj_payoff << endl;
	o << "insurance pays: " << insurance << endl;
	o << "surrender penalty: " << surrender_penalty << endl;
	o << "double-down multiplier: " << double_multiplier << endl;
	return o;
}

//-----------------------------------------------------------------------------
// class play_map method definitions
play_map::play_map(const variation& v) : var(v),
		dealer_hit(), player_hit(), player_resplit(), last_split() {
	set_dealer_policy();
	compute_player_hit_state();
	compute_player_split_state();
}

//-----------------------------------------------------------------------------
// class strategy method definitions

void
strategy::set_card_distribution(const deck_distribution& o) {
	card_odds = o;
	update_player_initial_state_odds();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
strategy::set_card_distribution(const deck_count_type& o) {
	deck_distribution t;
	copy(o.begin(), o.end(), t.begin());	// convert int to real
	normalize(t);
	set_card_distribution(t);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Compute the initial spread of player states.  
	Exclude the player's initial "blackjack" state because
	the playoff is accounted for in player_initial_edges.
	TODO: redo pairs
	\pre player_hit state transition table computed.
 */
void
strategy::update_player_initial_state_odds(void) {
	fill(player_initial_state_odds.begin(),
		player_initial_state_odds.end(), 0.0);
	size_t i;
for (i=0; i<card_values; ++i) {
	const probability_type& f(card_odds[i]); // / not_bj_odds;
	// scale up, to normalize for lack of blackjack
	size_t j;
	for (j=0; j<card_values; ++j) {
		const probability_type fs = f * card_odds[j];
		if (i == j) {		// splittable pairs
			player_initial_state_odds[pair_offset +i] += fs;
		} else {
			const size_t k =
				play.player_hit[play.p_initial_card_map[i]][j];
			player_initial_state_odds[k] += fs;
		}
	}
}
	// initial 21 is blackjack
	player_initial_state_odds[player_blackjack] =
		player_initial_state_odds[goal];
	player_initial_state_odds[goal] = 0.0;
//	normalize(player_initial_state_odds);	// normalizing not needed
#if DUMP_DURING_EVALUATE
	dump_player_initial_state_odds(cout) << endl;
#endif
}	// end update_player_initial_state_odds

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Player's initial state spread.
 */
ostream&
strategy::dump_player_initial_state_odds(ostream& o) const {
	o << "Player initial state odds:\n";
	// show enumeration of states
	copy(player_initial_state_odds.begin(), 
		player_initial_state_odds.end(), 
		ostream_iterator<probability_type>(o, "\n"));
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	To be called after card distribution is set.  
 */
void
strategy::evaluate(void) {
	compute_dealer_final_table();
	compute_player_stand_odds();
	compute_action_expectations();
	compute_reveal_edges();
	compute_overall_edge();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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
	Folds probability vector of player states to compact representation.
	Result goes into r.  
 */
void
strategy::player_final_state_probabilities(const probability_vector& s, 
		player_final_state_probability_vector& r) {
	assert(s.size() == p_action_states);
	fill(r.begin(), r.end(), 0.0);
	size_t i;
	for (i=0; i<p_action_states; ++i) {
		r[play_map::player_final_state_map(i)] += s[i];
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if 0
strategy::strategy() : 
	var(), card_odds(standard_deck_distribution), dealer_hit() {
	// only depends on H17:
	// don't *have* to do this right away...
	set_dealer_policy();
	compute_player_hit_state();
	compute_player_split_state();
	update_player_initial_state_odds();
}
#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
strategy::strategy(const play_map& v) : 
	var(v.var),
	play(v), 
	card_odds(standard_deck_distribution) {
	// only depends on H17:
	// don't *have* to do this right away...
#if 0
	set_dealer_policy();
	compute_player_hit_state();
	compute_player_split_state();
#endif
	update_player_initial_state_odds();
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
	return dealer_hit.dump(o) << endl;
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
	dump_player_hit_state(cout);
#endif
}	// end compute_player_hit_state()

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
play_map::dump_player_hit_state(ostream& o) const {
	o << "Player's hit transition table:\n";
	return player_hit.dump(o) << endl;
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
	for (i=0; i<card_values; ++i) {
		ostringstream oss;
		const char c = card_name[i];
		oss << c << ',' << c;
		last_split.name_state(i, oss.str());
		last_split.copy_edge_set(player_hit,
			p_initial_card_map[i], i);
	}
	// copy table, then adjust
	player_resplit = last_split;
	for (i=0; i<card_values; ++i) {
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
/**
	Given dealer's reveal card, calculate spread of dealer's final state.
	Include odds of dealer blackjack.
	\post computes dealer_final_given_revealed, and _post_peek.
	\pre dealer_hit state-machine already computed.
 */
void
strategy::compute_dealer_final_table(void) {
	// 17, 18, 19, 20, 21, BJ, bust, push
	deck_distribution cards_no_ten(card_odds);
	deck_distribution cards_no_ace(card_odds);
	cards_no_ten[TEN] = 0.0;
	cards_no_ace[ACE] = 0.0;
// no need to normalize if accounting for dealer blackjack here
//	normalize(cards_no_ten);
//	normalize(cards_no_ace);
	// use cards_no_ten for the case where dealer does NOT have blackjack
	// use cards_no_ace when dealer draws 10, and is not blackjack
	//	this is called 'peek'
	size_t j;
	for (j=0 ; j<card_values; ++j) {
		probability_vector init(play.dealer_hit.get_states().size(), 0.0);
		probability_vector final(init.size());
		init[play_map::d_initial_card_map[j]] = 1.0;
		// for the first iteration
		// isolate the case when dealer shows an ACE or TEN
		// but hole card is known to NOT give dealer blackjack!
		if (j == ACE) {
			if (var.peek_on_Ace) {
			final = init;
			play.dealer_hit.convolve(cards_no_ten, final, init);
			// then solve the rest
			}
		} else if (j == TEN) {
			if (var.peek_on_10) {
			final = init;
			play.dealer_hit.convolve(cards_no_ace, final, init);
			}
		}
		play.dealer_hit.solve(card_odds, init, final);
		dealer_final_vector_type& row(dealer_final_given_revealed[j]);
		dealer_final_vector_type::const_iterator i(&final[stop]);
		copy(i, i+dealer_states, row.begin());
		// odds of dealer blackjack
		if (j == ACE) {
			 row[dealer_states-3] = card_odds[TEN];
		} else if (j == TEN) {
			 row[dealer_states-3] = card_odds[ACE];
		}
	}
	// post-peek normalizing
	// start by copying, zero out the blackjack odds, and re-normalize
	dealer_final_given_revealed_post_peek = dealer_final_given_revealed;
{
	dealer_final_vector_type&
		ace_row(dealer_final_given_revealed_post_peek[ACE]);
	probability_type& dbj(ace_row[dealer_blackjack- stop]);
	if (var.peek_on_Ace && dbj > 0.0) {
		dbj = 0.0;
		normalize(ace_row);
	}
}{
	dealer_final_vector_type&
		ten_row(dealer_final_given_revealed_post_peek[TEN]);
	probability_type& dbj(ten_row[dealer_blackjack- stop]);
	if (var.peek_on_10 && dbj > 0.0) {
		dbj = 0.0;
		normalize(ten_row);
	}
}
#if DUMP_DURING_EVALUATE
	dump_dealer_final_table(cout) << endl;
#endif
}	// end compute_dealer_final_table()

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
strategy::dump_dealer_final_table(ostream& o) const {
	o << "Dealer's final state odds (pre-peek):\n";
	ostream_iterator<probability_type> osi(o, "\t");
{
	const dealer_final_matrix::const_iterator
		b(dealer_final_given_revealed.begin()),
		e(dealer_final_given_revealed.end());
	dealer_final_matrix::const_iterator i(b);
	o << "show\\do\t17\t18\t19\t20\t21\tBJ\tbust\tpush\n";
	o << setprecision(4);
	for ( ; i!=e; ++i) {
		o << play.dealer_hit[play_map::d_initial_card_map[i-b]].name << '\t';
		copy(i->begin(), i->end(), osi);
		o << endl;
	}
}
	o << endl;
{
	o << "Dealer's final state odds (post-peek):\n";
	const dealer_final_matrix::const_iterator
		b(dealer_final_given_revealed_post_peek.begin()),
		e(dealer_final_given_revealed_post_peek.end());
	dealer_final_matrix::const_iterator i(b);
	o << "show\\do\t17\t18\t19\t20\t21\tBJ\tbust\tpush\n";
	o << setprecision(4);
	for ( ; i!=e; ++i) {
		o << play.dealer_hit[play_map::d_initial_card_map[i-b]].name << '\t';
		copy(i->begin(), i->end(), osi);
		o << endl;
	}
}
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\pre already called compute_dealer_final_table() to
		compute dealer_final_given_revealed, pre/post-peek.
	\post populates values of ::player_stand and ::player_stand_edges
	\param bjp blackjack-payoff used to weigh player's blackjack edge
 */
void
strategy::compute_showdown_odds(const dealer_final_matrix& dfm,
		const edge_type& bjp, 
		outcome_matrix& stand, player_stand_edges_matrix& edges) {
	// represents: <=16, 17, 18, 19, 20, 21, BJ, bust
	static const size_t d_bj_ind = dealer_states -3;
	static const size_t p_bj_ind = player_states -2;
	size_t j;
	for (j=0; j < card_values; ++j) {	// dealer's revealed card, initial state
		const dealer_final_vector_type& dfv(dfm[j]);	// pre-peek!
		outcome_vector& ps(stand[j]);
	size_t k;
	// player blackjack and bust is separate
	for (k=0; k < player_states -2; ++k) {	// player's final state
		outcome_odds& o(ps[k]);
	size_t d;
	// -1: blackjack, push, and bust states separate
	for (d=0; d < dealer_states -3; ++d) {	// dealer's final state
		const probability_type& p(dfv[d]);
		int diff = k -d;
		if (diff > 1) {
			o.win += p;
		} else if (diff == 1) {
			o.push += p;
		} else {
			o.lose += p;
		}
	}
		o.lose += dfv[d];	// dealer blackjack
		o.win += dfv[d+1];	// dealer busts
		o.push += dfv[d+2];	// dealer pushes
	}
		const probability_type& d_bj(dfv[d_bj_ind]);
		ps[k].win += 1.0 -d_bj;	// player blackjack, dealer none
		ps[k].push += d_bj;	// both blackjack
		ps[k+1].lose += 1.0;	// player busts
	}
	// now compute player edges
	for (j=0; j<card_values; ++j) {	// dealer's reveal card
		const outcome_vector& ps(stand[j]);
		player_stand_edges_vector& ev(edges[j]);
		transform(ps.begin(), ps.end(), ev.begin(), 
			mem_fun_ref(&outcome_odds::edge));
		// TODO: or mem_fun_ref(&outcome_odds::ratioed_edge)?
		// compensate for player blackjack, pays 3:2
		ev[p_bj_ind] = ps[p_bj_ind].weighted_edge(bjp, 1.0);
	}
}	// end compute_showdown_odds

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
/**
	\pre dealer_final_given_revealed computed (pre/post-peek)
	\post computes player_stand (odds-given-reveal) and
		player_stand_edges (weighted expected outcome).
	TODO: evaluate peek for 10 and peek for Ace separately, independently
 */
void
strategy::compute_player_stand_odds(void) {
	// pre-peek edges
	compute_showdown_odds(dealer_final_given_revealed, var.bj_payoff,
		player_stand, player_stand_edges);
	// post-peek edges
	compute_showdown_odds(dealer_final_given_revealed_post_peek, var.bj_payoff,
		player_stand_post_peek, player_stand_edges_post_peek);
#if DUMP_DURING_EVALUATE
	dump_player_stand_odds(cout) << endl;
#endif
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
operator << (ostream& o, const strategy::outcome_odds& r) {
	o << r.win << '|' << r.push << '|' << r.lose;
#if 0
	// confirm sum
	o << '=' << r.win +r.push +r.lose;
#endif
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
strategy::dump_outcome_vector(const outcome_vector& v, ostream& o) {
	const outcome_vector::const_iterator b(v.begin()), e(v.end());
	outcome_vector::const_iterator i(b);
	o << "win";
	for (; i!=e; ++i) {
		o << '\t' << i->win;
	}
	o << endl;
	o << "push";
	for (i=b; i!=e; ++i) {
		o << '\t' << i->push;
	}
	o << endl;
	o << "lose";
	for (i=b; i!=e; ++i) {
		o << '\t' << i->lose;
	}
	return o << endl;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\param d state machine with names of states
 */
ostream&
strategy::__dump_player_stand_odds(ostream& o, const outcome_matrix& m, 
		const state_machine& d) {
	const outcome_matrix::const_iterator
		b(m.begin()), e(m.end());
	outcome_matrix::const_iterator i(b);
	o << "D\\P";
	size_t j;
	for (j=0; j<player_states; ++j) {
		o << '\t' << play_map::player_final_states[j];
	}
	o << endl;
	o << setprecision(3);
	for ( ; i!=e; ++i) {
		o << d[play_map::d_initial_card_map[i-b]].name << endl;
		dump_outcome_vector(*i, o);
		o << endl;
	}
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
strategy::__dump_player_stand_edges(ostream& o,
		const player_stand_edges_matrix& m, 
		const state_machine& d) {
	// dump the player's edges (win -lose)
	const player_stand_edges_matrix::const_iterator
		b(m.begin()), e(m.end());
	player_stand_edges_matrix::const_iterator i(b);
	o << "D\\P";
	size_t j;
	for (j=0; j<player_states; ++j) {
		o << '\t' << play_map::player_final_states[j];
	}
	o << endl;
	for ( ; i!=e; ++i) {
		o << d[play_map::d_initial_card_map[i-b]].name << '\t';
//		o << setprecision(3);
		copy(i->begin(), i->end(),
			ostream_iterator<probability_type>(o, "\t"));
		o << endl;
	}
	return o;
}
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
strategy::dump_player_stand_odds(ostream& o) const {
{
//	o << "Player's stand odds (win|push|lose):\n";
	o << "Player's stand odds (pre-peek):\n";
	__dump_player_stand_odds(o, player_stand, play.dealer_hit);
	o << "Player's stand odds (post-peek):\n";
	__dump_player_stand_odds(o, player_stand_post_peek, play.dealer_hit);
}{
	// dump the player's edges (win -lose)
	o << "Player's stand edges (pre-peek):\n";
	__dump_player_stand_edges(o, player_stand_edges, play.dealer_hit) << endl;
	o << "Player's stand edges (post-peek):\n";
	__dump_player_stand_edges(o, player_stand_edges_post_peek, play.dealer_hit);
}
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	This is the main strategy solver.  
	No action is needed if outcome is a blackjack for either the
	dealer or player, thus decisions are based on post-peek edges,
	(i.e. after dealer has checked for blackjack).
 */
void
strategy::compute_action_expectations(void) {
	const player_stand_edges_matrix&
		pse(player_stand_edges_post_peek);
	// recall, table is transposed
// stand
	size_t i, j;
	for (i=0; i<p_action_states; ++i) {
	for (j=0; j<card_values; ++j) {
		const probability_type&
			edge(pse[j][play_map::player_final_state_map(i)]);
		player_actions[i][j].stand = edge;
	}
	}
// double-down (hit-once, then stand)
	// TODO: this could be computed at the same time with hits
	// if done in reverse order
	for (i=0; i<p_action_states; ++i) {
	if (play.player_hit[i].size()) {
		probability_vector before_hit(p_action_states, 0.0);
		before_hit[i] = 1.0;	// unit vector
		probability_vector after_hit;
		play.player_hit.convolve(card_odds, before_hit, after_hit);
		player_final_state_probability_vector fin;
		player_final_state_probabilities(after_hit, fin);
	for (j=0; j<card_values; ++j) {	// dealer shows
		// static_assert
		assert(size_t(player_final_state_probability_vector::Size)
			== size_t(outcome_vector::Size));
		probability_type& p(player_actions[i][j].double_down);
		// perform inner_product, weighted expectations
		size_t k;
		for (k=0; k<player_states; ++k) {
			const probability_type& edge(pse[j][k]);
			p += fin[k] *edge;
		}
		p *= var.double_multiplier;	// because bet is doubled
	}
	} else {			// is terminal state, irrelevant
	for (j=0; j<card_values; ++j) {
		player_actions[i][j].double_down = -var.double_multiplier;
	}
	}
	}
// hit (and optimize choices, excluding splits)
	// really, just a reverse topological sort of reachable states
	vector<size_t> reorder;		// reordering for-loop: player states
	reorder.reserve(p_action_states);
	// backwards, starting from high hard value states
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
	copy(reorder.begin(), reorder.end(), ostream_iterator<size_t>(cout, ","));
	cout << endl;
#endif
	size_t r;
	for (r=0; r<reorder.size(); ++r) {
		const size_t ii = reorder[r];
		const state_machine::node& ni(play.player_hit[ii]);
		assert(ni.size());
	for (j=0; j<card_values; ++j) {
		expectations& p(player_actions[ii][j]);
		p.hit = 0.0;
	size_t k;
	for (k=0; k<card_values; ++k) {
//		cout << "i,j=" << ii << ',' << j <<
//			", card-index " << k << ", odds=" << card_odds[k];
		if (play.player_hit[k].size()) {
			// is not a terminal state
			// compare hit vs. stand in action table
			// which is assumed to be precomputed
			const expectations& e(player_actions[ni[k]][j]);
			const edge_type best =
				(e.hit > e.stand) ? e.hit : e.stand;
//			cout << ", best=" << best;
			p.hit += card_odds[k] *best;
		} else {
			// is a terminal state
			const probability_type& edge(pse[j]
				[play_map::player_final_state_map(ni[k])]);
//			cout << ", edge=" << edge;
			p.hit += card_odds[k] *edge;
		}
//		cout << endl;
	}
		// p.optimize(surrender_penalty);
	}
	}
// find optimal non-split decisions first, then back-patch
	optimize_actions();
// create hit-only (non-split, non-double) player state machines
// compute table for calculating spread of player's final state, 
// given dealer's reveal card, and player's optimal hit strategy.
// again hit-stand only
	optimize_player_hit_tables();
	compute_player_hit_edges();
#if DUMP_DURING_EVALUATE
	dump_player_hit_edges(cout) << endl;
#endif
	// OK up to here
	reset_split_edges();	// zero-out
if (var.split) {
	const bool DAS = var.double_after_split && var.some_double();
if (var.resplit) {
//	cout << "Resplitting allowed." <<  endl;
	// need to work backwards from post-split edges
	compute_player_initial_edges(DAS, var.resplit, false);
	compute_player_split_edges(DAS, var.resplit);	// iterate
	compute_player_initial_edges(DAS, var.resplit, false);
	compute_player_split_edges(DAS, var.resplit);	// iterate
	compute_player_initial_edges(var.some_double(), true, var.surrender_late);
} else {
//	cout << "Single split allowed." <<  endl;
	compute_player_initial_edges(DAS, false, false);
	compute_player_split_edges(DAS, false);	// iterate
	compute_player_initial_edges(var.some_double(), true, var.surrender_late);
}
#if DUMP_DURING_EVALUATE
	dump_player_split_edges(cout) << endl;
#endif
} else {
//	cout << "No split allowed." <<  endl;
	// no splitting allowed!
	compute_player_initial_edges(var.some_double(), false, var.surrender_late);
}
	// to account for player's blackjack
#if 0
	finalize_player_initial_edges();
#endif
// next: compute conditional edges given dealer reveal card
#if DUMP_DURING_EVALUATE
	dump_action_expectations(cout);
#endif
}	// end compute_action_expectations()

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Consider splitting not an option.  
	NOTE: this uses post-peek player_hit_edges
 */
void
strategy::reset_split_edges(void) {
	player_initial_edges_vector *player_split_edges =
		&player_initial_edges_post_peek[pair_offset];
	size_t i;
	for (i=0; i<card_values; ++i) {
		player_split_edges[i] =
			player_hit_edges[play.p_initial_card_map[i]];
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Evaluates when player should split pairs.  
	NOTE: this updates the split-expectations of the player_actions table!
	This does not modify the (non-split) player_initial_edges_post_peek tables.
	This should only depend on post-peek conditions, 
		when the action decision is relevant.  
	\pre player_stand_edges_post_peek already computed, 
		player_actions already partially computed.
	\param d double-after-split allowed
	\param s resplit allowed
 */
void
strategy::compute_player_split_edges(const bool d, const bool s) {
	const player_stand_edges_matrix&
		pse(player_stand_edges_post_peek);
	player_initial_edges_vector *player_split_edges =
		&player_initial_edges_post_peek[pair_offset];
	const state_machine& split_table(s ? play.player_resplit : play.last_split);
	size_t i;
	for (i=0; i<card_values; ++i) {
	// the single-card state after split
	size_t j;
	const size_t p = pair_offset +i;
	for (j=0; j<card_values; ++j) {	// for each dealer reveal card
		// need to take weighted sum over initial states
		// depending on initial card and spread of next states.
		edge_type expected_edge = 0.0;
	// convolution: spread over the player's next card
	size_t k;
	for (k=0; k<card_values; ++k) {	// player's second card
		const probability_type& o(card_odds[k]);
		// initial edges may not have double-downs, splits, nor surrenders
		// since player is forced to take the next card in each hand
		const size_t state = split_table[i][k];
		edge_type edge = player_initial_edges_post_peek[state][j];
		// similar to expectation::best(d, i==k, r)
		// already accounted for player_resplit vs. final_split
		if ((i == ACE) && var.one_card_on_split_aces) {
			// each aces takes exactly one more card only
			edge = pse[j][play_map::player_final_state_map(state)];
//			cout << "1-card-ACE[D" << j << ",F" << state << "]: " << edge << endl;
		}
		expected_edge += o * edge;
	}	// inner product
		expectations& sum(player_actions[p][j]);
		sum.split = 2.0 *expected_edge;	// b/c two hands are played
		sum.optimize(var.surrender_penalty);
	}	// end for each dealer reveal card
	}	// end for each player paired card

	// update the split entries for the initial-edges
	for (i=0; i<card_values; ++i) {
	// the single-card state after split
	size_t j;
	const size_t p = pair_offset +i;
	for (j=0; j<card_values; ++j) {	// for each dealer reveal card
		const expectations& sum(player_actions[p][j]);
		// player may decide whether or not to split
		player_split_edges[i][j] =
			sum.value(sum.best(d, s, false), var.surrender_penalty);
	}
	}
#if DUMP_DURING_EVALUATE
	dump_player_split_edges(cout) << endl;
#endif
}	// end compute_player_split_edges()

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
strategy::dump_player_split_edges(ostream& o) const {
	// dump the player's initial state edges 
	const player_initial_edges_vector *player_split_edges =
		&player_initial_edges_post_peek[pair_offset];
	o << "Player's split-permitted edges:" << endl;
	o << "P\\D";
	size_t j;
	for (j=0; j<card_values; ++j) {
		o << '\t' << card_name[j];
	}
	o << endl;
	for (j=0 ; j<card_values; ++j) {
		o << card_name[j] << ',' << card_name[j];
		size_t i;
		for (i=0; i<card_values; ++i) {
			o << '\t' << player_split_edges[j][i];
		}
		o << endl;
	}
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Create player hit state machines.
	Tables can be used to solve for distribution of expected
	player terminal states.  
	Decisions used to compute this table are *only* hit or stand, 
	no double-down or splits.
	Use this table to evaluate expected outcomes.  
	\pre player states are already optimized for stand,hit,double.
 */
void
strategy::optimize_player_hit_tables(void) {
	size_t j;
	for (j=0; j<card_values; ++j) {	// for each reveal card
		player_opt[j] = play.player_hit;	// copy state machine
		// use default assignment operator of util::array
		size_t i;
		for (i=0; i<p_action_states; ++i) {
			const expectations& e(player_actions[i][j]);
			// don't count surrender, split, double
			const player_choice b(e.best(false, false, false));
			// or just compare e.hit vs. e.stand
			if (b == STAND) {
				// is either STAND or DOUBLE
				player_opt[j][i].out_edges.clear();
				// make all stands terminal states
			}
		}
#if 0
		player_opt[j].dump(cout) << endl;
#endif
	}
#if DUMP_DURING_EVALUATE
	dump_player_hit_tables(cout);
#endif
	// now compute player_final_state_probability
	probability_vector unit(p_action_states, 0.0);
	probability_vector result;
	for (j=0; j<card_values; ++j) {
		size_t i;
		for (i=0; i<p_action_states; ++i) {
			unit[i] = 1.0;
			player_opt[j].solve(card_odds, unit, result);
			player_final_state_probabilities(result,
				player_final_state_probability[j][i]);
			unit[i] = 0.0;	// reset
		}
	}
#if DUMP_DURING_EVALUATE
	dump_player_final_state_probabilities(cout);
#endif
}	// end optimize_player_hit_tables()

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
strategy::dump_player_hit_tables(ostream& o) const {
	size_t j;
	for (j=0; j<card_values; ++j) {
		// dump for debugging only
		o << "Dealer shows " << card_name[j] <<
			", player hit state machine:" << endl;
		player_opt[j].dump(o) << endl;
	}
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
strategy::dump_player_final_state_probabilities(ostream& o) const {
	size_t j;
	for (j=0; j<card_values; ++j) {
		o << "Dealer shows " << card_name[j] <<
			", player\'s final state spread (hit/stand only):" << endl;
		o << "player";
		size_t i;
		for (i=0; i<player_states; ++i) {
			o << '\t' << play_map::player_final_states[i];
		}
		o << endl;
		for (i=0; i<p_action_states; ++i) {
			const player_final_state_probability_vector&
				v(player_final_state_probability[j][i]);
			o << player_opt[j][i].name << '\t';
			copy(v.begin(), v.end(),
				ostream_iterator<probability_type>(o, "\t"));
			o << endl;
		}
		o << endl;
	}
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Given player's optimal hit strategies, compute the edges given
	a dealer's reveal card, and the player's current state.  
	Computed by taking inner-product of player's final state spreads
	and the corresponding player-stand-edges.  
	TODO: need pre-peek and post-peek edges
 */
void
strategy::__compute_player_hit_edges(
	const player_stand_edges_matrix& pse, 
	const player_final_states_probability_matrix& fsp,
	const expectations_matrix& actions,
	const edge_type& surr, 
	player_hit_edges_matrix& phe) {
	static const edge_type eps = 
		sqrt(std::numeric_limits<edge_type>::epsilon());
#if 0
	// using post-peek conditions
	const player_stand_edges_matrix&
		pse(player_stand_edges_post_peek);
#endif
	size_t i;
	for (i=0; i<card_values; ++i) {
		const player_stand_edges_vector& edges(pse[i]);
	size_t j;
	for (j=0; j<p_action_states; ++j) {
		// probability-weighted sum of edge values for each state
		const edge_type e = inner_product(edges.begin(), edges.end(), 
			fsp[i][j].begin(), 0.0);
		phe[j][i] = e;
		const expectations& x(actions[j][i]);
		const edge_type sh =
			x.value(x.best(false, false, false), surr);
		const edge_type d = fabs(sh-e);
		// identity: sanity check
		if (d > eps) {
			cout << "p=" << j << ", d=" << i <<
				", sh=" << sh << ", e=" << e <<
				", diff=" << d << endl;
			assert(d <= eps);
		}
	}
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Hit edges should only be post-peek, when the player action
	(decision to hit) is relevant.
 */
void
strategy::compute_player_hit_edges(void) {
#if 1
	// hit-edges are post-peek, b/c/ using post-peek stand edges
	__compute_player_hit_edges(player_stand_edges_post_peek, 
		player_final_state_probability, player_actions, 
		var.surrender_penalty, player_hit_edges);
#else
	__compute_player_hit_edges(player_stand_edges, 
		player_final_state_probability, player_actions, 
		var.surrender_penalty, player_hit_edges);
	__compute_player_hit_edges(player_stand_edges_post_peek, 
		player_final_state_probability, player_actions, 
		var.surrender_penalty, player_hit_edges_post_peek);
#endif
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
strategy::dump_player_hit_edges(ostream& o) const {
	// dump the player's hitting edges 
	o << "Player's hit edges:\n";
	o << "P\\D";
	size_t j;
	for (j=0; j<card_values; ++j) {
		o << '\t' << card_name[j];
	}
	o << endl;
	for (j=0 ; j<p_action_states; ++j) {
		o << play.player_hit[j].name;
		size_t i;
		for (i=0; i<card_values; ++i) {
			o << '\t' << player_hit_edges[j][i];
		}
		o << endl;
	}
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Take the best expectation among player action choice, and report
	as optimal edge, per cell.  
	Of course, not every player state is a valid initial state.
	\param D whether doubles should be considered
	\param S whether splits should be considered
	\param R whether surrender should be considered
	This does not modify the player_actions table.  
	FIXME: distinguish between pre/post peek.
	\pre player actions have already been optimized (always post-peek).  
	\post computes player's initial edges, given dealer's reveal card.
	These initial edges are needed to evaluate post-peek splits, 
		and also overall pre-peek edges.  
 */
void
strategy::compute_player_initial_edges(
		const bool D, const bool S, const bool R) {
//	const player_initial_edges_vector *player_split_edges =
//		&player_initial_edges_post_peek[p_action_states];
	size_t i;
	for (i=0; i<pair_offset; ++i) {		// exclude splits first
	size_t j;
	for (j=0; j<card_values; ++j) {		// for dealer-reveal card
		expectations c(player_actions[i][j]);	// yes, copy
		c.hit = player_hit_edges[i][j];
		c.optimize(var.surrender_penalty);
		// since splits are folded into non-pair states
		const pair<player_choice, player_choice>
//			splits are computed in a separate section of the table
			p(c.best_two(D, S, R));
		const edge_type e = c.value(p.first, var.surrender_penalty);
		player_initial_edges_post_peek[i][j] = e;
	}
	}
#if DUMP_DURING_EVALUATE
	dump_player_initial_edges(cout) << endl;
#endif
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if 0
/**
	After dealer has checked for blackjack, and doesn't have it, 
	if the player has blackjack, she automatically wins +payoff.  
 */
void
strategy::finalize_player_initial_edges(void) {
#if 0
	fill(player_initial_edges[goal].begin(), 
		player_initial_edges[goal].end(), var.bj_payoff);
#endif
#if DUMP_DURING_EVALUATE
	dump_player_initial_edges(cout) << endl;
#endif
}
#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	TODO: pre-peek
 */
ostream&
strategy::dump_player_initial_edges(ostream& o) const {
	// dump the player's initial state edges 
	o << "Player's initial edges (post-peek):" << endl;
	o << "P\\D";
	size_t j;
	for (j=0; j<card_values; ++j) {
		o << '\t' << card_name[j];
	}
	o << endl;
	for (j=0 ; j<p_action_states; ++j) { // print split edges separately
		o << play.player_hit[j].name;
		size_t i;
		for (i=0; i<card_values; ++i) {
			o << '\t' << player_initial_edges_post_peek[j][i];
		}
		o << endl;
	}
#if 0
	dump_player_split_edges(o);
#endif
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Just sorts actions based on expected outcome to optimize player choice.  
 */
void
strategy::optimize_actions(void) {
	size_t i;
	for (i=0; i<p_action_states; ++i) {
	size_t j;
	for (j=0; j<card_values; ++j) {
		player_actions[i][j].optimize(var.surrender_penalty);
	}
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	After having computed initial edges, evaluate the 
	player's expectation, given only the dealer's reveal card.  
	\pre player actions have been optimized, 
		player_initial_edges_post_peek computed, 
		player_initial_edges_pre_peek computed.
	TODO: distinguish between pre-peek and post-peek edges.
 */
void
strategy::compute_reveal_edges(void) {
size_t r;	// index of dealer's reveal card
for (r=0; r<card_values; ++r) {
	edge_type& x(player_edges_given_reveal_post_peek[r]);
	x = 0.0;
	size_t i;	// index of player's initial state
	// inner product
	for (i=0; i<p_action_states; ++i) {
		const probability_type& p(player_initial_state_odds[i]);
	if (p > 0.0) {
		const edge_type& e(player_initial_edges_post_peek[i][r]);
#if 0
		cout << "reveal edge (" << i << "," << r << "): " <<
			e << " @ " << p << endl;
#endif
		x += p *e;
	}
	}
#if 0
	cout << "reveal edge (*," << r << "): " << x << endl;
#endif
}
	// copy first, then correct for ACE, TEN revealed
	player_edges_given_reveal_pre_peek =
		player_edges_given_reveal_post_peek;
#if 1
	// weigh by odds of blackjack
	const probability_type& pt = card_odds[TEN];
	const probability_type& pa = card_odds[ACE];
	const probability_type bjo = pt*pa;
	const probability_type bjc = 1.0 -bjo;
	player_edges_given_reveal_pre_peek[ACE] =
		// dealer has blackjack, account for player blackjack
		-pt * bjc
		// dealer doesn't have blackjack, post-peek component
		+(1.0 -pt)*player_edges_given_reveal_post_peek[ACE];
	player_edges_given_reveal_pre_peek[TEN] =
		// dealer has blackjack, account for player blackjack
		-pa * bjc
		// dealer doesn't have blackjack, post-peek component
		+(1.0 -pa)*player_edges_given_reveal_post_peek[TEN];
#endif
#if DUMP_DURING_EVALUATE
	dump_reveal_edges(cout) << endl;
#endif
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
strategy::dump_reveal_edges(ostream& o) const {
// reorder printing? nah...
	ostream_iterator<edge_type> osi(o, "\t");
{
	o << "Player edges given dealer's revealed card (pre-peek):\n";
	copy(card_name, card_name+TEN+1, ostream_iterator<char>(o, "\t"));
	o << endl;
	copy(player_edges_given_reveal_pre_peek.begin(),
		player_edges_given_reveal_pre_peek.end(), osi);
	o << endl;
}{
	o << "Player edges given dealer's revealed card (post-peek):\n";
	copy(card_name, card_name+TEN+1, ostream_iterator<char>(o, "\t"));
	o << endl;
	copy(player_edges_given_reveal_post_peek.begin(),
		player_edges_given_reveal_post_peek.end(), osi);
	o << endl;
}
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	The final step!
	Gets complicated when dealing with all the different variations...
	Calculation should be based on game tree:
	Cards dealt, dealer-shows-ace?
	A) yes. offer insurance as a side-bet (ignored here)
		i) has blackjack.  player-has-blackjack?
			a) yes: push
			b) no: player loses
		ii) no blackjack, player continues, with the knowledge
			that dealer's hole-card is not T (10).
	B) dealer-shows-10, no insurance side-bet
		i) has blackjack.  player-has-blackjack?
			a) yes: push
			b) no: player loses
		ii) no blackjack, player continues with the knowledge that
			dealer's hole-card is not A.
	C) no. player-has-blackjack?
		i) yes.  player wins, blackjack payoff.
		ii) no.  normal play
 */
void
strategy::compute_overall_edge(void) {
	_overall_edge =
		inner_product(card_odds.begin(), card_odds.end(), 
			player_edges_given_reveal_pre_peek.begin(), 0.0);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Prints using the designated reveal_print_ordering for columns.
	Is a member function to access surrender_penalty.
 */
ostream&
strategy::dump_expectations(const expectations_vector& v, ostream& o) const {
	dump_optimal_actions(v, o, 3, "\t");
	const expectations_vector::const_iterator b(v.begin()), e(v.end());
	expectations z(0,0,0,0);
	z = accumulate(b, e, z);
	size_t j;
#if 1
#define	EFORMAT(x)	setw(5) << int(x*1000)
// setprecision?
#else
#define	EFORMAT(x)	x
#endif
	o << "stand";
	for (j=0; j<card_values; ++j) {
		const expectations& ex(v[reveal_print_ordering[j]]);
		o << '\t' << EFORMAT(ex.stand);
		if (ex.best() == STAND) o << '*';
	}
	o << endl;
if (z.hit > -1.0 * card_values) {
	o << "hit";
	for (j=0; j<card_values; ++j) {
		const expectations& ex(v[reveal_print_ordering[j]]);
		o << '\t' << EFORMAT(ex.hit);
		if (ex.best() == HIT) o << '*';
	}
	o << endl;
}
if (z.double_down > -2.0 *card_values) {
	// TODO: pass double_multiplier?
	o << "double";
	for (j=0; j<card_values; ++j) {
		const expectations& ex(v[reveal_print_ordering[j]]);
		o << '\t' << EFORMAT(ex.double_down);
		if (ex.best() == DOUBLE) o << '*';
	}
	o << endl;
}
// for brevity, could omit non-splittable states...
if (z.split > -2.0 *card_values) {
	o << "split";
	for (j=0; j<card_values; ++j) {
		const expectations& ex(v[reveal_print_ordering[j]]);
		o << '\t' << EFORMAT(ex.split);
		if (ex.best() == SPLIT) o << '*';
	}
	o << endl;
}
#if 1
	o << "surr.";
	for (j=0; j<card_values; ++j) {
		const expectations& ex(v[reveal_print_ordering[j]]);
//		o << '\t' << EFORMAT(expectations::surrender);
		o << '\t' << EFORMAT(var.surrender_penalty);
		if (ex.best() == SURRENDER) o << '*';
	}
	o << endl;
#endif
#if 1
	o << "delta";
	for (j=0; j<card_values; ++j) {
		const expectations& ex(v[reveal_print_ordering[j]]);
		o << '\t';
		const pair<player_choice, player_choice>
			opt(ex.best_two(true, true, true));
		o << EFORMAT(ex.margin(opt.first, opt.second, var.surrender_penalty));
	}
	o << endl;
#endif

#undef	EFORMAT
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Print easier-to-use summary of optimal decision table.  
	\param n the top number of actions to print.
 */
ostream&
strategy::dump_optimal_actions(const expectations_vector& v, ostream& o, 
		const size_t n, const char* delim) const {
	assert(n<=5);
	size_t j;
	for (j=0; j<card_values; ++j) {
		const expectations& ex(v[reveal_print_ordering[j]]);
		o << delim;
		size_t k = 0;
		for ( ; k<n; ++k) {
			o << action_key[ex.actions[k]];
		}
	}
	o << endl;
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
strategy::dump_optimal_edges(const expectations_vector& v, ostream& o) const {
	dump_optimal_actions(v, o, 3, "\t");
#if 1
#define	EFORMAT(x)	setw(5) << int(x*1000)
// setprecision?
#else
#define	EFORMAT(x)	x
#endif
	size_t j;
	o << "edge";
	for (j=0; j<card_values; ++j) {
		const expectations& ex(v[reveal_print_ordering[j]]);
		o << '\t';
		switch (ex.best()) {
		case STAND: o << EFORMAT(ex.stand); break;
		case HIT: o << EFORMAT(ex.hit); break;
		case DOUBLE: o << EFORMAT(ex.double_down); break;
		case SPLIT: o << EFORMAT(ex.split); break;
		case SURRENDER: o << EFORMAT(var.surrender_penalty); break;
		default: break;
		}
	}
	o << endl;
#if 1
	o << "delta";	// difference between best and next best
	for (j=0; j<card_values; ++j) {
		const expectations& ex(v[reveal_print_ordering[j]]);
		o << '\t';
		const pair<player_choice, player_choice>
			opt(ex.best_two(true, true, true));
		o << EFORMAT(ex.margin(opt.first, opt.second, var.surrender_penalty));
	}
	o << endl;
#endif
#undef	EFORMAT
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const edge_type&
strategy::expectations::value(const player_choice c, 
		const edge_type& surrender) const {
	switch (c) {
	case STAND:	return stand;
	case HIT:	return hit;
	case DOUBLE:	return double_down;
	case SPLIT:	return split;
	case SURRENDER:	return surrender;
	default:	cerr << "Invalid player choice." << endl; assert(0);
	}
	return stand;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Assumes all choices (double, split, surrender) available.
	It is up to the best() method to filter out invalid choices.  
 */
void
strategy::expectations::optimize(const edge_type& surrender) {
	typedef	std::multimap<edge_type, player_choice>	sort_type;
	sort_type s;
	s.insert(make_pair(stand, STAND));
	s.insert(make_pair(hit, HIT));
	s.insert(make_pair(double_down, DOUBLE));
	s.insert(make_pair(split, SPLIT));
	s.insert(make_pair(surrender, SURRENDER));
	sort_type::const_iterator i(s.begin()), e(s.end());
	// sort by order of preference
	++i;	actions[3] = i->second;
	++i;	actions[2] = i->second;
	++i;	actions[1] = i->second;
	++i;	actions[0] = i->second;	// best choice
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\return the best two player choices (first better than second), 
	given constraints:
	\param d whether double-down is a valid option
	\param s whether split is a valid option
	\param r whether surrender is a valid option
	Invalid options are skipped in the response.  
 */
pair<player_choice, player_choice>
strategy::expectations::best_two(
	const bool d, const bool s, const bool r) const {
	player_choice ret[2];
	const player_choice* i(&actions[0]), *e(&actions[4]);
	player_choice* p = ret;
	player_choice* const pe = &ret[2];
	for ( ; i!=e && p!=pe; ++i) {
	switch (*i) {
	case STAND:
	case HIT:
		// hit and stand are always allowed
		*p = *i;
		++p;
		break;
	case DOUBLE:
		if (d) {
			*p = *i;
			++p;
		}
		break;
	case SPLIT:
		if (s) {
			*p = *i;
			++p;
		}
		break;
	case SURRENDER:
		if (r) {
			*p = *i;
			++p;
		}
		break;
	default: break;
	}
	}
	if (p != pe) {
	if (ret[0] == HIT) {
		ret[1] = STAND;
	} else if (p[1] == STAND) {
		ret[1] = HIT;
	}
	// else ???
	}
	return pair<player_choice, player_choice>(ret[0], ret[1]);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
strategy::dump_action_expectations(ostream& o) const {
	o << "Player's action expectations (edges x1000):\n";
	const expectations_matrix::const_iterator
		b(player_actions.begin()), e(player_actions.end());
	expectations_matrix::const_iterator i(b);
	// ostream_iterator<expectations> osi(o, "\t");
	o << "P\\D";
	size_t j;
	for (j=0; j<card_values; ++j) {
		o << '\t' << card_name[reveal_print_ordering[j]];
	}
	o << '\n' << endl;
	o << setprecision(3);
	for ( ; i!=e; ++i) {
		o << play.player_hit[i-b].name;
//		o << endl;
		dump_expectations(*i, o);
		o << endl;
	}
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
strategy::dump_optimal_actions(ostream& o) const {
	o << "Player's optimal action:\n";
	const expectations_matrix::const_iterator
		b(player_actions.begin()), e(player_actions.end());
	expectations_matrix::const_iterator i(b);
	// ostream_iterator<expectations> osi(o, "\t");
	static const char delim1[] = "   ";
	static const char delim2[] = "  ";
	o << "P\\D\t";
	size_t j;
	for (j=0; j<card_values; ++j) {
		o << delim1 << card_name[reveal_print_ordering[j]];
	}
	o << '\n' << endl;
	o << setprecision(3);
	for ( ; i!=e; ++i) {
		o << play.player_hit[i-b].name << "\t";
//		o << endl;
//		dump_optimal_edges(*i, o);	// still too much detail
		dump_optimal_actions(*i, o, 2, delim2);
	}
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
strategy::dump(ostream& o) const {
	var.dump(o) << endl;
	play.dump_dealer_policy(o);				// verified
	play.dump_player_hit_state(o);			// verified
	dump_dealer_final_table(o) << endl;		// verified
	dump_player_stand_odds(o) << endl;		// verified

	dump_action_expectations(o);
	dump_reveal_edges(o) << endl;
	const edge_type e(overall_edge());
	o << "Player\'s overall edge = " << e <<
		" (" << e*100.0 << "%)" << endl;
	return o;
}

//=============================================================================
// class deck_state method definitions

/**
	Initializes to default.
 */
deck_state::deck_state(const variation& v) : 
		num_decks(v.num_decks), 
		card_probabilities(card_values), 
		need_update(true) {
	reshuffle();		// does most of the initializing
	// default penetration before reshuffling: 75%
	maximum_penetration = (1.0 -v.maximum_penetration) * num_decks *52;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
deck_state::reshuffle(void) {
	// ACE is at index 0
	cards_remaining = num_decks*52;
	cards_spent = 0;
	const size_t f = num_decks*4;
	std::fill(used_cards.begin(), used_cards.end(), 0);
	std::fill(cards.begin(), cards.end() -1, f);
	cards[TEN] = f*4;	// T, J, Q, K
	need_update = true;
	update_probabilities();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\return true if deck/shoe was shuffled.
 */
bool
deck_state::reshuffle_auto(void) {
	if (cards_remaining < maximum_penetration) {
		reshuffle();
		return true;
	}
	return false;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
deck_state::update_probabilities(void) {
	if (need_update) {
		assert(card_probabilities.size() == cards.size());
		normalize(card_probabilities, cards);
		need_update = false;
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if 0
/**
	\param d the card index of the counter to decrement.
 */
void
deck_state::count(const size_t c) {
	assert(cards[c]);
	--cards[c];
}
#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Draws the user-determined card, useful for setting up
	and querying specific scenarios.
	Counts specified card as drawn from deck.
 */
void
deck_state::magic_draw(const size_t r) {
	// TODO: safe-guard against drawing non-existent
	assert(cards[r]);
	++used_cards[r];
	--cards[r];
	++cards_spent;
	--cards_remaining;
	need_update = true;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Removes one card from remaining deck at random.
	Does NOT re-compute probabilities.
 */
size_t
deck_state::quick_draw(void) {
	const size_t ret = quick_draw_uncounted();
	magic_draw(ret);
	return ret;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Peeks at one card from remaining deck at random.
	Does NOT re-compute probabilities.
 */
size_t
deck_state::quick_draw_uncounted(void) {
#if 0
	return util::random_draw_from_real_pdf(card_probabilities);
#else
	return util::random_draw_from_integer_pdf(cards);
#endif
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Gives user to choose her own card.
	Useful for specific scenario testing.
	\param m if true, prompt user to choose a specific card.
 */
size_t
deck_state::option_draw_uncounted(const bool m, istream& i, ostream& o) {
if (m) {
	string line;
	do {
		o << "card? ";
		i >> line;
	} while (line.empty() && i);
	const size_t c = card_index(line[0]);
	if (c != size_t(-1)) {
		return c;
	} else {
		o << "drawing randomly." << endl;
		return quick_draw_uncounted();
	}
} else {
	return quick_draw_uncounted();
}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t
deck_state::option_draw(const bool m, istream& i, ostream& o) {
	const size_t ret = option_draw_uncounted(m, i, o);
	magic_draw(ret);
	return ret;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Removes one card from remaining deck at random.
 */
size_t
deck_state::draw(void) {
	const size_t ret = quick_draw();
	update_probabilities();
	return ret;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
deck_state::draw_hole_card(void) {
	hole_card = quick_draw_uncounted();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
deck_state::option_draw_hole_card(const bool m, istream& i, ostream& o) {
	hole_card = option_draw_uncounted(m, i, o);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	This should only be called once per hole card draw.
 */
size_t
deck_state::reveal_hole_card(void) {
	magic_draw(hole_card);
//	update_probabilities();
	return hole_card;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
deck_state::show_count(ostream& o) const {
	const size_t p = o.precision(2);
	size_t i = 0;
	o << "card:\t";
	for (i=0 ; i<card_values; ++i) {
		const size_t j = reveal_print_ordering[i];
		o << "   " << card_name[j];
	}
	o << "\ttotal\t%" << endl;
	o << "used:\t";
	for (i=0 ; i<card_values; ++i) {
		const size_t j = reveal_print_ordering[i];
		o << setw(4) << used_cards[j];
	}
	o << "\t" << cards_spent << "\t(" <<
		double(cards_spent) *100.0 / (num_decks *52) << "%)\n";
	o << "rem:\t";
	for (i=0; i<card_values; ++i) {
		const size_t j = reveal_print_ordering[i];
		o << setw(4) << cards[j];
	}
	o << "\t" << cards_remaining << "\t(" <<
		double(cards_remaining) *100.0 / (num_decks *52) << "%)\n";
	// hi-lo count summary, details of true count, running count
	int hi_lo_count = 0;
	hi_lo_count += used_cards[1] +used_cards[2] +used_cards[3]
		+used_cards[4] +used_cards[5];	// 2 through 6
	hi_lo_count -= used_cards[ACE] +used_cards[TEN];
	double true_count = (double(hi_lo_count) * 52)
		/ double(cards_remaining);
	o << "hi-lo: " << hi_lo_count <<
	// true count adjusts for number of cards remaining
	// normalized to 1 deck
		", true-count: " << true_count;
	// adjusted count accounts for the house edge based on rules
	o.precision(p);
	return o << endl;
}

//=============================================================================
// class grader method definitions

grader::grader(const variation& v) :
		var(v), play(v), basic_strategy(play), dynamic_strategy(play), 
		C(v),
	player_hands(), 
	pick_cards(false),
	bankroll(100.0), bet(1.0) {
	basic_strategy.set_card_distribution(standard_deck_distribution);
	dynamic_strategy.set_card_distribution(standard_deck_distribution);
	basic_strategy.evaluate();
	dynamic_strategy.evaluate();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
grader::~grader() { }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool
grader::offer_insurance(istream& i, ostream& o, const bool pbj) const {
	// if peek_ACE
	const char* prompt = pbj ?  "even-money?" : "insurance?";
	bool done = false;
	bool buy_insurance = false;
	string line;
	do {
	do {
		o << prompt << " [ync]: ";
		i >> line;
	} while (line.empty() && i);
	if (line == "n" || line == "N") {
		done = true;
	} else if (line == "y" || line == "Y") {
		buy_insurance = true;
		done = true;
	} else if (line == "c" || line == "C") {
		C.show_count(o);
	}
	} while (!done);
	return buy_insurance;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Play a hand of blackjack.
 */
void
grader::deal_hand(istream& i, ostream& o) {
	player_hands.clear();
	player_hands.resize(1);
	if (pick_cards) {
		o << "choose player cards." << endl;
	}
	const size_t p1 = C.option_draw(pick_cards, i, o);
	const size_t p2 = C.option_draw(pick_cards, i, o);
	player_hands.front().deal(play, p1, p2);
	if (pick_cards) {
		o << "choose dealer up-card." << endl;
	}
	dealer_reveal = C.option_draw(pick_cards, i, o);
	if (pick_cards) {
		o << "choose dealer hole-card." << endl;
	}
	C.option_draw_hole_card(pick_cards, i, o);
	o << "dealer: " << card_name[dealer_reveal] << endl;
	const state_machine& ps(play.get_player_state_machine());
	// TODO: if early_surrender (rare) ...
	// prompt for insurance
	const bool pbj = player_hands.front().has_blackjack();
	if (pbj) {
		o << "Player has blackjack!" << endl;
	}
	bool end = pbj;
	if (dealer_reveal == ACE) {
		player_hands.front().dump(o, ps) << endl;
		if (var.peek_on_Ace) {
		const bool buy_insurance = offer_insurance(i, o, pbj);;
		// determine change in bankroll
		// check for blackjack for player
		const double half_bet = bet / 2.0;
		if (C.peek_hole_card() == TEN) {
			end = true;
			o << "Dealer has blackjack." << endl;
			if (buy_insurance) {
				bankroll += var.insurance *half_bet;
			}
			if (!pbj) {
				bankroll -= bet;
			} else {
				o << "Push." << endl;
			}
		} else {
//			o << "No dealer blackjack." << endl;
			if (buy_insurance) {
				bankroll -= half_bet;
			}
			if (pbj) {
				bankroll += var.bj_payoff *bet;
			}
			// else keep playing
		}
		}	// peek_on_Ace
	} else if (dealer_reveal == TEN) {
		player_hands.front().dump(o, ps) << endl;
		if (var.peek_on_10) {
		if (C.peek_hole_card() == ACE) {
			end = true;
			o << "Dealer has blackjack." << endl;
			if (!pbj) {
				bankroll -= bet;
			} else {
				o << "Push." << endl;
			}
		} else {
//			o << "No dealer blackjack." << endl;
		}
		}	// peek_on_10
	}
	// play ends if either dealer or player had blackjack (and was peeked)
if (!end) {
	size_t j = 0;
	for ( ; j<player_hands.size(); ++j) {
		hand& ph(player_hands[j]);
		o << '[' << j+1 << '/' << player_hands.size() << "] ";
		o << "dealer: " << card_name[dealer_reveal] << ", ";
		ph.dump(o, ps) << endl;
		// prompt for player action
		// o << "TODO: finish me" << endl;
		// player may resplit hands
		bool live = true;
		do {
			// these predicates can be further refined by
			// variation/rules etc...
			const bool d = ph.doubleable();
			// check for double_after_split and other limitations
			const bool p = ph.splittable();
			// check for resplit limit
			const bool r = ph.surrenderable() && !already_split();
			// only first hand is surrenderrable, normally
			bool prompt = false;
			do {
			const player_choice pc =
				prompt_player_action(i, o, d, p, r);
			switch (pc) {
			case STAND: live = false; break;
			case DOUBLE:
				live = false;
				ph.doubled_down = true;
				// fall-through
			case HIT:
				ph.hit(play.player_hit,
					C.option_draw(pick_cards, i, o));
				break;
			case SPLIT:
				ph.presplit(play);
				player_hands.push_back(ph);
				ph.hit(play.player_hit,
					C.option_draw(pick_cards, i, o));
				// check for special case of split-Aces
				break;
			case SURRENDER: bankroll -= bet *var.surrender_penalty;
				live = false; break;
			case HINT:
			case OPTIM:
				prompt = true;
				o << "Hints not yet available." << endl;
				break;
			default:
				prompt = true;
				break;
			}
			} while (prompt);
			if (ph.state == player_bust) {
				o << "Player busts." << endl;
				live = false;
			}
		} while (live);
	}
	// dealer plays after player is done
	// should dealer play when there are no live hands?
	
	
	// suspense double-down?
}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
player_choice
prompt_player_action(istream& i, ostream& o, 
		const bool d, const bool p, const bool r) {
	player_choice c = NIL;
do {
	string line;
	do {
		// prompt for legal choices
		o << "action? [hs";
		if (d) o << 'd';
		if (p) o << 'p';
		if (r) o << 'r';
		o << "?!] ";
		i >> line;
	} while (line.empty() && i);
	switch (line[0]) {
	case 'h':
	case 'H':
		c = HIT; break;
	case 'd':
	case 'D':
		c = DOUBLE; break;
	case 'p':
	case 'P':
		c = SPLIT; break;
	case 's':
	case 'S':
		c = STAND; break;
	case 'r':
	case 'R':
		c = SURRENDER; break;
	case '?':
		c = HINT; break;
		// caller should provide detailed hint with edges
		// for both basic and dynamic strategies
	case '!':
		c = OPTIM; break;
	default: break;
	}
} while (c == NIL);
	return c;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
grader::status(ostream& o) const {
	o << "bankroll: " << bankroll << endl;
	o << "bet: " << bet << endl;
	// deck remaning (%)
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
grader::table_help(ostream& o) {
	return o <<
"At the BlackJack table, the following commands are available:\n"
"?,help -- print this help\n"
"r,rules -- print the rule variation for BlackJack\n"
"b,bet -- change bet amount\n"
"d,deal -- deal a hand of BlackJack\n"
"status -- print current bankroll and bet\n"
"basic-strategy -- print optimal decisions for basic strategy\n"
"  -edge for player edge details\n"
"  -verbose for derivation details\n"
"dynamic-strategy -- print optimal decisions for dynamic strategy\n"
"  -edge for player edge details\n"
"  -verbose for derivation details\n"
// analyze count-dependent strategy
// modify deck (for analysis)
"mode -- change in-game options\n"
"c,count -- show current card count\n"
"q,quit,leave,exit,bye -- leave the table, return to lobby" << endl;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\param i the input stream, like cin
 */
void
grader::play_hand(istream& i, ostream& o) {
	o << "You have sat down at a BlackJack table." << endl;
	o << "Type 'help' for a list of table commands." << endl;
	// prompting loop
do {
	string line;
	do {
		o << "table> ";
		i >> line;
	} while (line.empty() && i);
	// switch-case on
	if (line == "?" || line == "help") {
		table_help(o);
	} else if (line == "d" || line == "deal") {
		// deal a hand
		deal_hand(i, o);
		C.update_probabilities();
		dynamic_strategy.set_card_distribution(C.get_card_probabilities());
	} else if (line == "b" || line == "bet") {
		// TODO: enforce table minimums
		o << "bet amount? ";
		string bamt;
		i >> bamt;
		if (bamt.length()) {
			istringstream iss(bamt);
			size_t a;
			iss >> a;
			if (!iss.fail()) {
			// enforce table minimum
			if (a > 0) {
				bet = double(a);
			} else {
				o << "Bet must be positive!" << endl;
			}
			} else {
				o << "Error reading bet amount." << endl;
			}
		}
		// else leave bet unchanged
		o << "bet: " << bet << endl;
	} else if (line == "r" || line == "rules") {
		basic_strategy.dump_variation(o);
	} else if (line == "c" || line == "count") {
		C.show_count(o);
	} else if (line == "status") {
		status(o);
	} else if (line == "cards-random") {
		pick_cards = false;
	} else if (line == "cards-pick") {
		pick_cards = true;
	} else if (line == "basic-strategy-verbose") {
		basic_strategy.dump(o);
	} else if (line == "basic-strategy-edges") {
		basic_strategy.dump_action_expectations(o);
	} else if (line == "basic-strategy") {
		basic_strategy.dump_optimal_actions(o);
	} else if (line == "dynamic-strategy-verbose") {
		dynamic_strategy.dump(o);
	} else if (line == "dynamic-strategy-edges") {
		dynamic_strategy.dump_action_expectations(o);
	} else if (line == "dynamic-strategy") {
		dynamic_strategy.dump_optimal_actions(o);
	} else if (line == "q" || line == "quit" || line == "exit" ||
			line == "bye" || line == "leave") {
		if (bankroll > 0.0) {
		o << "You collect your remaining chips from the table ("
			<< bankroll << ") and return to the lobby." << endl;
		} else {
			o << "Better luck next time!" << endl;
		}
		break;
	} else {
		o << "Unknown command: \"" << line <<
			"\".\nType 'help' for a list of commands." << endl;
	}
} while (i);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
grader::hand::hand(const play_map& play, const size_t p1) : cards(), state() {
	initial_card(play, p1);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
grader::hand::initial_card(const play_map& play, const size_t p1) {
	// the string stores the card names, not card indices
	cards.push_back(card_name[p1]);
	state = play.p_initial_card_map[p1];
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
grader::hand::hit(const state_machine& m, const size_t p2) {
	cards.push_back(card_name[p2]);
	const state_machine::node& current(m[state]);
	if (!current.is_terminal()) {
		state = current[p2];
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Assigns initial state based on player's first two cards.
 */
void
grader::hand::deal(const play_map& play, const size_t p1, const size_t p2) {
	initial_card(play, p1);
	hit(play.get_player_state_machine(), p2);
	// if initial total is 21, natural blackjack
	if (state == goal) {
		state = player_blackjack;
	}
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool
grader::hand::splittable(void) const {
	return (cards.size() == 2) && (cards[0] == cards[1]);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Split back to single-card.
	Does not hit.
 */
void
grader::hand::presplit(const play_map& play) {
	const size_t p1 = card_index(cards[0]);
	cards.clear();
	initial_card(play, p1);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\param p2 new up-card for player.
	TODO: check for one-card on split-aces
 */
void
grader::hand::split(const play_map& play, const size_t p2) {
	if (splittable()) {
		presplit(play);
		hit(play.get_player_state_machine(), p2);
		// 21 here does not count as blackjack
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
grader::hand::dump(ostream& o, const state_machine& m) const {
	o << "player: " << cards << " (" << m[state].name << ")";
	return o;
}

//=============================================================================
}	// end namespace blackjack

