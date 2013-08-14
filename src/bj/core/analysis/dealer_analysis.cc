/**
	\file "bj/core/analysis/dealer_analysis.cc"
	Implementation of edge analysis classes.  
	Includes variants of edge analysis: basic, dynamic, exact.
	$Id: $
 */

#include <iostream>
#include <numeric>

#define	ENABLE_STACKTRACE				0

#include "bj/core/analysis/dealer_analysis.hh"
#include "bj/core/blackjack.hh"
#include "util/array.tcc"
#include "util/stacktrace.hh"
#include "util/value_saver.hh"

#define	DEBUG_DEALER_ANALYSIS			(0 && ENABLE_STACKTRACE)

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
static const dealer_final_vector dealer_final_null(0.0);

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
#if 0
	// no longer connected to basic outcome cache
	case ANALYSIS_BASIC:
	return evaluate_dealer_basic(play, k.dealer);
#else
	return evaluate_dealer_dynamic(play, k, p);
#endif
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if 0
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
#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Evaluate basic strategy odds of the dealer's final state, 
	based solely on the standard card distribution (w/ replacement).
 */
const dealer_final_vector&
basic_dealer_outcome_cache_set::evaluate(const play_map& play,
		const key_type& k, const perceived_deck_state& pd) {
#if DEBUG_DEALER_ANALYSIS
	STACKTRACE_BRIEF;
#if ENABLE_STACKTRACE
	k.dump(STACKTRACE_INDENT_PRINT("dealer: "), play.dealer_hit) << endl;
#endif
#endif
	typedef	basic_map_type::value_type		pair_type;
	const pair_type probe(k, dealer_final_null);
	const pair<basic_map_type::iterator, bool>
		bi(basic_cache.insert(probe));
	dealer_final_vector& ret(bi.first->second);
if (bi.second) {
#if DEBUG_DEALER_ANALYSIS
	STACKTRACE_INDENT_PRINT("cache miss, calculating\n");
#endif
	// cache-miss: then this is a newly inserted entry, compute it
	// is this a hit or stand state?
	const state_machine::node& ds(play.dealer_hit[k.state]);
	if (ds.is_terminal()) {
		// dealer must stand
		const dealer_state_type final = k.state -stop;
		ret[final] = 1.0;
	} else {
		// dealer must hit
		// what distribution to use? (any peek?)
#if 0
		const deck_count_type&
			dref(get_basic_reduced_count(k.peek_state));
#else
		deck_count_type dref;
		pd.distribution_weight_adjustment(k.peek_state, dref);
//		STACKTRACE_INDENT_PRINT(dref << endl);
#endif
		const count_type total_weight =
			accumulate(dref.begin(), dref.end(), 0);
		card_type i = 0;
		for ( ; i<card_values; ++i) {
#if 0
			// when accounting for card removal
			perceived_deck_state npd(pd);
			npd.reveal(k.peek_state, i);
#else
			// don't bother removing cards for basic analysis
			const perceived_deck_state& npd(pd);	// re-use
#endif
			const count_type& w(dref[i]);
		if (w) {
			dealer_hand_base nk(ds[i]);	// NO_PEEK
			nk.check_blackjack(k.first_card);
			const dealer_final_vector& child(evaluate(play, nk, npd));
			// weight by card probability
			dealer_state_type j = 0;
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
#if DEBUG_DEALER_ANALYSIS
	STACKTRACE_INDENT_PRINT("cache hit\n");
#endif
}
#if DEBUG_DEALER_ANALYSIS
	dump_dealer_final_vector(STACKTRACE_STREAM << '\t', ret, false) << endl;
#endif
	// else return cached entry
	return ret;
}	// end basic_dealer_outcome_cache_set::evaluate()

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const dealer_final_vector&
basic_dealer_outcome_cache_set::evaluate(const play_map& play,
		const key_type& k) {
	static const perceived_deck_state pd(1);
	return evaluate(play, k, pd);
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
	k.dump(STACKTRACE_INDENT_PRINT("dealer: "), play) << endl;
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
		const dealer_state_type final = k.dealer.state -stop;
		ret[final] = 1.0;
	} else {
		// dealer must hit
		// what distribution to use? (any peek?)
		const perceived_deck_state& d(k.card_dist);
		// effective weights, given peeked not-10, not-Ace
		deck_count_type wd;
		d.distribution_weight_adjustment(k.dealer.peek_state, wd);
#if ENABLE_STACKTRACE && 0
		d.show_count_brief(STACKTRACE_STREAM);
		STACKTRACE_STREAM << wd << endl;
#endif
		const count_type total_weight =
			accumulate(wd.begin(), wd.end(), 0);
		perceived_deck_state subd(d);
		subd.reveal_replace(k.dealer.peek_state);
		card_type i = 0;
		for ( ; i<card_values; ++i) {
			const count_type& w(wd[i]);
		if (w) {
			analysis_parameters psub(p);
			// branch probability diminishes
			psub.parent_prob *= probability_type(w)
				/ probability_type(total_weight);
			// do not remove card, keep same dist
			dealer_situation_key_type nk(ds[i], subd);
			nk.dealer.check_blackjack(k.dealer.first_card);
			// NO_PEEK
			const dealer_final_vector
//				child(compute_dealer_final_distribution(play, nk, psub));
				child(evaluate_dealer_dynamic(play, nk, psub));
			STACKTRACE_INDENT_PRINT("weighted " << w << '/'
				<< total_weight << endl);
			// weight by card probability
			dealer_state_type j = 0;
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
	dump_dealer_final_vector(STACKTRACE_STREAM << '\t', ret, false) << endl;
#endif
	// else return cached entry
	return ret;
}	// end evaluate_dealer_dynamic

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
	k.dump(STACKTRACE_INDENT_PRINT("dealer: "), play) << endl;
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
		const dealer_state_type final = k.dealer.state -stop;
		ret[final] = 1.0;
	} else {
		// dealer must hit
		// what distribution to use? (any peek?)
		const perceived_deck_state& d(k.card_dist);
		// effective weights, given peeked not-10, not-Ace
		deck_count_type wd;
		d.distribution_weight_adjustment(k.dealer.peek_state, wd);
		const count_type total_weight =
			accumulate(wd.begin(), wd.end(), 0);
		perceived_deck_state subd(d);
		subd.reveal_replace(k.dealer.peek_state);
		card_type i = 0;
		for ( ; i<card_values; ++i) {
			const count_type& w(wd[i]);
		if (w) {
			analysis_parameters psub(p);
			// branch probability diminishes
			psub.parent_prob *= probability_type(w)
				/ probability_type(total_weight);
			// remove single card, change dist
			dealer_situation_key_type
				nk(ds[i], perceived_deck_state(subd, i));
			nk.dealer.check_blackjack(k.dealer.first_card);
			// NO_PEEK
			const dealer_final_vector
				child(compute_dealer_final_distribution(play, nk, psub));
//				child(evaluate_dealer_exact(play, nk, psub));
			// weight by card probability
			dealer_state_type j = 0;
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
	dump_dealer_final_vector(STACKTRACE_STREAM << '\t', ret, false) << endl;
#endif
	// else return cached entry
	return ret;
}	// end evaluate_dealer_exact

//=============================================================================
}	// end namespace blackjack

