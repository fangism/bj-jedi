/**
	\file "bj/core/analysis/player_analysis.cc"
	Implementation of edge analysis classes.  
	Includes variants of edge analysis: basic, dynamic, exact.
	$Id: $
 */

#include <iostream>
#include <numeric>

#define	ENABLE_STACKTRACE				0

#include "bj/core/analysis/player_analysis.hh"
#include "bj/core/blackjack.hh"
#include "util/array.tcc"
#include "util/stacktrace.hh"
#include "util/value_saver.hh"

/**
	An optimization to calculation to avoid recomputing
	the same double-down repeatedly.
	Status: tested, works
 */
#define	EVALUATE_TERMINAL_ACTIONS_FIRST			1

namespace blackjack {
using std::pair;
using std::accumulate;
using std::endl;
using cards::TEN;
using cards::ACE;
using cards::state_machine;
using cards::card_name;
using util::value_saver;

// initial default vector of all 0s
// static const dealer_final_vector dealer_final_null(0.0);
static const expectations null_expectations;

//=============================================================================
// class player_situation_key_type method definitions

ostream&
player_situation_key_type::dump(ostream& o, const play_map& p) const {
	player.hand.dump(o << "player: ", p.player_hit);
	dealer.dump(o << ", dealer: ", p.dealer_hit);
	card_dist.show_count_brief(o << ", deck:");
	return o;
}

//=============================================================================
// class player_situation_basic_key_type method definitions

ostream&
player_situation_basic_key_type::dump(ostream& o, const play_map& p) const {
	player.hand.dump(o << "player: ", p.player_hit);
	if (player.hand.is_paired()) {
		player.splits.dump_code(o << " [") << "]";
	}
	dealer.dump(o << ", dealer: ", p.dealer_hit);
	return o;
}

//=============================================================================
// basic_strategy_analyzer method definitions

basic_strategy_analyzer::basic_strategy_analyzer() :
		basic_cache(), dealer_cache() {
	// initialize back-reference pointers
	card_type i = 0;
	for ( ; i<card_values; ++i) {
		split_cache[i].split_card = i;
		split_cache[i]._basic_analyzer = this;
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
edge_type
basic_strategy_analyzer::__evaluate_stand(const play_map& play, 
		const player_situation_basic_key_type& k,
		const perceived_deck_state& pd) {
	// STACKTRACE("stand:");
	// stand is always a legal option
	// (busted hands considered as stand)
	// compute just the odds of standing with this hand
	const dealer_final_vector&
		dfv(dealer_cache.evaluate(play, k.dealer, pd));
	const player_state_type p_final =
		play.player_final_state_map(k.player.hand.state);
	outcome_odds soo;
	play.compute_player_final_outcome(p_final, dfv, soo);
	const edge_type ret = soo.edge();
	STACKTRACE_INDENT_PRINT("stand: edge(stand): "
		<< ret << endl);
	return ret;
}	// end __evaluate_stand

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
edge_type
basic_strategy_analyzer::__evaluate_double_down(const play_map& play, 
		const player_situation_basic_key_type& k, 
		const perceived_deck_state& pd) {
	STACKTRACE("double-down:");
	deck_count_type dref;
	pd.distribution_weight_adjustment(k.dealer.peek_state, dref);
	player_situation_basic_key_type nk(k);	// copy-and-modify state
	action_mask& am(nk.player.hand.player_options);
	const count_type total_weight =
		accumulate(dref.begin(), dref.end(), 0);
//	const value_saver<action_mask> ams(am);
	am &= play.post_double_down_actions;
	edge_type ret = 0.0;
	card_type i = 0;
	for ( ; i<card_values; ++i) {
		const count_type& w(dref[i]);
	if (w) {
		STACKTRACE_INDENT_PRINT("card: " << card_name[i] <<
			", weight: " << w << endl);
		const value_saver<player_hand_base> __bs(nk.player.hand);
		nk.player.hand.hit(play, i);
		// yes, do recursion, in case of non-terminal variations
		// don't bother accounting for car removal for basic analysis
		const expectations& child(evaluate(play, nk, pd));
#if ENABLE_STACKTRACE
		child.dump_choice_actions_1(STACKTRACE_INDENT_PRINT("child: "));
		am.dump_debug(STACKTRACE_INDENT_PRINT("options: ")) << endl;
#endif
		// weight by card probability
		const edge_type e(child.action_edge(child.best(am)));
		STACKTRACE_INDENT_PRINT("child.best: " << e << endl);
		ret += e *w;
	}
		// else 0 weight, leave as vector of 0s
	}	// end for all next card possibilities
	// normalize weighted sum of probabilities
	ret *= play.var.double_multiplier / total_weight;
	// triple-down??
	STACKTRACE_INDENT_PRINT("edge(double): " << ret << endl);
	return ret;
}	// end __evaluate_double_down

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
edge_type
basic_strategy_analyzer::__evaluate_hit(const play_map& play, 
		const player_situation_basic_key_type& k, 
		const perceived_deck_state& pd) {
	STACKTRACE("hit:");
	deck_count_type dref;
	pd.distribution_weight_adjustment(k.dealer.peek_state, dref);
	player_situation_basic_key_type nk(k);	// copy-and-modify state
	action_mask& am(nk.player.hand.player_options);
	const count_type total_weight =
		accumulate(dref.begin(), dref.end(), 0);
//	const value_saver<action_mask> ams(am);
	am &= play.post_hit_actions;
	edge_type ret = 0.0;
	card_type i = 0;
	for ( ; i<card_values; ++i) {
		const count_type& w(dref[i]);
	if (w) {
		const value_saver<player_hand_base> __bs(nk.player.hand);
		STACKTRACE_INDENT_PRINT("+card: " << cards::card_name[i] << endl);
		nk.player.hand.hit(play, i);
		const expectations& child(evaluate(play, nk, pd));
		// weight by card probability
		const edge_type e(child.action_edge(child.best(am)));
		STACKTRACE_INDENT_PRINT("child.best = " << e <<
			", w = " << w << endl);
		ret += e *w;
	}
		// else 0 weight, leave as vector of 0s
	}	// end for all next card possibilities
	// normalize weighted sum of probabilities
	ret /= total_weight;
	STACKTRACE_INDENT_PRINT("edge(hit): " << ret << endl);
	return ret;
}	// end __evaluate_hit

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Re-compute edges of actions in given situation.
	This applies to a single hand only.
	This is an uncached computation.
	\param ret is overwritten and updated (return)
 */
void
basic_strategy_analyzer::__evaluate_player_basic_single(const play_map& play,
		const player_situation_basic_key_type& k, 
		const perceived_deck_state& pd,
		expectations& ret) {
	STACKTRACE_BRIEF;
#if ENABLE_STACKTRACE
	k.dump(STACKTRACE_INDENT_PRINT(""), play) << endl;
#endif
#if EVALUATE_TERMINAL_ACTIONS_FIRST
	// always consider terminal actions first, mask those out
	const action_mask nonterminal_actions =
		k.player.hand.player_options - play.terminal_actions;
	const bool nonterminal_eval = nonterminal_actions;	// any left?
	const action_mask local_compute_actions =
		(nonterminal_eval ? nonterminal_actions : play.terminal_actions);
	STACKTRACE_INDENT_PRINT(
		(nonterminal_eval ? "non-" : "") << "terminal evaluation" << endl);
	if (nonterminal_eval) {
		STACKTRACE("re-calling evaluate on terminal actions");
		// calculate terminal actions once
		player_situation_basic_key_type tk(k);
		tk.player.hand.player_options = play.terminal_actions;
		ret = evaluate(play, tk, pd);;
		// even illegal actions are included
		// caller's responsibility to evaluate only legal actions
	}
#else
	const action_mask& local_compute_actions(action_mask::all);
#endif
	// here, dealer's progression is assumed independent from player actions
	// current state
	const state_machine::node& ps(play.player_hit[k.player.hand.state]);
	if (local_compute_actions.can_stand()) {
		ret.stand() = __evaluate_stand(play, k, pd);
	}
	if (!ps.is_terminal()) {	// i.e. not blackjack or bust
		// consider all possible actions, even illegal ones
		// let caller pick among the legal options
		// TODO: don't forget to adjust action_mask
#if 0
		const deck_count_type&
			dref(get_basic_reduced_count(k.dealer.peek_state));
		deck_count_type dref;
		pd.distribution_weight_adjustment(k.dealer.peek_state, dref);
#endif
#if 0
		const count_type total_weight =
			accumulate(dref.begin(), dref.end(), 0);
		STACKTRACE_INDENT_PRINT("total_weight: " << total_weight << endl);
#endif
	if (local_compute_actions.can_double_down()) {
		ret.double_down() = __evaluate_double_down(play, k, pd);
	}
	if (local_compute_actions.can_hit()) {
		ret.hit() = __evaluate_hit(play, k, pd);
	}
	// can we just skip evaluation if split is not permitted?
	if (k.player.hand.is_paired() && local_compute_actions.can_split()) {
		STACKTRACE("split:");
		const player_situation_basic_key_type& nk(k);	// alias
//		player_situation_basic_key_type nk(k);	// copy-and-modify state
//		action_mask& am(nk.player.hand.player_options);
//		ret.split() = __evaluate_split_basic(play, nk);
		ret.split() = split_cache[k.player.hand.pair_card()]
			.evaluate(play, nk, pd);
		// inside: deduct splits remaining, evaluate cases of second cards
	}	// is paired, can split
	}	// is terminal, else don't bother computing
	// surrender (if allowed) is constant expectation
	ret.optimize(-play.var.surrender_penalty);	// sort
#if ENABLE_STACKTRACE
	k.dump(STACKTRACE_INDENT_PRINT(""), play) << endl;
	ret.dump_choice_actions_1(STACKTRACE_INDENT_PRINT("options: "));
#endif
}	// end __evaluate_player_basic_single

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Evaluate basic strategy odds of the dealer's final state, 
	based solely on the standard card distribution (w/ replacement).
	This computation is cached.
 */
const expectations&
basic_strategy_analyzer::evaluate(const play_map& play,
		const player_situation_basic_key_type& k, 
		const perceived_deck_state& pd) {
	STACKTRACE_BRIEF;
	// if multiple hands, if single hand
#if ENABLE_STACKTRACE
	const action_mask& m(k.player.hand.player_options);
	k.dump(STACKTRACE_INDENT_PRINT(""), play) << endl;
	STACKTRACE_INDENT_PRINT("options: " << m.raw() << endl);
#endif
	typedef	basic_map_type::value_type		pair_type;
#if EVALUATE_TERMINAL_ACTIONS_FIRST
	// don't use maximal set of actions
	const pair_type probe(k, null_expectations);
#else
	const player_hand_base ak(k.player.hand.state, play);
	const player_situation_base psb(ak, k.player.splits);
	// TODO: do lookup with maximal action options,
	// then caller should filter out the disallowed choices
	// should result in better cache locality
	const pair_type probe(
		player_situation_basic_key_type(psb, k.dealer),
		null_expectations);
#endif
	const pair<basic_map_type::iterator, bool>
		bi(basic_cache.insert(probe));
	expectations& ret(bi.first->second);
if (bi.second) {
	STACKTRACE_INDENT_PRINT("cache miss (basic), calculating\n");
	// cache-miss: then this is a newly inserted entry, compute it
	// in basic analysis mode, dealer's final outcome spread
	// is independent of card distribution and player action,
	// thus we can look it up once.
	__evaluate_player_basic_single(play, k, pd, ret);
	// surrender (if allowed) is constant expectation
	ret.optimize(-play.var.surrender_penalty);	// sort
} else {
	STACKTRACE_INDENT_PRINT("cache hit (basic)\n");
}
#if ENABLE_STACKTRACE
	k.dump(STACKTRACE_INDENT_PRINT(""), play) << endl;
	ret.dump_choice_actions_1(STACKTRACE_INDENT_PRINT("options: "));
#endif
	// else return cached entry
	return ret;
}	// end basic_strategy_analyzer::evaluate

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const expectations&
basic_strategy_analyzer::evaluate(const play_map& play,
		const player_situation_basic_key_type& k) {
	// number of decks doesn't matter, modeling with replacement
	static const perceived_deck_state pd(1);
	return evaluate(play, k, pd);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if 0
/**
	Routine for recursively analyzing split scenarios.
	Splits are handled differently, using split_states.
	\param k situation key, must be a single-card state, post-split, 
		*prior* to taking any other cards.
		For example: A,A -> A.
	\return edge of splitting in this state (optimal)
	TODO: account for post-split action limitations.
 */
edge_type
player_outcome_cache_set::__evaluate_split_basic(const play_map& play, 
		const player_situation_basic_key_type& k) {
	STACKTRACE_VERBOSE;
	assert(k.player.hand.is_paired());
	player_situation_basic_key_type nk(k);	// copy-and-modify state
	action_mask& am(nk.player.hand.player_options);
#if ENABLE_STACKTRACE
	k.player.splits.dump_code(
		STACKTRACE_INDENT_PRINT("split code: ")) << endl;
#endif
	const value_saver<action_mask> ams(am);
	const card_type pc = k.player.hand.pair_card();
	// divide this into cases, based on post-split-state
	const deck_count_type&
		dref(get_basic_reduced_count(k.dealer.peek_state));
	const count_type total_weight =
		accumulate(dref.begin(), dref.end(), 0);
	const count_type& pw(dref[pc]);		// pairing cards
	// here, using weights that account for pair-card removal
	// instead of p^2, q^2, 2pq
	const count_type npw = total_weight -pw;	// non-pairing
	const count_type pwt = total_weight *(total_weight -1);
	count_type pwa[3];	// should be const
	pwa[2] = pw *(pw -1);	// 2 new pairs
	pwa[0] = npw *(npw -1);	// 0 new pairs
	pwa[1] = pwt -pwa[2] -pwa[0];	// 1 new pair
	split_state ps[3];
	split_state& pss(nk.player.splits);
	pss.post_split_states(ps[2], ps[1], ps[0]);
	am &= play.post_split_actions;
	edge_type ret = 0.0;
if (pss.is_splittable()) {
	STACKTRACE_INDENT_PRINT("has splittable pair(s)" << endl);
	// weighted sum expected value of 3 sub cases
	size_t j = 0;
	for ( ; j<3; ++j) {
		STACKTRACE("iterate over three cases");
		STACKTRACE_INDENT_PRINT("for j = " << j << endl);
		const value_saver<split_state> __ss(pss, ps[j]);
		const expectations child(evaluate_player_basic(play, nk));
//		const expectations child(__evaluate_split_basic(play, nk));
		const player_choice b(child.best(am));
		const edge_type e(child.action_edge(b));
		ret += e *(pwa[j]);
	}
	ret /= pwt;
} else {
	STACKTRACE_INDENT_PRINT("no more splittable pairs" << endl);
	// now accumulate over Xs and Ys
	am -= SPLIT;
	// splitting: hit card on top of base card
	// assume that order of Xs and Ys are independent
	// compute non-pairing edges (Y)
	edge_type X = 0.0;	// possible pair card next
	edge_type Y = 0.0;	// non-paired only
	// loop over possible second cards
	card_type i = 0;
	for ( ; i<card_values; ++i) {
		const count_type& w(dref[i]);
	if (w) {
		const value_saver<player_hand_base> __bs(nk.player.hand);
		nk.player.hand.final_split(play, i);
		nk.player.splits.initialize_default();
		const expectations
			child(evaluate_player_basic(play, nk));
		// weight by card probability
		const edge_type e(child.action_edge(child.best(am)));
		const edge_type sub = e *w;
		if (i != pc) {
			Y += sub;
		}
		X += sub;
	}
	}
	Y /= npw;
	X /= total_weight;
	// TODO: cache X and Y, this is a basic approximation
	size_t j = 0;
	for ( ; j<3; ++j) {
		const edge_type nX = X *ps[j].nonsplit_pairs();
		const edge_type nY = Y *ps[j].unpaired_hands;
		ret += pwa[j] *(nX +nY);
	}
	ret /= pwt;
}	// splittable pairs remaining
	return ret;
}	// end __evaluate_split_basic
#endif

//=============================================================================
// class player_split_basic_cache_type method definitions

/**
	Key down-slicing, stripping out player_state.
 */
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
basic_split_situation_key_type::basic_split_situation_key_type(
		const player_situation_basic_key_type& k) :
		splits(k.player.splits),
		// omit player.hand.state
		actions(k.player.hand.player_options),
		dealer(k.dealer) {
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Re-construct basic key from split key with card.
 */
player_situation_basic_key_type::player_situation_basic_key_type(
		const basic_split_situation_key_type& k, 
		const card_type c) :
		player(player_situation_base(
			player_hand_base(
				play_map::paired_state(c),
				k.actions),
			split_state(k.splits))), 
		dealer(k.dealer) {
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Compute the odds of case terminal hands XX and XY, where
	XX is non-splittable pair
	XY is an unpaired hand (Y != X)
	This should be done up front once.  
	\param k is the player state of a non-splittable pair XX.
 */
void
player_split_basic_cache_type::update_nonsplit_cache_outcome(
		const play_map& play, 
		const player_situation_basic_key_type& k, 
		const perceived_deck_state& pd) {
	STACKTRACE_VERBOSE;
	assert(_basic_analyzer);	// must already be set
	assert(k.player.hand.is_paired());
	assert(split_card == k.player.hand.pair_card());
if (!nonsplit_exp_valid) {
#if 0
	const deck_count_type&
		dref(get_basic_reduced_count(k.dealer.peek_state));
#else
	deck_count_type dref;
	pd.distribution_weight_adjustment(k.dealer.peek_state, dref);
#endif
//	const count_type& pw(dref[split_card]);		// pairing cards
	// here, using weights that account for pair-card removal
	edge_type& X(nonsplit_pair_exp);	// possible pair card next
	edge_type& Y(unpaired_exp);		// non-paired only
	player_situation_basic_key_type nk(k);		// copy
	action_mask& am(nk.player.hand.player_options);
	am -= SPLIT;			// terminal, never consider split
//	rm &= post_split_actions;
	count_type npw = 0;
	card_type i = 0;
	for ( ; i<card_values; ++i) {
		const count_type& w(dref[i]);
		const value_saver<player_hand_base> __bs(nk.player.hand);
		nk.player.hand.final_split(play, i);
		nk.player.splits.initialize_default();
		const expectations&
			child(_basic_analyzer->evaluate(play, nk, pd));
		const edge_type e(child.action_edge(child.best(am)));
		const edge_type sub = e *w;
		if (i != split_card) {
			Y += sub;
			npw += w;
		} else {
			X = e;
		}
	}	// end for
	Y /= npw;
	nonsplit_exp_valid = true;
}
}	// end update_nonsplit_cache_outcome

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Automatically converts basic_key_type to split_key_type.
 */
const edge_type&
player_split_basic_cache_type::evaluate(
		const play_map& play,
		const player_situation_basic_key_type& k, 
		const perceived_deck_state& pd) {
	assert(split_card == k.player.hand.pair_card());
	const key_type kk(k);
	return evaluate(play, kk, pd);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	TODO: move this into basic_strategy_analyzer
	This evaluates the expectation of a split action 
	from a paired state.
	computation:
	if there are splittable pairs P, do (recursion)
	else no more splittable pairs, 
		weighted sum over pair and unpaired hands
	Need permissible action mask.
 */
const edge_type&
player_split_basic_cache_type::evaluate(const play_map& play,
		const key_type& k, const perceived_deck_state& pd) {
	STACKTRACE("player_split_basic_cache_type::evaluate");
	typedef	map_type::iterator		iterator;
	const pair<iterator, bool>
		p(_map.insert(std::make_pair(k, value_type(0.0))));
	value_type& e(p.first->second);
if (p.second) {
		// was cache miss, compute
//	const player_state_type pst = play_map::paired_state(split_card);
#if ENABLE_STACKTRACE
	k.splits.dump_code(
		STACKTRACE_INDENT_PRINT("split code: ")) << endl;
#endif
	// divide this into cases, based on post-split-state
#if 0
	const deck_count_type&
		dref(get_basic_reduced_count(k.dealer.peek_state));
#else
	deck_count_type dref;
	pd.distribution_weight_adjustment(k.dealer.peek_state, dref);
	STACKTRACE_INDENT_PRINT(dref << endl);
#endif
	const count_type total_weight =
		accumulate(dref.begin(), dref.end(), 0);
	STACKTRACE_INDENT_PRINT("total_weight = " << total_weight << endl);
	const count_type& pw(dref[split_card]);		// pairing cards
	// here, using weights that account for pair-card removal
	// instead of p^2, q^2, 2pq
	const count_type npw = total_weight -pw;	// non-pairing
	const count_type pwt = total_weight *(total_weight -1);
	STACKTRACE_INDENT_PRINT("pw = " << pw << endl);
	STACKTRACE_INDENT_PRINT("npw = " << npw << endl);
	STACKTRACE_INDENT_PRINT("pwt = " << pwt << endl);
	count_type pwa[3];	// should be const
	pwa[2] = pw *(pw -1);	// 2 new pairs
	pwa[0] = npw *(npw -1);	// 0 new pairs
	pwa[1] = pwt -pwa[2] -pwa[0];	// 1 new pair
	STACKTRACE_INDENT_PRINT("pwa[] = {" << pwa[0] << ", "
		<< pwa[1] << ", " << pwa[2] << "}" << endl);
	assert(pwa[0] +pwa[1] +pwa[2] == pwt);
	split_state ps[3];
	const split_state& pss(k.splits);
	pss.post_split_states(ps[2], ps[1], ps[0]);
	e = 0.0;
if (pss.is_splittable()) {
	STACKTRACE_INDENT_PRINT("has splittable pair(s)" << endl);
	// weighted sum expected value of 3 sub cases
	key_type nk(k);		// copy
	size_t j = 0;
	do {
		STACKTRACE("iterate over three cases");
		STACKTRACE_INDENT_PRINT("for j = " << j << endl);
		const value_saver<split_state> __ss(nk.splits, ps[j]);
		const edge_type exp = evaluate(play, nk, pd);
		STACKTRACE_INDENT_PRINT("exp = " << exp << endl);
		STACKTRACE_INDENT_PRINT("pwa[j] = " << pwa[j] << endl);
		e += exp *(pwa[j]);
		++j;
	} while (j<2);
	e /= pwt;
	// weighted average
} else {
	STACKTRACE_INDENT_PRINT("no more splittable pairs" << endl);
	// now accumulate over Xs and Ys
	// splitting: hit card on top of base card
	if (!nonsplit_exp_valid) {
		player_situation_basic_key_type nk(k, split_card);
		nk.player.hand.player_options -= SPLIT;
		update_nonsplit_cache_outcome(play, nk, pd);
	}
	// assume that order of Xs and Ys are independent, weighted sum
	e = nonsplit_pair_exp *k.splits.nonsplit_pairs()
		+unpaired_exp *k.splits.unpaired_hands;
}	// splittable pairs remaining
}	// was cache hit, re-use
	STACKTRACE_INDENT_PRINT("edge = " << e << endl);
	return e;
}

//=============================================================================
}	// end namespace blackjack

