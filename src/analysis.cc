/**
	\file "analysis.cc"
	$Id: $
 */

#include <iostream>
#include <numeric>

#define	ENABLE_STACKTRACE				1

#include "analysis.hh"
#include "blackjack.hh"
#include "util/array.tcc"
#include "util/stacktrace.hh"
#include "util/value_saver.hh"

namespace blackjack {
using std::pair;
using std::accumulate;
using std::endl;
using cards::TEN;
using cards::ACE;
using cards::state_machine;
using util::value_saver;

// initial default vector of all 0s
static const dealer_final_vector dealer_final_null(0.0);
static const expectations null_expectations;

//=============================================================================
ostream&
operator << (ostream& o, const analysis_mode mode) {
	switch (mode) {
	case ANALYSIS_EXACT: o << "exact"; break;
	case ANALYSIS_DYNAMIC: o << "dynam"; break;
	case ANALYSIS_BASIC: o << "basic"; break;
	}
	return o;
}

//=============================================================================
// class analysis_parameters method definitions

ostream&
analysis_parameters::dump(ostream& o) const {
	return o << mode << ", " <<
		parent_prob << '/' << dynamic_threshold;
}

//=============================================================================
// class dealer_situation_key_type method definitions

ostream&
dealer_situation_key_type::dump(ostream& o, const play_map& p) const {
	dealer.dump(o, p.dealer_hit);
	return card_dist.show_count_brief(o);
}

//=============================================================================
// class dealer_outcome_cache_set method definitions

const dealer_final_vector&
dealer_outcome_cache_set::compute_dealer_final_distribution(
		const play_map& play,
		const dealer_situation_key_type& k,
		const analysis_parameters& p) {
	const analysis_mode m = p.get_auto_mode();
	switch (m) {
	case ANALYSIS_EXACT:
		return evaluate_dealer_exact(play, k, p);
	case ANALYSIS_DYNAMIC:
		return evaluate_dealer_dynamic(play, k, p);
	default: break;
	}
//	case ANALYSIS_BASIC:
	return evaluate_dealer_basic(play, k.dealer);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static
const deck_count_type&
get_basic_reduced_count(const peek_state_enum e) {
	switch (e) {
	case PEEKED_NO_10:
		return cards::standard_deck_count_reduced_no_10;
	case PEEKED_NO_ACE:
		return cards::standard_deck_count_reduced_no_Ace;
	default: break;
	}
//	case NO_PEEK:
	return cards::standard_deck_count_reduced;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Evaluate basic strategy odds of the dealer's final state, 
	based solely on the standard card distribution (w/ replacement).
 */
const dealer_final_vector&
dealer_outcome_cache_set::evaluate_dealer_basic(const play_map& play,
		const dealer_hand_base& k) {
	STACKTRACE_BRIEF;
#if ENABLE_STACKTRACE
	k.dump(STACKTRACE_STREAM, play.dealer_hit) << endl;
#endif
	typedef	basic_map_type::value_type		pair_type;
	const pair_type probe(k, dealer_final_null);
	const pair<basic_map_type::iterator, bool>
		bi(basic_cache.insert(probe));
	dealer_final_vector& ret(bi.first->second);
if (bi.second) {
	STACKTRACE_INDENT_PRINT("cache miss, calculating\n");
	// cache-miss: then this is a newly inserted entry, compute it
	// is this a hit or stand state?
	const state_machine::node& ds(play.dealer_hit[k.state]);
	if (ds.is_terminal()) {
		// dealer must stand
		const size_t final = k.state -stop;
		ret[final] = 1.0;
	} else {
		// dealer must hit
		// what distribution to use? (any peek?)
		const deck_count_type&
			dref(get_basic_reduced_count(k.peek_state));
		const size_t total_weight =
			accumulate(dref.begin(), dref.end(), 0);
		size_t i = 0;
		for ( ; i<card_values; ++i) {
			const size_t& w(dref[i]);
		if (w) {
			dealer_hand_base nk(ds[i]);	// NO_PEEK
			const dealer_final_vector
				child(evaluate_dealer_basic(play, nk));
			// weight by card probability
			size_t j = 0;
			for ( ; j<d_final_states; ++j) {
				ret[j] += child[j] * w;
			}
		}
			// else 0 weight, leave as vector of 0s
		}
		// normalize weighted sum of probabilities
		for (i=0; i< d_final_states; ++i) {
			ret[i] /= total_weight;
		}
	}
} else {
	STACKTRACE_INDENT_PRINT("cache hit\n");
}
#if ENABLE_STACKTRACE
	dump_dealer_final_vector(STACKTRACE_STREAM, ret) << endl;
#endif
	// else return cached entry
	return ret;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Evaluate basic strategy odds of the dealer's final state, 
	based solely on the current card distribution (w/ replacement).
 */
const dealer_final_vector&
dealer_outcome_cache_set::evaluate_dealer_dynamic(const play_map& play,
		const dealer_situation_key_type& k, 
		const analysis_parameters& p) {
	STACKTRACE_BRIEF;
#if ENABLE_STACKTRACE
	k.dump(STACKTRACE_STREAM, play) << endl;
#endif
	typedef	map_type::value_type		pair_type;
	const pair_type probe(k, dealer_final_null);
	const pair<map_type::iterator, bool>
		bi(dynamic_cache.insert(probe));
	dealer_final_vector& ret(bi.first->second);
if (bi.second) {
	// cache-miss: then this is a newly inserted entry, compute it
	// is this a hit or stand state?
	const state_machine::node& ds(play.dealer_hit[k.dealer.state]);
	if (ds.is_terminal()) {
		// dealer must stand
		const size_t final = k.dealer.state -stop;
		ret[final] = 1.0;
	} else {
		// dealer must hit
		// what distribution to use? (any peek?)
		perceived_deck_state d(k.card_dist);
		switch (k.dealer.peek_state) {
		case PEEKED_NO_10: d.remove_all(TEN); break;
		case PEEKED_NO_ACE: d.remove_all(ACE); break;
		default: break;
		}
		// effective weights, given peeked not-10, not-Ace
		deck_count_type wd;
		d.distribution_weight_adjustment(wd);
		const size_t total_weight = accumulate(wd.begin(), wd.end(), 0);
		size_t i = 0;
		for ( ; i<card_values; ++i) {
			const size_t& w(wd[i]);
		if (w) {
			analysis_parameters psub(p);
			// branch probability diminishes
			psub.parent_prob *= probability_type(w)
				/ probability_type(total_weight);
			// do not remove card, keep same dist
			const dealer_situation_key_type
				nk(ds[i], k.card_dist);
			// NO_PEEK
			const dealer_final_vector
				child(compute_dealer_final_distribution(play, nk, psub));
//				child(evaluate_dealer_dynamic(play, nk, psub));
			// weight by card probability
			size_t j = 0;
			for ( ; j<d_final_states; ++j) {
				ret[j] += child[j] * w;
			}
		}
			// else 0 weight, leave as vector of 0s
		}
		// normalize weighted sum of probabilities
		for (i=0; i< d_final_states; ++i) {
			ret[i] /= total_weight;
		}
	}
} else {
	STACKTRACE_INDENT_PRINT("cache hit\n");
}
#if ENABLE_STACKTRACE
	dump_dealer_final_vector(STACKTRACE_STREAM, ret) << endl;
#endif
	// else return cached entry
	return ret;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Evaluate basic strategy odds of the dealer's final state, 
	based on the current card distribution (w/o replacement).
 */
const dealer_final_vector&
dealer_outcome_cache_set::evaluate_dealer_exact(const play_map& play,
		const dealer_situation_key_type& k, 
		const analysis_parameters& p) {
	STACKTRACE_BRIEF;
#if ENABLE_STACKTRACE
	k.dump(STACKTRACE_STREAM, play) << endl;
#endif
	typedef	map_type::value_type		pair_type;
	const pair_type probe(k, dealer_final_null);
	const pair<map_type::iterator, bool>
		bi(exact_cache.insert(probe));
	dealer_final_vector& ret(bi.first->second);
if (bi.second) {
	// cache-miss: then this is a newly inserted entry, compute it
	// is this a hit or stand state?
	const state_machine::node& ds(play.dealer_hit[k.dealer.state]);
	if (ds.is_terminal()) {
		// dealer must stand
		const size_t final = k.dealer.state -stop;
		ret[final] = 1.0;
	} else {
		// dealer must hit
		// what distribution to use? (any peek?)
		perceived_deck_state d(k.card_dist);
		switch (k.dealer.peek_state) {
		case PEEKED_NO_10: d.remove_all(TEN); break;
		case PEEKED_NO_ACE: d.remove_all(ACE); break;
		default: break;
		}
		// effective weights, given peeked not-10, not-Ace
		deck_count_type wd;
		d.distribution_weight_adjustment(wd);
		const size_t total_weight = accumulate(wd.begin(), wd.end(), 0);
		size_t i = 0;
		for ( ; i<card_values; ++i) {
			const size_t& w(wd[i]);
		if (w) {
			analysis_parameters psub(p);
			// branch probability diminishes
			psub.parent_prob *= probability_type(w)
				/ probability_type(total_weight);
			// remove single card, change dist
			const dealer_situation_key_type
				nk(ds[i], perceived_deck_state(k.card_dist, i));
			// NO_PEEK
			const dealer_final_vector
				child(compute_dealer_final_distribution(play, nk, psub));
//				child(evaluate_dealer_exact(play, nk, psub));
			// weight by card probability
			size_t j = 0;
			for ( ; j<d_final_states; ++j) {
				ret[j] += child[j] * w;
			}
		}
			// else 0 weight, leave as vector of 0s
		}
		// normalize weighted sum of probabilities
		for (i=0; i< d_final_states; ++i) {
			ret[i] /= total_weight;
		}
	}
} else {
	STACKTRACE_INDENT_PRINT("cache hit\n");
}
#if ENABLE_STACKTRACE
	dump_dealer_final_vector(STACKTRACE_STREAM, ret) << endl;
#endif
	// else return cached entry
	return ret;
}

//=============================================================================
// class player_situation_key_type method definitions

ostream&
player_situation_key_type::dump(ostream& o, const play_map& p) const {
	player.hand.dump(o, p.player_hit);
	return card_dist.show_count_brief(o);
}

//=============================================================================
// class player_situation_basic_key_type method definitions

ostream&
player_situation_basic_key_type::dump(ostream& o, const play_map& p) const {
	player.hand.dump(o, p.player_hit);
	dealer.dump(o, p.dealer_hit);
	return o;
}

//=============================================================================
/**
	Evaluate basic strategy odds of the dealer's final state, 
	based solely on the standard card distribution (w/ replacement).
 */
const expectations&
player_outcome_cache_set::evaluate_player_basic(const play_map& play,
		const player_situation_basic_key_type& k) {
	STACKTRACE_BRIEF;
#if ENABLE_STACKTRACE
	k.dump(STACKTRACE_STREAM, play) << endl;
#endif
	typedef	basic_map_type::value_type		pair_type;
	const player_hand_base ak(k.player.hand.state, play);
	const player_situation_base psb(ak, k.player.splits);
	const pair_type probe(
		player_situation_basic_key_type(psb, k.dealer),
		null_expectations);
	const pair<basic_map_type::iterator, bool>
		bi(basic_cache.insert(probe));
	expectations& ret(bi.first->second);
if (bi.second) {
	STACKTRACE_INDENT_PRINT("cache miss, calculating\n");
	// cache-miss: then this is a newly inserted entry, compute it
	// in basic analysis mode, dealer's final outcome spread
	// is independent of card distribution and player action,
	// thus we can look it up once.
	const dealer_final_vector&
		dfv(dealer_outcome_cache.evaluate_dealer_basic(play, k.dealer));
	// current state
	const state_machine::node& ps(play.player_hit[k.player.hand.state]);
	const action_mask& m(k.player.hand.player_options);
	if (m.can_stand()) {
		// stand is always a legal option
		// (busted hands considered as stand)
		// compute just the odds of standing with this hand
		const size_t p_final =
			play.player_final_state_map(k.player.hand.state);
		outcome_odds soo;
		play.compute_player_final_outcome(p_final, dfv, soo);
		ret.stand() = soo.edge();
	}
	if (!ps.is_terminal()) {
		// consider all possible actions, even illegal ones
		// let caller pick among the legal options
		// TODO: don't forget to adjust action_mask
		const deck_count_type&
			dref(get_basic_reduced_count(k.dealer.peek_state));
		const size_t total_weight =
			accumulate(dref.begin(), dref.end(), 0);
		player_situation_basic_key_type nk(k);	// copy-and-modify state
	if (m.can_double_down()) {
		nk.player.hand.player_options &= play.post_double_down_actions;
		size_t i = 0;
		for ( ; i<card_values; ++i) {
			const size_t& w(dref[i]);
		if (w) {
			const value_saver<player_hand_base> __bs(nk.player.hand);
			nk.player.hand.hit(play, i);
			// yes, actually do recursion, in case of variations
			const expectations child(evaluate_player_basic(play, nk));
			// weight by card probability
			ret.double_down() += child.best(nk.player.hand.player_options) *w;
		}
			// else 0 weight, leave as vector of 0s
		}	// end for all next card possibilities
		// normalize weighted sum of probabilities
		ret.double_down() *= play.var.double_multiplier / total_weight;
	}
	if (m.can_hit()) {
		nk.player.hand.player_options &= play.post_double_down_actions;
		size_t i = 0;
		for ( ; i<card_values; ++i) {
			const size_t& w(dref[i]);
		if (w) {
			const value_saver<player_hand_base> __bs(nk.player.hand);
			nk.player.hand.hit(play, i);
			const expectations child(evaluate_player_basic(play, nk));
			// weight by card probability
			ret.hit() += child.best(nk.player.hand.player_options) *w;
		}
			// else 0 weight, leave as vector of 0s
		}	// end for all next card possibilities
		// normalize weighted sum of probabilities
		ret.hit() /= total_weight;
	}
	if (m.can_split() && k.player.hand.is_paired()) {
		const size_t pc = k.player.hand.pair_card();
		// divide this into cases, based on post-split-state
		const size_t& pw(dref[pc]);		// pairing cards
		// here, using weights that account for pair-card removal
		// instead of p^2, q^2, 2pq
		const size_t npw = total_weight -pw;	// non-pairing cards
		const size_t pwt = npw *(npw -1);
		size_t pwa[3];	// should be const
		pwa[2] = pw *(pw -1);	// 2 new pairs
		pwa[0] = npw *(npw -1);	// 0 new pairs
		pwa[1] = pwt -pwa[2] -pwa[0];	// 1 new pair
		split_state ps[3];
		k.player.splits.post_split_states(ps[2], ps[1], ps[0]);
		player_situation_basic_key_type sk(k);
		sk.player.hand.player_options &= play.post_split_actions;
	if (k.player.splits.is_splittable()) {
		// weighted sum expected value of 3 sub cases
		size_t j = 0;
		for ( ; j<3; ++j) {
			const value_saver<split_state> __ss(sk.player.splits, ps[j]);
			const expectations child(evaluate_player_basic(play, sk));
			ret.split() += child.best(sk.player.hand.player_options) *pwa[j];
		}
		ret.split() /= pwt;
	} else {
		// no more splittable pairs
		sk.player.hand.player_options -= SPLIT;
		// splitting: hit card on top of base card
		// assume that order of Xs and Ys are independent
		// compute non-pairing edges (Y)
		edge_type X = 0.0;	// possible pair card next
		edge_type Y = 0.0;	// non-paired only
		size_t i = 0;
		for ( ; i<card_values; ++i) {
			const size_t& w(dref[i]);
		if (w) {
			const value_saver<player_hand_base> __bs(sk.player.hand);
			sk.player.hand.split(play, i);
			const expectations
				child(evaluate_player_basic(play, sk));
			// weight by card probability
			const edge_type sub = child.best(sk.player.hand.player_options) *w;
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
			ret.split() += pwa[j] *(nX +nY);
		}
		ret.split() /= pwt;
	}	// splittable pairs remaining
	}	// is paired, can split
	}	// is terminal, else don't bother computing
	// surrender (if allowed) is constant expectation
} else {
	STACKTRACE_INDENT_PRINT("cache hit\n");
}
	// else return cached entry
	return ret;
}

//=============================================================================
}	// end namespace blackjack

