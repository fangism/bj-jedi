// "bj/core/strategy.cc"

#include <cassert>
#include <algorithm>
#include <functional>
#include <numeric>		// for accumulate
#include <limits>		// for numeric_limits
#include <iostream>
#include <iomanip>
#include <iterator>
#include <cmath>		// for fabs
#include "bj/core/strategy.hh"
#include "bj/core/variation.hh"
#include "bj/core/blackjack.hh"
#include "util/array.tcc"
#include "util/probability.tcc"
// for commands
#include "util/string.tcc"
#include "util/command.tcc"
#include "util/value_saver.hh"
#include "util/iosfmt_saver.hh"
#include "util/member_select_iterator.hh"

/**
	Debug switch: print tables as they are computed.
 */
#define		DUMP_DURING_EVALUATE		0

#define		DEBUG_SPLIT_CALC		0
#if DEBUG_SPLIT_CALC
#define	DEBUG_SPLIT_PRINT(o, x)			o << x
#else
#define	DEBUG_SPLIT_PRINT(o, x)
#endif

namespace blackjack {
typedef	util::Command<const strategy>		StrategyCommand;
}
namespace util {
template class command_registry<blackjack::StrategyCommand>;
}

namespace blackjack {
typedef	util::command_registry<StrategyCommand>		strategy_command_registry;
using std::ios_base;
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
using std::bind2nd;
using util::normalize;
using util::precision_saver;
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
using util::member_select_iterator;
using util::var_member_select_iterator;

#define	DP_WRAP(x, y)		action_mask(action_mask::dp_tag(), x, y)
#define	DPR_WRAP(x, y, z)	action_mask(action_mask::dpr_tag(), x, y, z)
#define	STAND_HIT_ONLY		action_mask::stand_hit
#define	ALL_OPTIONS		action_mask::all

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
	initial_need_update = true;
	overall_need_update = true;
	card_type i = 0;
	for ( ; i<card_values; ++i) {
		reveal[i].need_update = true;
	}
//	update_player_initial_state_odds();	// lazy update, only when needed
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
strategy::set_card_distribution(const deck_count_type& o) {
	card_count = o;
	deck_distribution t;
	copy(o.begin(), o.end(), t.begin());	// convert int to real
	normalize(t);
	set_card_distribution(t);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
strategy::set_card_distribution(const extended_deck_count_type& o) {
	// TODO: convert extended to non-extended count
	deck_count_type t;
	copy(o.begin(), o.begin() +t.size(), t.begin());	// convert int to real
	t[TEN] += o[cards::JACK] +o[cards::QUEEN] +o[cards::KING];
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
if (initial_need_update) {
	fill(player_initial_state_odds.begin(),
		player_initial_state_odds.end(), 0.0);
	card_type i;
for (i=0; i<card_values; ++i) {
	const probability_type& f(card_odds[i]); // / not_bj_odds;
	// scale up, to normalize for lack of blackjack
	card_type j;
	for (j=0; j<card_values; ++j) {
		const probability_type fs = f * card_odds[j];
		if (i == j) {		// splittable pairs
			player_initial_state_odds[pair_offset +i] += fs;
		} else {
			const player_state_type k =
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
	initial_need_update = false;
}
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
void
reveal_strategy::evaluate(
	const play_map& play, const deck_distribution& card_odds, 
	const player_state_probability_vector& player_initial_state_odds) {
if (need_update) {
	const variation& var(play.var);
	compute_dealer_final_table(play, card_odds);
	compute_player_stand_odds(play, var.bj_payoff);
	compute_action_expectations(play, card_odds);
	compute_reveal_edges(card_odds, player_initial_state_odds);
	need_update = false;
}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	To be called after card distribution is set.  
	Evaluate's all cases of reveal cards.
 */
void
strategy::evaluate(void) {
	update_player_initial_state_odds();
	card_type i = 0;
	for ( ; i<card_values; ++i) {
		evaluate(i);
	}
	compute_overall_edge();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
strategy::evaluate(const card_type c) {
	update_player_initial_state_odds();
	reveal[c].evaluate(play, card_odds, player_initial_state_odds);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Folds probability vector of player states to compact representation.
	Result goes into r.  
 */
void
reveal_strategy::player_final_state_probabilities(const probability_vector& s, 
		player_final_state_probability_vector& r) {
	assert(s.size() == p_action_states);
	fill(r.begin(), r.end(), 0.0);
	player_state_type i;
	for (i=0; i<p_action_states; ++i) {
		r[play_map::player_final_state_map(i)] += s[i];
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
reveal_strategy::reveal_strategy() :
		need_update(true) {
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
strategy::strategy(const play_map& v) : 
	var(v.var),
	play(v), 
	card_odds(standard_deck_distribution),
	initial_need_update(true),
	overall_need_update(true) {
	// each sub-strategy needs to know it's corresponding reveal card
	card_type j = 0;
	for ( ; j<card_values; ++j) {
		reveal[j].reveal_card = j;
	}
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
reveal_strategy::compute_dealer_final_table(const play_map& play, 
		const deck_distribution& card_odds) {
	const card_type j = reveal_card;
	const variation& var(play.var);
	probability_vector init(play.dealer_hit.get_states().size(), 0.0);
	probability_vector final(init.size());
	init[play_map::d_initial_card_map[j]] = 1.0;
	// for the first iteration
	// isolate the case when dealer shows an ACE or TEN
	// but hole card is known to NOT give dealer blackjack!
	if (j == ACE) {
		if (var.peek_on_Ace) {
		final = init;
		deck_distribution cards_no_ten(card_odds);
		cards_no_ten[TEN] = 0.0;
		play.dealer_hit.convolve(cards_no_ten, final, init);
		// then solve the rest
		}
	} else if (j == TEN) {
		if (var.peek_on_10) {
		final = init;
		deck_distribution cards_no_ace(card_odds);
		cards_no_ace[ACE] = 0.0;
		play.dealer_hit.convolve(cards_no_ace, final, init);
		}
	}
	play.dealer_hit.solve(card_odds, init, final);
	dealer_final_vector& row(dealer_final_given_revealed_pre_peek);
	dealer_final_vector::const_iterator i(&final[stop]);
	copy(i, i+d_final_states, row.begin());
	// odds of dealer blackjack
	if (j == ACE) {
		 row[d_final_states-3] = card_odds[TEN];
	} else if (j == TEN) {
		 row[d_final_states-3] = card_odds[ACE];
	}

	// post-peek normalizing
	// start by copying, zero out the blackjack odds, and re-normalize
	dealer_final_given_revealed_post_peek = dealer_final_given_revealed_pre_peek;
if (j == ACE) {
	dealer_final_vector& ace_row(dealer_final_given_revealed_post_peek);
	probability_type& dbj(ace_row[dealer_blackjack- stop]);
	if (var.peek_on_Ace && dbj > 0.0) {
		dbj = 0.0;
		normalize(ace_row);
	}
} else if (j == TEN) {
	dealer_final_vector& ten_row(dealer_final_given_revealed_post_peek);
	probability_type& dbj(ten_row[dealer_blackjack- stop]);
	if (var.peek_on_10 && dbj > 0.0) {
		dbj = 0.0;
		normalize(ten_row);
	}
}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
strategy::dump_dealer_final_table(ostream& o) const {
	const precision_saver p(o, 4);
	o << "Dealer's final state odds (pre-peek):\n";
	static const char header[] = "show\\do";
	ostream_iterator<probability_type> osi(o, "\t");
{
	play_map::dealer_final_table_header(o << header) << endl;
#if 0
	typedef member_select_iterator<reveal_array_type::const_iterator,
		reveal_strategy::dealer_final_vector,
		&reveal_strategy::dealer_final_given_revealed_pre_peek>
							const_iterator;
#else
	typedef reveal_array_type::const_iterator	const_iterator;
#endif
	const const_iterator b(reveal.begin()), e(reveal.end());
	const_iterator i(b);
	for ( ; i!=e; ++i) {
		o << play.dealer_hit[play_map::d_initial_card_map[i-b]].name << '\t';
		dump_dealer_final_vector(o,
			i->dealer_final_given_revealed_pre_peek, false);
	}
}
	o << endl;
{
	o << "Dealer's final state odds (post-peek):\n";
	play_map::dealer_final_table_header(o << header) << endl;
#if 0
	typedef member_select_iterator<reveal_array_type::const_iterator,
		reveal_strategy::dealer_final_vector,
		&reveal_strategy::dealer_final_given_revealed_post_peek>
							const_iterator;
#else
	typedef reveal_array_type::const_iterator	const_iterator;
#endif
	const const_iterator b(reveal.begin()), e(reveal.end());
	const_iterator i(b);
	for ( ; i!=e; ++i) {
		o << play.dealer_hit[play_map::d_initial_card_map[i-b]].name << '\t';
		dump_dealer_final_vector(o,
			i->dealer_final_given_revealed_post_peek, false);
	}
}
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
reveal_strategy::compute_showdown_odds(const play_map& play,
		const dealer_final_vector& dfv,
		const edge_type& bjp, player_final_outcome_vector& ps,
		player_stand_edges_vector& edges) {
	static const player_state_type p_bj_ind = p_final_states -2;	// see enums.hh
//	const card_type j = reveal_card;
	play.compute_player_final_outcome_vector(dfv, ps);
	const player_final_outcome_vector& cps(ps);
	transform(cps.begin(), cps.end(), edges.begin(), 
		mem_fun_ref(&outcome_odds::edge));
	// later, compensate for player blackjack, pays 3:2
	edges[p_bj_ind] = cps[p_bj_ind].weighted_edge(bjp, 1.0);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\pre dealer_final_given_revealed computed (pre/post-peek)
	\post computes player_stand (odds-given-reveal) and
		player_stand_edges (weighted expected outcome).
	TODO: evaluate peek for 10 and peek for Ace separately, independently
 */
void
reveal_strategy::compute_player_stand_odds(const play_map& play,
		const edge_type bjp) {
	// pre-peek edges
	compute_showdown_odds(play, dealer_final_given_revealed_pre_peek, bjp,
		player_stand_pre_peek, player_stand_edges_pre_peek);
	// post-peek edges
	compute_showdown_odds(play, dealer_final_given_revealed_post_peek, bjp,
		player_stand_post_peek, player_stand_edges_post_peek);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
/**
	\param d state machine with names of states
 */
ostream&
strategy::__dump_player_stand_odds(ostream& o,
		player_final_outcome_vector reveal_strategy::*mem,
		const state_machine& d) const {
#if 0
	typedef	var_member_select_iterator<reveal_array_type::const_iterator,
			const player_final_outcome_vector>
							const_iterator;
	const const_iterator b(reveal.begin(), mem), e(reveal.end(), mem);
#else
	typedef	reveal_array_type::const_iterator	const_iterator;
	const const_iterator b(reveal.begin()), e(reveal.end());
#endif
	const_iterator i(b);
	play_map::player_final_table_header(o << "D\\P") << endl;
//	const precision_saver p(o, 3);
	for ( ; i!=e; ++i) {
		o << d[play_map::d_initial_card_map[i-b]].name << endl;
		dump_player_final_outcome_vector(i->*mem, o);
		o << endl;
	}
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
strategy::__dump_player_stand_edges(ostream& o,
		reveal_strategy::player_stand_edges_vector
			reveal_strategy::*mem, 
		const state_machine& d) const {
	// dump the player's edges (win -lose)
#if 0
	typedef	var_member_select_iterator<reveal_array_type::const_iterator,
			reveal_strategy::player_stand_edges_vector>
							const_iterator;
	const const_iterator b(reveal.begin(), mem), e(reveal.end(), mem);
#else
	typedef	reveal_array_type::const_iterator	const_iterator;
	const const_iterator b(reveal.begin()), e(reveal.end());
#endif
	const_iterator i(b);
	play_map::player_final_table_header(o << "D\\P") << endl;
//	const precision_saver p(o, 3);
	for ( ; i!=e; ++i) {
		o << d[play_map::d_initial_card_map[i-b]].name << '\t';
		copy(
			(i->*mem).begin(), (i->*mem).end(),
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
	__dump_player_stand_odds(o, &reveal_strategy::player_stand_pre_peek, play.dealer_hit);
	o << "Player's stand odds (post-peek):\n";
	__dump_player_stand_odds(o, &reveal_strategy::player_stand_post_peek, play.dealer_hit);
}{
	// dump the player's edges (win -lose)
	o << "Player's stand edges (pre-peek):\n";
	__dump_player_stand_edges(o, &reveal_strategy::player_stand_edges_pre_peek, play.dealer_hit) << endl;
	o << "Player's stand edges (post-peek):\n";
	__dump_player_stand_edges(o, &reveal_strategy::player_stand_edges_post_peek, play.dealer_hit);
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
reveal_strategy::compute_action_expectations(const play_map& play,
		const deck_distribution& card_odds) {
	DEBUG_SPLIT_PRINT(cout,
		"dealer showing: " << card_name[reveal_card] << endl);
	// pre-peek conditions are not revelant here
	const variation& var(play.var);
	const player_stand_edges_vector&
		pse(player_stand_edges_post_peek);
	// recall, table is transposed
// stand
	player_state_type i;
	for (i=0; i<p_action_states; ++i) {
		const probability_type&
			edge(pse[play_map::player_final_state_map(i)]);
		player_actions[i].stand() = edge;
	}
// double-down (hit-once, then stand)
	// TODO: this could be computed at the same time with hits
	// if done in reverse order
	for (i=0; i<p_action_states; ++i) {
	if (play.player_hit[i].size()) {
		// is non-terminal state
		probability_vector before_hit(p_action_states, 0.0);
		before_hit[i] = 1.0;	// unit vector
		probability_vector after_hit;
		play.player_hit.convolve(card_odds, before_hit, after_hit);
		player_final_state_probability_vector fin;
		reveal_strategy::player_final_state_probabilities(after_hit, fin);
		// static_assert
		assert(size_t(player_final_state_probability_vector::Size)
			== size_t(player_final_outcome_vector::Size));
		probability_type& p(player_actions[i].double_down());
		// perform inner_product, weighted expectations
		p = inner_product(fin.begin(), fin.end(), pse.begin(), 0.0);
		p *= var.double_multiplier;	// because bet is doubled
	} else {			// is terminal state, irrelevant
		player_actions[i].double_down() = -var.double_multiplier;
	}
	}
	const vector<state_type>& reorder(play_map::reverse_topo_order);

	state_type r;
	for (r=0; r<reorder.size(); ++r) {
		const state_type ii = reorder[r];
		const state_machine::node& ni(play.player_hit[ii]);
		assert(ni.size());
		expectations& p(player_actions[ii]);
		p.hit() = 0.0;
	card_type k;
	for (k=0; k<card_values; ++k) {
//		cout << "i=" << ii <<
//			", card-index " << k << ", odds=" << card_odds[k];
		if (play.player_hit[k].size()) {
			// is not a terminal state
			// compare hit vs. stand in action table
			// which is assumed to be precomputed
			const expectations& e(player_actions[ni[k]]);
			const edge_type best =
				(e.hit() > e.stand()) ? e.hit() : e.stand();
//			cout << ", best=" << best;
			p.hit() += card_odds[k] *best;
		} else {
			// is a terminal state
			const probability_type& edge(pse
				[play_map::player_final_state_map(ni[k])]);
//			cout << ", edge=" << edge;
			p.hit() += card_odds[k] *edge;
		}
//		cout << endl;
	}
		// p.optimize(surrender_penalty);
	}	// end for all states
// find optimal non-split decisions first, then back-patch
	optimize_actions(-var.surrender_penalty);
// create hit-only (non-split, non-double) player state machines
// compute table for calculating spread of player's final state, 
// given dealer's reveal card, and player's optimal hit strategy.
// again hit-stand only
	optimize_player_hit_tables(play, card_odds);
	compute_player_hit_stand_edges(var.surrender_penalty);
	// OK up to here
	reset_split_edges(play);	// zero-out
	const action_mask& dm(play.initial_actions_given_dealer[reveal_card]);
	const action_mask drm(dm & play.post_split_actions);
	const action_mask dfm(drm -SPLIT);	// final, no more split
	// the following computed edges are post-peek
#if IMPROVED_SPLIT_ANALYSIS
if (var.max_split_hands >= 2)
#else
if (var.split)
#endif
{
#if 0
	const bool DAS = var.double_after_split && var.some_double();
#define	RESPLIT_DP	DAS, var.resplit
#define	RESPLIT_DPR	RESPLIT_DP, false
#define	SPLIT_DP	DAS, false
#define	SPLIT_DPR	var.some_double(), true, var.surrender_late
	const action_mask rm(action_mask::dpr_tag(), RESPLIT_DPR);
	const action_mask sm(action_mask::dpr_tag(), SPLIT_DPR);
#define	RESPLIT_OPTION_ARG	rm
#define	SPLIT_OPTION_ARG	sm
#endif
// cannot surrender after splitting, but that would be interesting...
#if IMPROVED_SPLIT_ANALYSIS
if (var.max_split_hands > 2)
#else
if (var.resplit)
#endif
{
	DEBUG_SPLIT_PRINT(cout, "Resplitting allowed." << endl);
	DEBUG_SPLIT_PRINT(cout, "  nonsplit_edges 1" << endl);
	// need to work backwards from post-split edges
	compute_player_initial_nonsplit_edges(play, dfm);
		// resplit up to 4
	DEBUG_SPLIT_PRINT(cout, "  split_edges 1" << endl);
	compute_player_split_edges(play, card_odds, dfm);
		// resplit up to 4
	// iterate
	DEBUG_SPLIT_PRINT(cout, "  nonsplit_edges 2" << endl);
	compute_player_initial_nonsplit_edges(play, drm);
	DEBUG_SPLIT_PRINT(cout, "  split_edges 2" << endl);
	compute_player_split_edges(play, card_odds, drm);	// iterate
	DEBUG_SPLIT_PRINT(cout, "  nonsplit_edges 3" << endl);
	compute_player_initial_nonsplit_edges(play, dm);
} else {
	DEBUG_SPLIT_PRINT(cout, "Single split allowed." << endl);
	DEBUG_SPLIT_PRINT(cout, "  nonsplit_edges 1" << endl);
	compute_player_initial_nonsplit_edges(play, dfm);
	DEBUG_SPLIT_PRINT(cout, "  split_edges 1" << endl);
	compute_player_split_edges(play, card_odds, dfm);	// iterate
	DEBUG_SPLIT_PRINT(cout, "  nonsplit_edges 2" << endl);
	compute_player_initial_nonsplit_edges(play, dm);
}
} else {
	DEBUG_SPLIT_PRINT(cout, "No split allowed." << endl);
	// no splitting allowed!
	compute_player_initial_nonsplit_edges(play, dm);
}
	// to account for player's blackjack
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Consider splitting not an option.  
	NOTE: this uses only post-peek player_hit_stand_edges
	Rationale: edges for splitting cannot be worse
	than the weighted edges for hit/stand-only decisions.  
	TODO: factor of 2x for playing two hands?
 */
void
reveal_strategy::reset_split_edges(const play_map& play) {
	edge_type *player_split_edges =
		&player_initial_edges_post_peek[pair_offset];
	*player_split_edges =
		player_hit_stand_edges[play.p_initial_card_map[reveal_card]];
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
reveal_strategy::compute_player_split_edges(const play_map& play,
		const deck_distribution& card_odds,
		const action_mask& m) {
	const variation& var(play.var);
	const player_stand_edges_vector&
		pse(player_stand_edges_post_peek);
	edge_type *player_split_edges =
		&player_initial_edges_post_peek[pair_offset];
	const bool s = m.can_split();
//	const bool d = m.can_double_down();
	const state_machine& split_table(s ? play.player_resplit : play.last_split);
	card_type i;
	for (i=0; i<card_values; ++i) {
	// the single-card state after split
	const player_state_type p = pair_offset +i;
		// need to take weighted sum over initial states
		// depending on initial card and spread of next states.
		edge_type expected_edge = 0.0;
	// convolution: spread over the player's next card
	card_type k;
	for (k=0; k<card_values; ++k) {	// player's second card
		const probability_type& o(card_odds[k]);
		// initial edges may not have double-downs, splits, nor surrenders
		// since player is forced to take the next card in each hand
		const player_state_type state = split_table[i][k];
		edge_type edge = player_initial_edges_post_peek[state];
		// similar to expectation::best(d, i==k, r)
		// already accounted for player_resplit vs. final_split
		if ((i == ACE) && !var.hit_split_aces) {
			// each aces takes exactly one more card only
			edge = pse[play_map::player_final_state_map(state)];
//			cout << "1-card-ACE[D" << j << ",F" << state << "]: " << edge << endl;
		}
		expected_edge += o * edge;
	}	// inner product
		expectations& sum(player_actions[p]);
		sum.split() = 2.0 *expected_edge;	// b/c two hands are played
		sum.optimize(-var.surrender_penalty);
	}	// end for each player paired card

	// update the split entries for the initial-edges
	for (i=0; i<card_values; ++i) {
	// the single-card state after split
	const player_state_type p = pair_offset +i;
	const expectations& sum(player_actions[p]);
	// player may decide whether or not to split
	player_split_edges[i] =
		sum.value(sum.best(m & play.initial_actions_per_state[p]),
			-var.surrender_penalty);
	}
}	// end compute_player_split_edges()

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
strategy::dump_player_split_edges(ostream& o) const {
	// dump the player's initial state edges 
	o << "Player's split-permitted edges:" << endl;
	o << "P\\D";
	card_type j;
	for (j=0; j<card_values; ++j) {
		o << '\t' << card_name[j];
	}
	o << endl;
	for (j=0 ; j<card_values; ++j) {
		o << card_name[j] << ',' << card_name[j];
		card_type i;
		for (i=0; i<card_values; ++i) {
			const edge_type *player_split_edges =
				&reveal[i].player_initial_edges_post_peek[pair_offset];
			o << '\t' << player_split_edges[j];
		}
		o << endl;
	}
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Create player optimal hit state machines.
	Tables can be used to solve for distribution of expected
	player terminal states.  
	Decisions used to compute this table are *only* hit or stand, 
	no double-down or splits.
	Use this table to evaluate expected outcomes.  
	\pre player states are already optimized for stand,hit,double.
 */
void
reveal_strategy::optimize_player_hit_tables(const play_map& play, 
		const deck_distribution& card_odds) {
//	const card_type j = reveal_card;
	player_opt_hit = play.player_hit;	// copy entire state machine
	// using default assignment operator of util::array
	player_state_type i;
	for (i=0; i<p_action_states; ++i) {
		const expectations& e(player_actions[i]);
		// don't count surrender, split, double
		const player_choice b(e.best(STAND_HIT_ONLY));
		// or just compare e.hit vs. e.stand
		if (b == STAND) {
			// is either STAND or DOUBLE
			player_opt_hit[i].out_edges.clear();
			// make all stands terminal states
		}
	}
#if DUMP_DURING_EVALUATE
	cout << "Dealer shows " << card_name[reveal_card] <<
		", player hit state machine:" << endl;
	player_opt_hit.dump(cout) << endl;
#endif
	// now compute player_final_state_probability
	probability_vector unit(p_action_states, 0.0);
	probability_vector result;
	for (i=0; i<p_action_states; ++i) {
		unit[i] = 1.0;
		player_opt_hit.solve(card_odds, unit, result);
		player_final_state_probabilities(result,
			player_final_state_probability[i]);
		unit[i] = 0.0;	// reset
	}
}	// end optimize_player_hit_tables()

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
strategy::dump_player_hit_tables(ostream& o) const {
	card_type j;
	for (j=0; j<card_values; ++j) {
		// dump for debugging only
		o << "Dealer shows " << card_name[j] <<
			", player hit state machine:" << endl;
		reveal[j].player_opt_hit.dump(o) << endl;
	}
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
reveal_strategy::dump_player_final_state_probabilities(ostream& o) const {
	const card_type j = reveal_card;
	o << "Dealer shows " << card_name[j] <<
		", player\'s final state spread (hit/stand only):" << endl;
	play_map::player_final_table_header(o << "player") << endl;
	player_state_type i = 0;
	for (; i<p_action_states; ++i) {
		const player_final_state_probability_vector&
			v(player_final_state_probability[i]);
		o << player_opt_hit[i].name << '\t';
		copy(v.begin(), v.end(),
			ostream_iterator<probability_type>(o, "\t"));
		o << endl;
	}
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
strategy::dump_player_final_state_probabilities(ostream& o) const {
	card_type j;
	for (j=0; j<card_values; ++j) {
		reveal[j].dump_player_final_state_probabilities(o);
		o << endl;
	}
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
reveal_strategy::__compute_player_hit_stand_edges(
//	const card_type r,		// reveal_card
	const player_stand_edges_vector& edges, 
	const player_final_state_probability_matrix& fsp,
	const expectations_vector& actions,
	const edge_type& surr, 
	player_hit_stand_edges_vector& phe) {
	static const edge_type eps = 
		sqrt(std::numeric_limits<edge_type>::epsilon());
//	const card_type i = r;
#if 0
	// using post-peek conditions
	const player_stand_edges_matrix&
		edges(player_stand_edges_post_peek);
#endif
	player_state_type j;
	for (j=0; j<p_action_states; ++j) {
		// probability-weighted sum of edge values for each state
		const edge_type e = inner_product(edges.begin(), edges.end(), 
			fsp[j].begin(), 0.0);
		phe[j] = e;
		const expectations& x(actions[j]);
		// take the better edge between hit/stand
		// (no surr, double, split)
		const edge_type sh =
			x.value(x.best(STAND_HIT_ONLY), surr);
		const edge_type d = fabs(sh-e);
		// identity: sanity check for numerical noise
		if (d > eps) {
			cout << "p=" << j <<
//				", d=" << i <<
				", sh=" << sh << ", e=" << e <<
				", diff=" << d << endl;
			assert(d <= eps);
		}
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Hit edges should only be post-peek, when the player action
	(decision to hit at all vs. stand) is relevant.
	\param surr_pen surrender penalty.
 */
void
reveal_strategy::compute_player_hit_stand_edges(const double& surr_pen) {
	// hit-edges are post-peek, b/c using post-peek stand edges
	// pre-peek is meaningless for hit/stand
	__compute_player_hit_stand_edges(	// reveal_card, 
		player_stand_edges_post_peek, 
		player_final_state_probability, player_actions, 
		-surr_pen, player_hit_stand_edges);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
strategy::dump_player_hit_stand_edges(ostream& o) const {
	// dump the player's hitting/standing edges 
	o << "Player's hit/stand edges:\n";
	o << "P\\D";
	card_type j;
	for (j=0; j<card_values; ++j) {
		o << '\t' << card_name[j];
	}
	o << endl;
	for (j=0 ; j<p_action_states; ++j) {
		o << play.player_hit[j].name;
		card_type i;
		for (i=0; i<card_values; ++i) {
			o << '\t' << reveal[i].player_hit_stand_edges[j];
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
reveal_strategy::compute_player_initial_nonsplit_edges(
		const play_map& play,
		const action_mask& m)
{
//	const player_initial_edges_vector *player_split_edges =
//		&player_initial_edges_post_peek[p_action_states];
	const double& surr_pen(play.var.surrender_penalty);
	player_state_type i;
	for (i=0; i<pair_offset; ++i) {		// exclude splits first
		expectations c(player_actions[i]);	// yes, copy
		c.hit() = player_hit_stand_edges[i];
		c.optimize(-surr_pen);
		// since splits are folded into non-pair states
#if DEBUG_SPLIT
		const bool t = play.player_hit[i].is_terminal();
//			splits are computed in a separate section of the table
			const action_mask mm(m & play.initial_actions_per_state[i]);
			const action_mask mref(t ? action_mask::stand : m);
			if (mm != mref) {
				cout << "ERROR: in state " << i << ", mm != mref" << endl;
				cout << "\tgot: ";
				mm.dump_debug(cout) << " vs. ";
				mref.dump_debug(cout) << endl;
				play.initial_actions_per_state[i].dump_debug(
					cout << "\tinitial: ") << endl;
			}
#endif
		const pair<player_choice, player_choice>
			p(c.best_two(m & play.initial_actions_per_state[i]));
		const edge_type e = c.value(p.first, -surr_pen);
		DEBUG_SPLIT_PRINT(cout, "p.first=" << p.first << ", t=" << t
			<< ", i=" << i << ", e=" << e << endl);
		player_initial_edges_post_peek[i] = e;
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	TODO: pre-peek
 */
ostream&
strategy::dump_player_initial_edges(ostream& o) const {
{
#if 0
	// dump the player's initial state edges 
	o << "Player's initial edges (pre-peek):" << endl;
	o << "P\\D";
	card_type j;
	for (j=0; j<card_values; ++j) {
		o << '\t' << card_name[j];
	}
	o << endl;
	for (j=0 ; j<p_action_states; ++j) { // print split edges separately
		o << play.player_hit[j].name;
		card_type i;
		for (i=0; i<card_values; ++i) {
			o << '\t' << reveal[i].player_initial_edges_pre_peek[j];
		}
		o << endl;
	}
	o << endl;
}{
#endif
	// dump the player's initial state edges 
	o << "Player's initial edges (post-peek):" << endl;
	o << "P\\D";
	card_type j;
	for (j=0; j<card_values; ++j) {
		o << '\t' << card_name[j];
	}
	o << endl;
	for (j=0 ; j<p_action_states; ++j) { // print split edges separately
		o << play.player_hit[j].name;
		card_type i;
		for (i=0; i<card_values; ++i) {
			o << '\t' << reveal[i].player_initial_edges_post_peek[j];
		}
		o << endl;
	}
}
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Just sorts actions based on expected outcome to optimize player choice.  
 */
void
reveal_strategy::optimize_actions(const double& surr_pen) {
	player_state_type i;
	for (i=0; i<p_action_states; ++i) {
		player_actions[i].optimize(surr_pen);
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
reveal_strategy::compute_reveal_edges(const deck_distribution& card_odds, 
	const player_state_probability_vector& player_initial_state_odds) {
	edge_type& x(player_edge_given_reveal_post_peek);
	x = 0.0;
	player_state_type i;	// index of player's initial state
	// inner product
	for (i=0; i<p_action_states; ++i) {
		const probability_type& p(player_initial_state_odds[i]);
	if (p > 0.0) {
		const edge_type& e(player_initial_edges_post_peek[i]);
		x += p *e;
	}
	}
	// copy first, then correct for ACE, TEN revealed
	player_edge_given_reveal_pre_peek =
		player_edge_given_reveal_post_peek;
	// weigh by odds of blackjack
	const probability_type& pt(card_odds[TEN]);
	const probability_type& pa(card_odds[ACE]);
	const probability_type bjo = pt*pa;
	const probability_type bjc = 1.0 -bjo;
if (reveal_card == ACE) {
	player_edge_given_reveal_pre_peek =
		// dealer has blackjack, account for player blackjack
		-pt * bjc
		// dealer doesn't have blackjack, post-peek component
		+(1.0 -pt)*player_edge_given_reveal_post_peek;
} else if (reveal_card == TEN) {
	player_edge_given_reveal_pre_peek =
		// dealer has blackjack, account for player blackjack
		-pa * bjc
		// dealer doesn't have blackjack, post-peek component
		+(1.0 -pa)*player_edge_given_reveal_post_peek;
}
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
	typedef	member_select_iterator<reveal_array_type::const_iterator,
		const edge_type,
		&reveal_strategy::player_edge_given_reveal_pre_peek>
						const_iterator;
	const const_iterator i(reveal.begin()), e(reveal.end());
	copy(i, e, osi);
	o << endl;
}{
	o << "Player edges given dealer's revealed card (post-peek):\n";
	copy(card_name, card_name+TEN+1, ostream_iterator<char>(o, "\t"));
	o << endl;
	typedef	member_select_iterator<reveal_array_type::const_iterator,
		const edge_type,
		&reveal_strategy::player_edge_given_reveal_post_peek>
						const_iterator;
	const const_iterator i(reveal.begin()), e(reveal.end());
	copy(i, e, osi);
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
if (overall_need_update) {
	typedef	member_select_iterator<reveal_array_type::const_iterator,
		const edge_type,
		&reveal_strategy::player_edge_given_reveal_pre_peek>
						const_iterator;
	const const_iterator i(reveal.begin());
	_overall_edge =
		inner_product(card_odds.begin(), card_odds.end(), i, 0.0);
	overall_need_update = false;
}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	This could also be used to compute pre-peek initial edges,
	which is currently missing.
	\param p player's state
	\param d dealer's reveal card
 */
edge_type
strategy::lookup_pre_peek_initial_edge(const player_state_type p,
		const card_type d) const {
	const edge_type& post(reveal[d].player_initial_edges_post_peek[p]);
	switch (d) {
	// conditioned on probability of the hole card making 
	// a dealer natural (blackjack)
	case ACE: {
		const probability_type& pt(card_odds[TEN]);
		if (p == player_blackjack) {
			return var.bj_payoff * (1.0 -pt);
		} else {
			return pt*(post -1.0);
		}
	}
	case TEN: {
		const probability_type& pa(card_odds[ACE]);
		if (p == player_blackjack) {
			return var.bj_payoff * (1.0 -pa);
		} else {
			return pa*(post -1.0);
		}
	}
	default:
		break;
	}
	return post;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\param p player's state
	\param d dealer's reveal card
 */
const expectations&
strategy::lookup_player_action_expectations(
		const player_state_type p, const card_type d) const {
	return reveal[d].player_actions[p];
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Prints using the designated reveal_print_ordering for columns.
	Is a member function to access surrender_penalty.
 */
ostream&
strategy::dump_expectations(const player_state_type state, ostream& o) const {
	const action_mask& m(play.initial_actions_per_state[state]);
	dump_optimal_actions(state, o, 3, "\t");
#if 0
	typedef	member_select_iterator<reveal_array_type::const_iterator,
		const reveal_strategy::expectations_vector,
		&reveal_strategy::player_actions>
						const_iterator;
#else
	typedef	reveal_array_type::const_iterator	const_iterator;
#endif
	const const_iterator b(reveal.begin()), e(reveal.end());
	// need i->player_actions[state], std::accumulate
	expectations z(0,0,0,0);
	const_iterator i(b);
	for ( ; i!=e; ++i) {
		z += i->player_actions[state];
	}
	card_type j;
#define	EFORMAT(x)	setw(6) << x
#define	EXPECTATION_REF		const expectations& ex(reveal[reveal_print_ordering[j]].player_actions[state]);
// stand is always allowed
	o << "stand";
	for (j=0; j<card_values; ++j) {
		EXPECTATION_REF
		o << '\t' << EFORMAT(ex.stand());
		if (ex.best() == STAND) o << '*';
	}
	o << endl;
if (m.can_hit())
if (z.hit() > -1.0 * card_values) {
	o << "hit";
	for (j=0; j<card_values; ++j) {
		EXPECTATION_REF
		o << '\t' << EFORMAT(ex.hit());
		if (ex.best() == HIT) o << '*';
	}
	o << endl;
}
if (m.can_double_down())
if (z.double_down() > -2.0 *card_values) {
	// TODO: pass double_multiplier?
	o << "double";
	for (j=0; j<card_values; ++j) {
		EXPECTATION_REF
		o << '\t' << EFORMAT(ex.double_down());
		if (ex.best() == DOUBLE) o << '*';
	}
	o << endl;
}
// for brevity, could omit non-splittable states...
if (m.can_split())
if (z.split() > -2.0 *card_values) {
	o << "split";
	for (j=0; j<card_values; ++j) {
		EXPECTATION_REF
		o << '\t' << EFORMAT(ex.split());
		if (ex.best() == SPLIT) o << '*';
	}
	o << endl;
}
if (m.can_surrender())
{
	o << "surr.";
	for (j=0; j<card_values; ++j) {
		EXPECTATION_REF
//		o << '\t' << EFORMAT(expectations::surrender);
		o << '\t' << EFORMAT(-var.surrender_penalty);
		if (ex.best() == SURRENDER) o << '*';
	}
	o << endl;
}
#if 1
if (m.has_multiple_choice())
{
	o << "delta";
	for (j=0; j<card_values; ++j) {
		EXPECTATION_REF
		o << '\t';
		const pair<player_choice, player_choice>
			opt(ex.best_two(ALL_OPTIONS));
		o << EFORMAT(ex.margin(opt.first, opt.second,
			-var.surrender_penalty));
	}
	o << endl;
}
#endif

#undef	EFORMAT
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Print easier-to-use summary of optimal decision table.  
	\param n the top number of actions to print.
	TODO: doesn't make sense to show surrender/hit/stand for 
	terminal states, clean that up.
 */
ostream&
strategy::dump_optimal_actions(const player_state_type state,
		ostream& o, const size_t n, const char* delim) const {
	const action_mask& m(play.initial_actions_per_state[state]);
	assert(n<=5);
	card_type j;
	for (j=0; j<card_values; ++j) {
		EXPECTATION_REF
		o << delim;
		size_t k = 0;
		for ( ; k<n; ++k) {
			const player_choice a(ex.actions[k]);
			if (m.action_permitted(a))
				o << action_key[a];
		}
	}
	o << endl;
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
strategy::dump_optimal_edges(const player_state_type state, ostream& o) const {
	dump_optimal_actions(state, o, 3, "\t");
#define	EFORMAT(x)	setw(6) << x
	card_type j;
	o << "edge";
	for (j=0; j<card_values; ++j) {
		EXPECTATION_REF
		o << '\t';
		switch (ex.best()) {
		case STAND: o << EFORMAT(ex.stand()); break;
		case HIT: o << EFORMAT(ex.hit()); break;
		case DOUBLE: o << EFORMAT(ex.double_down()); break;
		case SPLIT: o << EFORMAT(ex.split()); break;
		case SURRENDER: o << EFORMAT(-var.surrender_penalty); break;
		default: break;
		}
	}
	o << endl;
#if 1
	o << "delta";	// difference between best and next best
	for (j=0; j<card_values; ++j) {
		EXPECTATION_REF
		o << '\t';
		const pair<player_choice, player_choice>
			opt(ex.best_two(ALL_OPTIONS));
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
	o << "Player's action expectations (edges):\n";
	// ostream_iterator<expectations> osi(o, "\t");
	o << "P\\D";
	card_type j;
	for (j=0; j<card_values; ++j) {
		o << '\t' << card_name[reveal_print_ordering[j]];
	}
	o << '\n' << endl;
	player_state_type i = 0;
	for ( ; i < p_action_states; ++i) {
		o << play.player_hit[i].name;
		dump_expectations(i, o);
		o << endl;
	}
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
strategy::dump_optimal_actions(ostream& o) const {
	o << "Player's optimal action:\n";
	// ostream_iterator<expectations> osi(o, "\t");
	static const char delim1[] = "   ";
	static const char delim2[] = "  ";
	o << "P\\D\t";
	card_type j;
	for (j=0; j<card_values; ++j) {
		o << delim1 << card_name[reveal_print_ordering[j]];
	}
	o << '\n' << endl;
//	const precision_saver p(o, 3);
	player_state_type i = 0;
	for ( ; i < p_action_states; ++i) {
		o << play.player_hit[i].name << "\t";
//		o << endl;
//		dump_optimal_edges(*i, o);	// still too much detail
		dump_optimal_actions(i, o, 2, delim2);
	}
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
strategy::dump(ostream& o) const {
	var.dump(o) << endl;
	play.dump_final_outcomes(o) << endl;	// verified
	play.dump_dealer_policy(o) << endl;		// verified
	play.dump_player_hit_state(o) << endl;		// verified
	play.dump_player_split_state(o) << endl;	// verified
	dump_dealer_final_table(o) << endl;		// verified
	dump_player_stand_odds(o) << endl;		// verified
	dump_player_hit_tables(o) << endl;
	dump_player_hit_stand_edges(o) << endl;
	dump_player_split_edges(o) << endl;

	dump_action_expectations(o);
	dump_player_initial_edges(o) << endl;
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
	strategy_command_registry::interpret(*this);
	return 0;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace strategy_commands {
#define DECLARE_STRATEGY_COMMAND_CLASS(class_name, _cmd, _brief)	\
	DECLARE_AND_INITIALIZE_COMMAND_CLASS(const strategy, class_name, _cmd, _brief)

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_STRATEGY_COMMAND_CLASS(Help, "help",
	"[cmd] : list strategy command(s)")
int
Help::main(const strategy&, const string_list& args) {
switch (args.size()) {
case 1:
	strategy_command_registry::list_commands(cout);
	break;
default: {
	string_list::const_iterator i(++args.begin()), e(args.end());
	for ( ; i!=e; ++i) {
		if (!strategy_command_registry::help_command(cout, *i)) {
			cout << "Command not found: " << *i << endl;
		} else {
			cout << endl;
		}
	}
}
}
	return CommandStatus::NORMAL;
}
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_STRATEGY_COMMAND_CLASS(Help2, "?",
	"[cmd] : list strategy command(s)")
int
Help2::main(const strategy& v, const string_list& args) {
	return Help::main(v, args);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_STRATEGY_COMMAND_CLASS(Quit, "quit", ": exit strategy menu")
int
Quit::main(const strategy&, const string_list&) {
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
Echo::main(const strategy&, const string_list& args) {
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
StandEdges::main(const strategy& g, const string_list&) {
	g.dump_player_stand_odds(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_STRATEGY_COMMAND_CLASS(InitialOdds, "initial-odds",
	": show prob. dist. of all initial hands")
int
InitialOdds::main(const strategy& g, const string_list&) {
	g.dump_player_initial_state_odds(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_STRATEGY_COMMAND_CLASS(InitialEdges, "initial-edges",
	": show all edges of initial hands")
int
InitialEdges::main(const strategy& g, const string_list&) {
	g.dump_player_initial_edges(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_STRATEGY_COMMAND_CLASS(HitEdges, "hit-edges",
	": show all hitting edges")
int
HitEdges::main(const strategy& g, const string_list&) {
	g.dump_player_hit_stand_edges(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_STRATEGY_COMMAND_CLASS(HitStrategy, "hit-strategy",
	": show optimal hit strategy vs. dealer")
int
HitStrategy::main(const strategy& g, const string_list&) {
// TODO: optional arg, given reveal
	g.dump_player_hit_tables(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_STRATEGY_COMMAND_CLASS(SplitEdges, "split-edges",
	": show all splitting edges")
int
SplitEdges::main(const strategy& g, const string_list&) {
	g.dump_player_split_edges(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_STRATEGY_COMMAND_CLASS(PlayerFinalOdds, "player-final-state-odds",
	": show prob. dist. of player's final state vs. dealer")
int
PlayerFinalOdds::main(const strategy& g, const string_list&) {
// TODO: optional arg, given reveal
	g.dump_player_final_state_probabilities(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_STRATEGY_COMMAND_CLASS(RevealEdges, "reveal-edges",
	": show initial edges pre/post-peek")
int
RevealEdges::main(const strategy& g, const string_list&) {
	g.dump_reveal_edges(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_STRATEGY_COMMAND_CLASS(OverallEdge, "overall-edge",
	": show pre-deal overall player's edge")
int
OverallEdge::main(const strategy& g, const string_list&) {
	cout << "overall edge: " << g.overall_edge() << endl;
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_STRATEGY_COMMAND_CLASS(ActionEdges, "action-edges",
	": show complete edges/action table vs. dealer")
int
ActionEdges::main(const strategy& g, const string_list&) {
	g.dump_action_expectations(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_STRATEGY_COMMAND_CLASS(ActionOptimal, "action-optimal",
	": show optimal action table vs. dealer")
int
ActionOptimal::main(const strategy& g, const string_list&) {
	g.dump_optimal_actions(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_STRATEGY_COMMAND_CLASS(AllInfo, "all-info",
	": show all edge calculations")
int
AllInfo::main(const strategy& g, const string_list&) {
	g.dump(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace strategy_commands

//=============================================================================
}	// end namespace blackjack

