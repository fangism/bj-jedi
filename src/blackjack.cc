// "card_state.cc"

#include "blackjack.hh"
#include "util/array.tcc"
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

//=============================================================================
// Blackjack specific routines

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/// mapping of initial card (dealer or player) to state indices
const size_t
strategy::vals;

const size_t
strategy::initial_card_states[strategy::vals] =
	{ 23, 2, 3, 4, 5, 6, 7, 8, 9, 10};

const size_t
strategy::cols;

const size_t
strategy::phit_states;

const char
strategy::action_key[] = "-SHDPR";
// - = nil (invalid)
// S = stand
// H = hit
// D = double
// P = split pair
// R = surrender (run!)

const double
strategy::expectations::surrender = -0.5;

const char strategy::card_name[] = "A23456789T";

const char strategy::player_final_states[][cols +1] = {
	"<=16",
	"17",
	"18",
	"19",
	"20",
	"21",
	"bust"
};

static const probability_type __standard_deck[strategy::vals] = {
1.0/13.0,		// A
1.0/13.0,		// 2
1.0/13.0,		// 3
1.0/13.0,		// 4
1.0/13.0,		// 5
1.0/13.0,		// 6
1.0/13.0,		// 7
1.0/13.0,		// 8
1.0/13.0,		// 9
4.0/13.0		// 10,J,Q,K
};

const deck strategy::standard_deck_odds(__standard_deck, __standard_deck+vals);

//-----------------------------------------------------------------------------
// class variation method definitions

static
const char*
yn(const bool y) {
	return y ? "yes" : "no";
}

ostream&
variation::dump(ostream& o) const {
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
	o << "blackjack pays: " << bj_payoff << endl;
	o << "insurance pays: " << insurance << endl;
	o << "surrender penalty: " << surrender_penalty << endl;
	o << "double-down multiplier: " << double_multiplier << endl;
	return o;
}

//-----------------------------------------------------------------------------
// class strategy method definitions

