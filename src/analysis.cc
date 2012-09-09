/**
	\file "analysis.cc"
	$Id: $
 */

#include <iostream>
#include <numeric>

#include "analysis.hh"
#include "blackjack.hh"
#include "util/array.tcc"
#include "util/stacktrace.hh"

namespace blackjack {
using std::pair;
using std::accumulate;
using cards::TEN;
using cards::ACE;

// initial default vector of all 0s
static const dealer_final_vector dealer_final_null(0.0);

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
		return __evaluate_exact(play, k, p);
	case ANALYSIS_DYNAMIC:
		return __evaluate_dynamic(play, k, p);
	case ANALYSIS_BASIC:
		return __evaluate_basic(play, k.dealer);
	}
	// unreachable
	return __evaluate_basic(play, k.dealer);
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
dealer_outcome_cache_set::__evaluate_basic(const play_map& play,
		const dealer_hand_base& k) {
	typedef	basic_map_type::value_type		pair_type;
	const pair_type probe(k, dealer_final_null);
	const pair<basic_map_type::iterator, bool>
		bi(basic_cache.insert(probe));
	dealer_final_vector& ret(bi.first->second);
if (bi.second) {
	// cache-miss: then this is a newly inserted entry, compute it
	// is this a hit or stand state?
	const cards::state_machine::node& ds(play.dealer_hit[k.state]);
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
			dealer_final_vector child(__evaluate_basic(play, nk));
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
}
	// else return cached entry
	return ret;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Evaluate basic strategy odds of the dealer's final state, 
	based solely on the standard card distribution (w/ replacement).
 */
const dealer_final_vector&
dealer_outcome_cache_set::__evaluate_dynamic(const play_map& play,
		const dealer_situation_key_type& k, 
		const analysis_parameters& p) {
	typedef	map_type::value_type		pair_type;
	const pair_type probe(k, dealer_final_null);
	const pair<map_type::iterator, bool>
		bi(dynamic_cache.insert(probe));
	dealer_final_vector& ret(bi.first->second);
if (bi.second) {
	// cache-miss: then this is a newly inserted entry, compute it
	// is this a hit or stand state?
	const cards::state_machine::node& ds(play.dealer_hit[k.dealer.state]);
	if (ds.is_terminal()) {
		// dealer must stand
		const size_t final = k.dealer.state -stop;
		ret[final] = 1.0;
	} else {
		// dealer must hit
		// what distribution to use? (any peek?)
		perceived_deck_state d(k.card_dist);
		switch (k.dealer.peek_state) {
		case PEEKED_NO_10: d.remove_if_any(TEN); break;
		case PEEKED_NO_ACE: d.remove_if_any(ACE); break;
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
			psub.parent_prob *=
				probability_type(w)/probability_type(total_weight);
			const dealer_situation_key_type
				nk(ds[i], perceived_deck_state(k.card_dist, i));	// NO_PEEK
			dealer_final_vector child(__evaluate_dynamic(play, nk, psub));
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
}
	// else return cached entry
	return ret;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//=============================================================================
}	// end namespace blackjack

