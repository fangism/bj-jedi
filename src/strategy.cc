// "strategy.cc"

#include <cassert>
#include <algorithm>
#include <functional>
#include <numeric>		// for accumulate
#include <limits>		// for numeric_limits
#include <iostream>
#include <iterator>
#include <iomanip>
#include <cmath>		// for fabs
#include "strategy.hh"
#include "variation.hh"
#include "blackjack.hh"
#include "util/array.tcc"
#include "util/probability.tcc"
// for commands
#include "util/string.tcc"
#include "util/command.tcc"
#include "util/value_saver.hh"

/**
	Debug switch: print tables as they are computed.
 */
#define		DUMP_DURING_EVALUATE		0
namespace blackjack {
typedef	util::Command<const strategy>		StrategyCommand;
}
namespace util {
template class command_registry<blackjack::StrategyCommand>;
}

namespace blackjack {
typedef	util::command_registry<StrategyCommand>		strategy_command_registry;
using std::vector;
using std::fill;
using std::copy;
using std::for_each;
using std::transform;
using std::inner_product;
using std::accumulate;
using std::mem_fun_ref;
using std::ostream_iterator;
using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::multimap;
using std::make_pair;
using std::setw;
using std::setprecision;
using util::normalize;
using cards::ACE;
using cards::TEN;
using cards::card_name;
using cards::standard_deck_distribution;
using cards::reveal_print_ordering;
using cards::card_index;

using util::value_saver;
using util::Command;
using util::CommandStatus;
using util::string_list;
using util::strings::string_to_num;

//=============================================================================
// Blackjack specific routines

// TODO: relocate this
const char
strategy::action_key[] = "-SHDPR";
// - = nil (invalid)
// S = stand
// H = hit
// D = double
// P = split pair
// R = surrender (run!)

//-----------------------------------------------------------------------------
// class strategy method definitions

void
strategy::set_card_distribution(const deck_distribution& o) {
	card_odds = o;
	need_update = true;
//	update_player_initial_state_odds();
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
#if FACE_CARDS
void
strategy::set_card_distribution(const extended_deck_count_type& o) {
	deck_count_type t;
	copy(o.begin(), o.begin() +t.size(), t.begin());	// convert int to real
	t[TEN] += o[cards::JACK] +o[cards::QUEEN] +o[cards::KING];
	set_card_distribution(t);
}
#endif

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
if (need_update) {
	update_player_initial_state_odds();
	compute_dealer_final_table();
	compute_player_stand_odds();
	compute_action_expectations();
	compute_reveal_edges();
	compute_overall_edge();
	need_update = false;
}
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
strategy::strategy(const play_map& v) : 
	var(v.var),
	play(v), 
	card_odds(standard_deck_distribution),
	need_update(true) {
	// only depends on H17:
	// don't *have* to do this right away...
	// update_player_initial_state_odds();
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
	static const char header[] = 
		"show\\do\t17\t18\t19\t20\t21\tBJ\tbust\tpush";
	ostream_iterator<probability_type> osi(o, "\t");
{
	const dealer_final_matrix::const_iterator
		b(dealer_final_given_revealed.begin()),
		e(dealer_final_given_revealed.end());
	dealer_final_matrix::const_iterator i(b);
	o << header << endl;
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
	o << header << endl;
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
	// first, clear values back to 0
	outcome_matrix::iterator mi(stand.begin()), me(stand.end());
	for ( ; mi!=me; ++mi) {
		fill(mi->begin(), mi->end(), outcome_odds());
	}
	// represents: <=16, 17, 18, 19, 20, 21, BJ, bust
//	static const size_t d_bj_ind = dealer_states -3;
	static const size_t p_bj_ind = player_states -2;
	size_t j;
	for (j=0; j < card_values; ++j) {	// dealer's revealed card, initial state
		const dealer_final_vector_type& dfv(dfm[j]);	// pre-peek!
		outcome_vector& ps(stand[j]);
	size_t k;
	// player blackjack and bust is separate
	for (k=0; k < player_states; ++k) {	// player's final state
		outcome_odds& o(ps[k]);
		const play_map::outcome_array_type&
			v(play_map::outcome_matrix[k]);
	size_t d;
	// -1: blackjack, push, and bust states separate
	for (d=0; d < dealer_states; ++d) {	// dealer's final state
		const probability_type& p(dfv[d]);
		switch (v[d]) {
		case WIN: o.win += p; break;
		case LOSE: o.lose += p; break;
		case PUSH: o.push += p; break;
		}
	}	// end for dealer_states
	}	// end for player_states
#if 0
		const probability_type& d_bj(dfv[d_bj_ind]);
		ps[k].win += 1.0 -d_bj;	// player blackjack, dealer none
		ps[k].push += d_bj;	// both blackjack
		ps[k+1].lose += 1.0;	// player busts
#endif
	}	// end for card_values
	// now compute player edges
	for (j=0; j<card_values; ++j) {	// dealer's reveal card
		const outcome_vector& ps(stand[j]);
		player_stand_edges_vector& ev(edges[j]);
		transform(ps.begin(), ps.end(), ev.begin(), 
			mem_fun_ref(&outcome_odds::edge));
		// later, compensate for player blackjack, pays 3:2
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
#if 0
		p = 0.0;
		size_t k;
		for (k=0; k<player_states; ++k) {
			const probability_type& edge(pse[j][k]);
			p += fin[k] *edge;
		}
#else
		p = inner_product(fin.begin(), fin.end(), pse[j].begin(), 0.0);
#endif
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
		sum.optimize(-var.surrender_penalty);
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
			sum.value(sum.best(d, s, false), -var.surrender_penalty);
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
		-var.surrender_penalty, player_hit_edges);
#else
	__compute_player_hit_edges(player_stand_edges, 
		player_final_state_probability, player_actions, 
		-var.surrender_penalty, player_hit_edges);
	__compute_player_hit_edges(player_stand_edges_post_peek, 
		player_final_state_probability, player_actions, 
		-var.surrender_penalty, player_hit_edges_post_peek);
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
		c.optimize(-var.surrender_penalty);
		// since splits are folded into non-pair states
		const pair<player_choice, player_choice>
//			splits are computed in a separate section of the table
			p(c.best_two(D, S, R));
		const edge_type e = c.value(p.first, -var.surrender_penalty);
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
		player_actions[i][j].optimize(-var.surrender_penalty);
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
	\param p player's state
	\param d dealer's reveal card
 */
const expectations&
strategy::lookup_player_action_expectations(
		const size_t p, const size_t d) const {
	return player_actions[p][d];
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
		o << '\t' << EFORMAT(-var.surrender_penalty);
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
		o << EFORMAT(ex.margin(opt.first, opt.second,
			-var.surrender_penalty));
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
		case SURRENDER: o << EFORMAT(-var.surrender_penalty); break;
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
		o << EFORMAT(ex.margin(opt.first, opt.second,
			-var.surrender_penalty));
	}
	o << endl;
#endif
#undef	EFORMAT
	return o;
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
	play_map::dump_final_outcomes(o) << endl;	// verified
	play.dump_dealer_policy(o) << endl;		// verified
	play.dump_player_hit_state(o) << endl;		// verified
	dump_dealer_final_table(o) << endl;		// verified
	dump_player_stand_odds(o) << endl;		// verified

	dump_action_expectations(o);
	dump_reveal_edges(o) << endl;
	const edge_type e(overall_edge());
	o << "Player\'s overall edge = " << e <<
		" (" << e*100.0 << "%)" << endl;
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int
strategy::command(const string_list& cmd) const {
	return strategy_command_registry::execute(*this, cmd);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int
strategy::main(const char* prompt) const {
	cout <<
"Strategy menu.\n"
"Type 'help' or '?' for a list of information commands." << endl;
	const value_saver<string>
		tmp1(strategy_command_registry::prompt, prompt);
	const value_saver<util::completion_function_ptr>
		tmp(rl_attempted_completion_function,
			&strategy_command_registry::completion);
	strategy_command_registry::interpret(*this);
	return 0;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace strategy_commands {
#define DECLARE_STRATEGY_COMMAND_CLASS(class_name, _cmd, _brief)	\
	DECLARE_AND_INITIALIZE_COMMAND_CLASS(const strategy, class_name, _cmd, _brief)

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_STRATEGY_COMMAND_CLASS(Help, "help", ": list all strategy commands")
int
Help::main(const strategy& g, const string_list&) {
	strategy_command_registry::list_commands(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_STRATEGY_COMMAND_CLASS(Help2, "?", ": list all strategy commands")
int
Help2::main(const strategy& g, const string_list&) {
	strategy_command_registry::list_commands(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_STRATEGY_COMMAND_CLASS(Quit, "quit", ": exit strategy menu")
int
Quit::main(const strategy& g, const string_list&) {
	return CommandStatus::END;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_STRATEGY_COMMAND_CLASS(Exit, "exit", ": exit strategy menu")
int
Exit::main(const strategy& g, const string_list& args) {
	return Quit::main(g, args);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_STRATEGY_COMMAND_CLASS(Echo, "echo", ": print arguments")
int
Echo::main(const strategy& g, const string_list& args) {
	copy(++args.begin(), args.end(), ostream_iterator<string>(cout, " "));
	cout << endl;
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_STRATEGY_COMMAND_CLASS(Variation, "variation", ": show rule variations")
int
Variation::main(const strategy& g, const string_list& args) {
if (args.size() > 1) {
	cout << "error: cannot change rules variations here." << endl;
	return CommandStatus::BADARG;
} else {
	// can't modify/configure here
	g.var.dump(cout);
	return CommandStatus::NORMAL;
}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// information dump commands
DECLARE_STRATEGY_COMMAND_CLASS(StandEdges, "stand-edges",
	": show all standing edges")
int
StandEdges::main(const strategy& g, const string_list& args) {
	g.dump_player_stand_odds(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_STRATEGY_COMMAND_CLASS(InitialOdds, "initial-odds",
	": show prob. dist. of all initial hands")
int
InitialOdds::main(const strategy& g, const string_list& args) {
	g.dump_player_initial_state_odds(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_STRATEGY_COMMAND_CLASS(InitialEdges, "initial-edges",
	": show all edges of initial hands")
int
InitialEdges::main(const strategy& g, const string_list& args) {
	g.dump_player_initial_edges(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_STRATEGY_COMMAND_CLASS(HitEdges, "hit-edges",
	": show all hitting edges")
int
HitEdges::main(const strategy& g, const string_list& args) {
	g.dump_player_hit_edges(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_STRATEGY_COMMAND_CLASS(HitStrategy, "hit-strategy",
	": show optimal hit strategy vs. dealer")
int
HitStrategy::main(const strategy& g, const string_list& args) {
// TODO: optional arg, given reveal
	g.dump_player_hit_tables(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_STRATEGY_COMMAND_CLASS(SplitEdges, "split-edges",
	": show all splitting edges")
int
SplitEdges::main(const strategy& g, const string_list& args) {
	g.dump_player_split_edges(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_STRATEGY_COMMAND_CLASS(PlayerFinalOdds, "player-final-state-odds",
	": show prob. dist. of player's final state vs. dealer")
int
PlayerFinalOdds::main(const strategy& g, const string_list& args) {
// TODO: optional arg, given reveal
	g.dump_player_final_state_probabilities(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_STRATEGY_COMMAND_CLASS(RevealEdges, "reveal-edges",
	": show initial edges pre/post-peek")
int
RevealEdges::main(const strategy& g, const string_list& args) {
	g.dump_reveal_edges(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_STRATEGY_COMMAND_CLASS(OverallEdge, "overall-edge",
	": show pre-deal overall player's edge")
int
OverallEdge::main(const strategy& g, const string_list& args) {
	cout << "overall edge: " << g.overall_edge() << endl;
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_STRATEGY_COMMAND_CLASS(ActionEdges, "action-edges",
	": show complete edges/action table vs. dealer")
int
ActionEdges::main(const strategy& g, const string_list& args) {
	g.dump_action_expectations(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_STRATEGY_COMMAND_CLASS(ActionOptimal, "action-optimal",
	": show optimal action table vs. dealer")
int
ActionOptimal::main(const strategy& g, const string_list& args) {
	g.dump_optimal_actions(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_STRATEGY_COMMAND_CLASS(AllInfo, "all-info",
	": show all edge calculations")
int
AllInfo::main(const strategy& g, const string_list& args) {
	g.dump(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace strategy_commands

//=============================================================================
}	// end namespace blackjack