void
strategy::set_card_distribution(const deck& o) {
	card_odds = o;
	update_player_initial_state_odds();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Make the sums add up to 1.0.
 */
template <class A>
static
void
normalize(A& a) {
	typedef	typename A::value_type	value_type;
	const value_type sum = accumulate(a.begin(), a.end(), 0.0);
	transform(a.begin(), a.end(), a.begin(), 
		std::bind2nd(std::divides<value_type>(), sum));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Include the player's initial "blackjack" state because
	the playoff is accounted for in player_initial_edges.
	NOT anymore.
 */
void
strategy::update_player_initial_state_odds(void) {
#if 0
	deck cards_no_ten(card_odds);
	deck cards_no_ace(card_odds);
	cards_no_ten[TEN] = 0.0;
	cards_no_ace[ACE] = 0.0;
	normalize(cards_no_ten);
	normalize(cards_no_ace);
#endif
	size_t i;
	fill(player_initial_state_odds.begin(),
		player_initial_state_odds.end(), 0.0);
	// Q: is this the correct way to normalize?
for (i=0; i<vals; ++i) {
	const probability_type& f(card_odds[i]); // / not_bj_odds;
	// scale up, to normalize for lack of blackjack
#if 0
	const deck* C;
	if (i == TEN) {
		C = &cards_no_ace;
	} else if (i == ACE) {
		C = &cards_no_ten;
	} else {
		C = &card_odds;
	}
	C = &card_odds;
#endif
	size_t j;
	for (j=0; j<vals; ++j) {
		const probability_type fs = f * card_odds[j];
		if (i == j) {		// splittable pairs
			player_initial_state_odds[phit_states +i] += fs;
		} else {
			const size_t k = player_hit[initial_card_states[i]][j];
			player_initial_state_odds[k] += fs;
		}
	}
}
	// remove the case for when player has blackjack, handled separately
	player_initial_state_odds[goal] = 0.0;
	normalize(player_initial_state_odds);
#if DUMP_DURING_EVALUATE
	dump_player_initial_state_odds(cout) << endl;
#endif
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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
	Maps player states to compact representation of final states.  
	\return an index [0,cols] into an array of size cols +1.
 */
size_t
strategy::player_final_state_map(const size_t i) {
	if (i < stop) {
		return 0;	// represents <= 16
	} else if (i <= goal) {
		return i -stop +1;	// represents 17..21
	} else if (i == bust) {
		return cols;
	} else {		// is Ace state, take the higher value
		return player_final_state_map(i -bust +vals);
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
	assert(s.size() == phit_states);
	fill(r.begin(), r.end(), 0.0);
	size_t i;
	for (i=0; i<phit_states; ++i) {
		r[player_final_state_map(i)] += s[i];
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
strategy::strategy(const variation& v) : 
	variation(v), card_odds(standard_deck), dealer() {
	// only depends on H17:
	// don't *have* to do this right away...
	set_dealer_policy();
	compute_player_hit_state();
	update_player_initial_state_odds();
#if DUMP_DURING_EVALUATE
	dump_dealer_policy(cout);
	dump_player_hit_state(cout);
#endif
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Build the dealer's state machine, independent of probability.
	\param H17 If H17, then dealer must hit on a soft 17, else must stand.  
 */
void
strategy::set_dealer_policy(void) {
	// enumerating edges:
	const size_t soft_max = H17 ? 7 : 6;	// 6-16, 7-17
	// 0-21 are hard values (no ace)
	// 22-28 are soft 11 through soft 17
	// 29 is bust
	dealer.resize(bust +soft_max +1);
	// first handle hard values (no ACE)
	size_t i, c;
	for (i=0; i<stop; ++i) {	// hit through hard 16
		ostringstream oss;
		oss << i;
		dealer.name_state(i, oss.str());
	for (c=1; c<vals; ++c) {
		const size_t t = i +c +1;	// sum value
		if (t > goal) {			// bust
			dealer.add_edge(i, bust, c, vals);
		} else {
			dealer.add_edge(i, t, c, vals);
		}
	}
		// for c=0 (ace)
		const size_t t = i+1;		// A = 1
		const size_t tA = t+vals;	// A = 11
		if (t >= soft_min && t <= soft_max) {
			dealer.add_edge(i, t +soft -soft_min, 0, vals);
		} else if (tA <= goal) {
			dealer.add_edge(i, tA, 0, vals);
		} else if (t <= goal) {
			dealer.add_edge(i, t, 0, vals);
		} else {
			dealer.add_edge(i, bust, 0, vals);
		}
	}
	for ( ; i<=goal; ++i) {
		ostringstream oss;
		oss << i;
		dealer.name_state(i, oss.str());
	}
	// soft-values
	for (i=soft_min; i<=soft_max; ++i) {
		ostringstream oss;
		oss << "A";
		if (i>soft_min) { oss << "," << i-1; }
		dealer.name_state(i+bust, oss.str());
	for (c=0; c<vals; ++c) {		// A = 1 only
		const size_t t = i+c;
		const size_t tA = t +vals +1;
		if (tA > goal) {
			dealer.add_edge(i+bust, t +1, c, vals);
		} else if (tA > soft_max +vals) {
			dealer.add_edge(i+bust, tA, c, vals);
		} else {
			dealer.add_edge(i+bust, t +soft, c, vals);
		}
	}
	}
	dealer.name_state(bust, "bust");
	// the bust-state is terminal
	dealer.check();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
strategy::dump_dealer_policy(ostream& o) const {
	o << "Dealer's action table:\n";
	return dealer.dump(o) << endl;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Compute player hit states.  
	This is just an assessment of state transitions on hitting, 
	not any indication of whether hitting is a advisiable.  :)
 */
void
strategy::compute_player_hit_state(void) {
	const size_t soft_max = 10;
	// 0-21 are hard values (no ace)
	// 22-30 are soft 12 through soft 20
	// 31 is bust
	player_hit.resize(phit_states);
	// first handle hard values (no ACE)
	size_t i, c;
	for (i=0; i<goal; ++i) {	// hit through hard 16
		ostringstream oss;
		oss << i;
		player_hit.name_state(i, oss.str());
	for (c=1; c<vals; ++c) {
		const size_t t = i +c +1;	// sum value
		if (t > goal) {			// bust
			player_hit.add_edge(i, bust, c, vals);
		} else {
			player_hit.add_edge(i, t, c, vals);
		}
	}
		// for c=0 (ace)
		const size_t t = i+1;		// A = 1
		const size_t tA = t+vals;	// A = 11
		if (t >= soft_min && t <= soft_max) {
			player_hit.add_edge(i, t +soft -soft_min, 0, vals);
		} else if (tA <= goal) {
			player_hit.add_edge(i, tA, 0, vals);
		} else if (t <= goal) {
			player_hit.add_edge(i, t, 0, vals);
		} else {
			player_hit.add_edge(i, bust, 0, vals);
		}
	}
	for ( ; i<=goal; ++i) {
		ostringstream oss;
		oss << i;
		player_hit.name_state(i, oss.str());
	}
	// soft-values
	for (i=soft_min; i<=soft_max; ++i) {
		ostringstream oss;
		oss << "A";
		if (i>soft_min) { oss << "," << i-1; }
		player_hit.name_state(i+bust, oss.str());
	for (c=0; c<vals; ++c) {		// A = 1 only
		const size_t t = i+c;
		const size_t tA = t +vals +1;
		if (tA > goal) {
			player_hit.add_edge(i+bust, t +1, c, vals);
		} else if (tA > soft_max +vals) {
			player_hit.add_edge(i+bust, tA, c, vals);
		} else {
			player_hit.add_edge(i+bust, t +soft, c, vals);
		}
	}
	}
	player_hit.name_state(bust, "bust");
	// the bust-state is terminal
	player_hit.check();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
strategy::dump_player_hit_state(ostream& o) const {
	o << "Player's hit transition table:\n";
	return player_hit.dump(o) << endl;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
strategy::compute_dealer_final_table(void) {
	// 17, 18, 19, 20, 21, bust
	deck cards_no_ten(card_odds);
	deck cards_no_ace(card_odds);
	cards_no_ten[TEN] = 0.0;
	cards_no_ace[ACE] = 0.0;
	normalize(cards_no_ten);
	normalize(cards_no_ace);
	// use cards_no_ten for the case where dealer does NOT have blackjack
	// use cards_no_ace when dealer draws 10, and is not blackjack
	//	this is called 'peek'
	size_t j;
	for (j=0 ; j<vals; ++j) {
		probability_vector init(dealer.get_states().size(), 0.0);
		probability_vector final(init.size());
		init[initial_card_states[j]] = 1.0;
		// for the first iteration
		// isolate the case when dealer shows an ACE or TEN
		// but hole card is known to NOT give dealer blackjack!
		if (j == ACE) {
			final = init;
			dealer.convolve(cards_no_ten, final, init);
			// then solve the rest
		} else if (j == TEN) {
			final = init;
			dealer.convolve(cards_no_ace, final, init);
		}
		dealer.solve(card_odds, init, final);
		dealer_final_vector_type& row(dealer_final_given_revealed[j]);
		dealer_final_vector_type::const_iterator i(&final[stop]);
		copy(i, i+cols, row.begin());
	}
#if DUMP_DURING_EVALUATE
	dump_dealer_final_table(cout) << endl;
#endif
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
strategy::dump_dealer_final_table(ostream& o) const {
	o << "Dealer's final state odds:\n";
	const dealer_final_matrix::const_iterator
		b(dealer_final_given_revealed.begin()),
		e(dealer_final_given_revealed.end());
	dealer_final_matrix::const_iterator i(b);
	ostream_iterator<probability_type> osi(o, "\t");
	o << "show\\do\t17\t18\t19\t20\t21\tbust\n";
	o << setprecision(4);
	for ( ; i!=e; ++i) {
		o << dealer[initial_card_states[i-b]].name << '\t';
		copy(i->begin(), i->end(), osi);
		o << endl;
	}
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\pre already called compute_dealer_final_state().
	\post populates values of ::player_stand and ::player_stand_edges
 */
void
strategy::compute_player_stand_odds(void) {
	// represents: <=16, 17, 18, 19, 20, 21, bust
	size_t j;
	for (j=0; j<vals; ++j) {	// dealer's revealed card, initial state
	size_t k;
	for (k=0; k<cols; ++k) {	// player's final state
		outcome_odds& o(player_stand[j][k]);
	size_t d;
	for (d=0; d<cols-1; ++d) {	// dealer's final state
		const probability_type& p(dealer_final_given_revealed[j][d]);
		int diff = k -d;
		if (diff > 1) {
			o.win += p;
		} else if (diff == 1) {
			o.push += p;
		} else {
			o.lose += p;
		}
	}
		o.win += dealer_final_given_revealed[j][d];	// dealer busts
	}
	player_stand[j][k].lose += 1.0;	// player busts
	}
	// now compute player edges
	for (j=0; j<vals; ++j) {
		transform(player_stand[j].begin(), player_stand[j].end(), 
			player_stand_edges[j].begin(), 
			mem_fun_ref(&outcome_odds::edge));
		// TODO: or mem_fun_ref(&outcome_odds::ratioed_edge)?
	}
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
ostream&
strategy::dump_player_stand_odds(ostream& o) const {
{
//	o << "Player's stand odds (win|push|lose):\n";
	o << "Player's stand odds:\n";
	const outcome_matrix::const_iterator
		b(player_stand.begin()), e(player_stand.end());
	outcome_matrix::const_iterator i(b);
//	ostream_iterator<outcome_odds> osi(o, "\t");
	o << "D\\P";
	size_t j;
	for (j=0; j<=cols; ++j) {
		o << '\t' << player_final_states[j];
	}
	o << endl;
	o << setprecision(3);
	for ( ; i!=e; ++i) {
		o << dealer[initial_card_states[i-b]].name << endl;
		dump_outcome_vector(*i, o);
		o << endl;
	}
}{
	// dump the player's edges (win -lose)
	o << "Player's stand edges:\n";
	const player_stand_edges_matrix::const_iterator
		b(player_stand_edges.begin()), e(player_stand_edges.end());
	player_stand_edges_matrix::const_iterator i(b);
	o << "D\\P";
	size_t j;
	for (j=0; j<=cols; ++j) {
		o << '\t' << player_final_states[j];
	}
	o << endl;
	for ( ; i!=e; ++i) {
		o << dealer[initial_card_states[i-b]].name << '\t';
//		o << setprecision(3);
		copy(i->begin(), i->end(),
			ostream_iterator<probability_type>(o, "\t"));
		o << endl;
	}
}
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	This is the main strategy solver.  
 */
void
strategy::compute_action_expectations(void) {
	// recall, table is transposed
// stand
	size_t i, j;
	for (i=0; i<phit_states; ++i) {
	for (j=0; j<vals; ++j) {
		const probability_type&
			edge(player_stand_edges[j][player_final_state_map(i)]);
		player_actions[i][j].stand = edge;
	}
	}
// double-down (hit-once, then stand)
	// TODO: this could be computed at the same time with hits
	// if done in reverse order
	for (i=0; i<phit_states; ++i) {
	if (player_hit[i].size()) {
		probability_vector before_hit(phit_states, 0.0);
		before_hit[i] = 1.0;	// unit vector
		probability_vector after_hit;
		player_hit.convolve(card_odds, before_hit, after_hit);
		player_final_state_probability_vector fin;
		player_final_state_probabilities(after_hit, fin);
	for (j=0; j<vals; ++j) {	// dealer shows
		// static_assert
		assert(size_t(player_final_state_probability_vector::Size)
			== size_t(outcome_vector::Size));
		probability_type& p(player_actions[i][j].double_down);
		// perform inner_product, weighted expectations
		size_t k;
		for (k=0; k<=cols; ++k) {
			const probability_type& edge(player_stand_edges[j][k]);
			p += fin[k] *edge;
		}
		p *= double_multiplier;		// because bet is doubled
	}
	} else {			// is terminal state, irrelevant
	for (j=0; j<vals; ++j) {
		player_actions[i][j].double_down = -double_multiplier;
	}
	}
	}
// hit (and optimize choices, excluding splits)
	vector<size_t> reorder;		// reordering for-loop: player states
	reorder.reserve(phit_states);
	// backwards, starting from high hard value states
	for (i=goal-1; i>goal-vals; --i) {
		reorder.push_back(i);
	}
	// backwards from soft states (Aces)
	for (i=phit_states-1; i>=soft; --i) {
		reorder.push_back(i);
	}
	// backwards from low hard values
	for (i=goal-vals; i<phit_states; --i) {
		reorder.push_back(i);
	}
	size_t r;
	for (r=0; r<reorder.size(); ++r) {
		const size_t i = reorder[r];
		const state_machine::node& ni(player_hit[i]);
		assert(ni.size());
	for (j=0; j<vals; ++j) {
		expectations& p(player_actions[i][j]);
		p.hit = 0.0;
	size_t k;
	for (k=0; k<vals; ++k) {
//		cout << "i,j=" << i << ',' << j <<
//			", card-index " << k << ", odds=" << card_odds[k];
		if (player_hit[k].size()) {
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
			const probability_type& edge(player_stand_edges[j]
				[player_final_state_map(ni[k])]);
//			cout << ", edge=" << edge;
			p.hit += card_odds[k] *edge;
		}
//		cout << endl;
	}
		// p.optimize();
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
if (split) {
	const bool DAS = double_after_split && some_double();
if (resplit) {
	cout << "Resplitting allowed." <<  endl;
	// need to work backwards from post-split edges
	compute_player_initial_edges(DAS, resplit, false);
	compute_player_split_edges(DAS, resplit);	// iterate
	compute_player_initial_edges(DAS, resplit, false);
	compute_player_split_edges(DAS, resplit);	// iterate
	compute_player_initial_edges(some_double(), true, surrender_late);
} else {
	cout << "Single split allowed." <<  endl;
	compute_player_initial_edges(DAS, false, false);
	compute_player_split_edges(DAS, false);	// iterate
	compute_player_initial_edges(some_double(), true, surrender_late);
}
#if DUMP_DURING_EVALUATE
	dump_player_split_edges(cout) << endl;
#endif
} else {
	cout << "No split allowed." <<  endl;
	// no splitting allowed!
	compute_player_initial_edges(some_double(), false, surrender_late);
}
	// to account for player's blackjack
	finalize_player_initial_edges();
#if 0
	dump_player_initial_edges(cout) << endl;
#endif
// next: compute conditional edges given dealer reveal card
#if DUMP_DURING_EVALUATE
	dump_action_expectations(cout);
#endif
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Consider splitting not an option.  
 */
void
strategy::reset_split_edges(void) {
	player_initial_edges_vector *player_split_edges =
		&player_initial_edges[phit_states];
	size_t i;
	for (i=0; i<vals; ++i) {
#if 0
	// the single-card state after split
	size_t j;
	for (j=0; j<vals; ++j) {	// for each dealer reveal card
		// player_split_edges[i][j] = -2.0;
		// initially no worse than hitting/standing
		player_split_edges[i][j] = player_hit_edges[initial_card_states[i]][j];
	}
#else
		player_split_edges[i] = player_hit_edges[initial_card_states[i]];
#endif
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Evaluates when player should split pairs.  
	NOTE: this updates the split-expectations of the player_actions table!
	This does not modify the (non-split) player_initial_edges tables.
 */
void
strategy::compute_player_split_edges(const bool d, const bool s) {
	player_initial_edges_vector *player_split_edges =
		&player_initial_edges[phit_states];
	size_t i;
	for (i=0; i<vals; ++i) {
	// the single-card state after split
	size_t j;
	const size_t p = player_hit[initial_card_states[i]][i];
	for (j=0; j<vals; ++j) {	// for each dealer reveal card
		// need to take weighted sum over initial states
		// depending on initial card and spread of next states.
		edge_type expected_edge = 0.0;
	// convolution: spread over the player's next card
	size_t k;
	for (k=0; k<vals; ++k) {	// player's second card
		const probability_type& o(card_odds[k]);
		// initial edges may not have double-downs, splits, nor surrenders
		// since player is forced to take the next card in each hand
		const size_t state = player_hit[initial_card_states[i]][k];
		edge_type edge = player_initial_edges[state][j];
		// similar to expectation::best(d, i==k, r)
#if 1
		if (resplit && (i == k)) {	// if resplit?
			// then can consider split entry
			const edge_type& split_edge(player_split_edges[i][j]);
#if 0
			if (split_edge > edge) {
				// weight by probability of pair
				edge = (o * split_edge) +(1.0 -o) *edge;
			}
#else
			edge = split_edge;
#endif
		}
#endif
		if ((i == ACE) && one_card_on_split_aces) {
			// each aces takes exactly one more card only
			edge = player_stand_edges[j][player_final_state_map(state)];
//			cout << "1-card-ACE[D" << j << ",F" << state << "]: " << edge << endl;
		}
		expected_edge += o * edge;
	}	// inner product
		expectations& sum(player_actions[p][j]);
		sum.split = 2.0 *expected_edge;	// b/c two hands are played
		sum.optimize();
#if 0
		// TODO: update this in second pass?
		player_split_edges[i][j] =
			sum.value(sum.best(true, true, false));
#endif
	}	// end for each dealer reveal card
	}	// end for each player paired card

	// update the split entries for the initial-edges
	for (i=0; i<vals; ++i) {
	// the single-card state after split
	size_t j;
	const size_t p = player_hit[initial_card_states[i]][i];
	for (j=0; j<vals; ++j) {	// for each dealer reveal card
		const expectations& sum(player_actions[p][j]);
		// player may decide whether or not to split
		player_split_edges[i][j] =
			sum.value(sum.best(d, s, false));
	}
	}
#if DUMP_DURING_EVALUATE
	dump_player_split_edges(cout) << endl;
#endif
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
strategy::dump_player_split_edges(ostream& o) const {
	// dump the player's initial state edges 
	const player_initial_edges_vector *player_split_edges =
		&player_initial_edges[phit_states];
	o << "Player's split-permitted edges:" << endl;
	o << "P\\D";
	size_t j;
	for (j=0; j<vals; ++j) {
		o << '\t' << card_name[j];
	}
	o << endl;
	for (j=0 ; j<vals; ++j) {
		o << card_name[j] << ',' << card_name[j];
		size_t i;
		for (i=0; i<vals; ++i) {
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
	Use this table to evaluate expected outcomes.  
	\pre player states are already optimized for stand,hit,double.
 */
void
strategy::optimize_player_hit_tables(void) {
	size_t j;
	for (j=0; j<vals; ++j) {
		player_opt[j] = player_hit;	// copy state machine
		// use default assignment operator of util::array
		size_t i;
		for (i=0; i<phit_states; ++i) {
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
	probability_vector unit(phit_states, 0.0);
	probability_vector result;
	for (j=0; j<vals; ++j) {
		size_t i;
		for (i=0; i<phit_states; ++i) {
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
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
strategy::dump_player_hit_tables(ostream& o) const {
	size_t j;
	for (j=0; j<vals; ++j) {
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
	for (j=0; j<vals; ++j) {
		o << "Dealer shows " << card_name[j] <<
			", player\'s final state spread:" << endl;
		o << "player";
		size_t i;
		for (i=0; i<=cols; ++i) {
			o << '\t' << player_final_states[i];
		}
		o << endl;
		for (i=0; i<phit_states; ++i) {
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
 */
void
strategy::compute_player_hit_edges(void) {
	static const edge_type eps = 
		sqrt(std::numeric_limits<edge_type>::epsilon());
	size_t i;
	for (i=0; i<vals; ++i) {
		const player_stand_edges_vector&
			edges(player_stand_edges[i]);
	size_t j;
	for (j=0; j<phit_states; ++j) {
		// probability-weighted sum of edge values for each state
		const edge_type e = inner_product(edges.begin(), edges.end(), 
			player_final_state_probability[i][j].begin(), 0.0);
		player_hit_edges[j][i] = e;
		const expectations& x(player_actions[j][i]);
		const edge_type sh = x.value(x.best(false, false, false));
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
ostream&
strategy::dump_player_hit_edges(ostream& o) const {
	// dump the player's hitting edges 
	o << "Player's hit edges:\n";
	o << "P\\D";
	size_t j;
	for (j=0; j<vals; ++j) {
		o << '\t' << card_name[j];
	}
	o << endl;
	for (j=0 ; j<phit_states; ++j) {
		o << player_hit[j].name;
		size_t i;
		for (i=0; i<vals; ++i) {
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
 */
void
strategy::compute_player_initial_edges(
		const bool D, const bool S, const bool R) {
//	const player_initial_edges_vector *player_split_edges =
//		&player_initial_edges[phit_states];
	size_t i;
	for (i=0; i<phit_states; ++i) {		// exclude splits first
	size_t j;
	for (j=0; j<vals; ++j) {
		expectations c(player_actions[i][j]);	// yes, copy
		c.hit = player_hit_edges[i][j];
		c.optimize();
		// since splits are folded into non-pair states
		const pair<player_choice, player_choice>
//			splits are computed in a separate section of the table
			p(c.best_two(D, S, R));
		const edge_type e = c.value(p.first);
		player_initial_edges[i][j] = e;
	}
	}
#if DUMP_DURING_EVALUATE
	dump_player_initial_edges(cout) << endl;
#endif
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	After dealer has checked for blackjack, and doesn't have it, 
	if the player has blackjack, she automatically wins +payoff.  
 */
void
strategy::finalize_player_initial_edges(void) {
#if 0
	fill(player_initial_edges[goal].begin(), 
		player_initial_edges[goal].end(), bj_payoff);
#endif
#if DUMP_DURING_EVALUATE
	dump_player_initial_edges(cout) << endl;
#endif
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
strategy::dump_player_initial_edges(ostream& o) const {
	// dump the player's initial state edges 
	o << "Player's initial edges:" << endl;
	o << "P\\D";
	size_t j;
	for (j=0; j<vals; ++j) {
		o << '\t' << card_name[j];
	}
	o << endl;
	for (j=0 ; j<phit_states; ++j) { // print split edges separately
		o << player_hit[j].name;
		size_t i;
		for (i=0; i<vals; ++i) {
			o << '\t' << player_initial_edges[j][i];
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
	for (i=0; i<phit_states; ++i) {
	size_t j;
	for (j=0; j<vals; ++j) {
		player_actions[i][j].optimize();
	}
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	After having computed initial edges, evaluate the 
	player's expectation, given only the dealer's reveal card.  
 */
void
strategy::compute_reveal_edges(void) {
size_t r;	// index of dealer's reveal card
for (r=0; r<vals; ++r) {
	edge_type& x = player_edges_given_reveal[r];
	x = 0.0;
	size_t i;	// index of player's initial state
	for (i=0; i<phit_pair_states; ++i) {
		// inner product
		x += player_initial_state_odds[i] *player_initial_edges[i][r];
	}
}
#if DUMP_DURING_EVALUATE
	dump_reveal_edges(cout) << endl;
#endif
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
strategy::dump_reveal_edges(ostream& o) const {
	o << "Player edges given dealer's revealed card:\n";
	copy(card_name, card_name+TEN+1, ostream_iterator<char>(o, "\t"));
	o << endl;
	copy(player_edges_given_reveal.begin(),
		player_edges_given_reveal.end(), 
		ostream_iterator<edge_type>(o, "\t"));
	return o << endl;
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
	const probability_type& a(card_odds[ACE]);
	const probability_type& t(card_odds[ACE]);
	const probability_type bj_odds = 2 *a *t;
	cout << "blackjack odds = " << bj_odds << endl;
	// skip ACE and TEN because case is computed separately
	const edge_type n =
		inner_product(card_odds.begin()+1, card_odds.end()-1, 
			player_edges_given_reveal.begin() +1, 0.0);
	cout << "normal play edge (no ACE, no TEN) = " << n << endl;
#if 1
	_overall_edge =
		a*(		// A: dealer-has-ace
			t*(bj_odds-1)	// dealer-has-blackjack, player does not
			// dealer-no-blackjack
			+(1-t)*player_edges_given_reveal[ACE])
		+t*(		// B: dealer-has-ten
			a*(bj_odds-1)	// dealer-has-blackjack, player does not
			// dealer-no-blackjack
			+(1-a)*player_edges_given_reveal[TEN])
		+(1-a-t)*(	// C: dealer-no-ace
			(bj_odds * bj_payoff)	// player-has-blackjack
			+(1-bj_odds) *n);
#else
// does dealer have blackjack? 10-A or A-10
	const probability_type nbj = 1.0 - bj_odds;
	_overall_edge = 0.0;
	// dealer has blackjack and player doesn't:
	// does NOT depend on peek variation
	const probability_type q = bj_odds * nbj;
	_overall_edge += q * (bj_payoff -1.0);		// player loses bet
	cout << "blackjack edge = " << _overall_edge << endl;
	// player blackjack is NOT accounted in player_initial_edges[goal][].
	// if both have blackjack, it's a push.
	// don't bother with insurance, is only count-dependent
	// normal play:
	cout << "normal play edge, weight = " << n << ", " << nbj * nbj << endl;
	_overall_edge += nbj * nbj * n;	// no dealer bj
#endif
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
strategy::dump_expectations(const expectations_vector& v, ostream& o) {
	const expectations_vector::const_iterator b(v.begin()), e(v.end());
	expectations_vector::const_iterator i(b);

	for (; i!=e; ++i) {
		o << '\t' << action_key[i->actions[0]]
			<< action_key[i->actions[1]]
			<< action_key[i->actions[2]];
	}
	o << endl;
	expectations z(0,0,0,0);
	z = accumulate(b, e, z);
#if 1
#define	EFORMAT(x)	setw(5) << int(x*1000)
// setprecision?
#else
#define	EFORMAT(x)	x
#endif
	o << "stand";
	for (i=b; i!=e; ++i) {
		o << '\t' << EFORMAT(i->stand);
	}
	o << endl;
if (z.hit > -1.0 * vals) {
	o << "hit";
	for (i=b; i!=e; ++i) {
		o << '\t' << EFORMAT(i->hit);
	}
	o << endl;
}
if (z.double_down > -2.0 *vals) {
	// TODO: pass double_multiplier?
	o << "double";
	for (i=b; i!=e; ++i) {
		o << '\t' << EFORMAT(i->double_down);
	}
	o << endl;
}
// for brevity, could omit non-splittable states...
if (z.split > -2.0 *vals) {
	o << "split";
	for (i=b; i!=e; ++i) {
		o << '\t' << EFORMAT(i->split);
	}
	o << endl;
}
#if 1
	o << "surr.";
	for (i=b; i!=e; ++i) {
		o << '\t' << EFORMAT(expectations::surrender);
	}
	o << endl;
#endif
#if 1
	o << "delta";
	for (i=b; i!=e; ++i) {
		o << '\t';
		const pair<player_choice, player_choice>
			opt(i->best_two(true, true, true));
		o << EFORMAT(i->margin(opt.first, opt.second));
	}
	o << endl;
#endif

#undef	EFORMAT
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const edge_type&
strategy::expectations::value(const player_choice c) const {
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
strategy::expectations::optimize(void) {
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
pair<strategy::player_choice, strategy::player_choice>
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
	o << "Player's action expectations:\n";
	const expectations_matrix::const_iterator
		b(player_actions.begin()), e(player_actions.end());
	expectations_matrix::const_iterator i(b);
	// ostream_iterator<expectations> osi(o, "\t");
	o << "P\\D";
	size_t j;
	for (j=0; j<vals; ++j) {
		o << '\t' << card_name[j];
	}
	o << '\n' << endl;
	o << setprecision(3);
	for ( ; i!=e; ++i) {
		o << player_hit[i-b].name;
//		o << endl;
		dump_expectations(*i, o);
		o << endl;
	}
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
strategy::dump(ostream& o) const {
	variation::dump(o) << endl;
	dump_dealer_policy(o);
	dump_player_hit_state(o);
	dump_dealer_final_table(o) << endl;
	dump_player_stand_odds(o) << endl;
	dump_action_expectations(o);
	dump_reveal_edges(o) << endl;
	o << "Player\'s overall edge = " << overall_edge() << endl;
	return o;
}

//=============================================================================
}	// end namespace blackjack

