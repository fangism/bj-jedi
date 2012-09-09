/**
	\file "analysis.hh"
	Probability analysis routines.
	$Id: $
 */

#ifndef	__BOC2_ANALYSIS_HH__
#define	__BOC2_ANALYSIS_HH__

#include <map>
#include "outcome.hh"
#include "hand.hh"
#include "deck_state.hh"

namespace blackjack {
class variation;
class play_map;

/**
	Enumeration representing analysis mode.
 */
enum analysis_mode {
	/**
		Basic strategy, assumes a standard, static card distribution,
		analysis assumes an infinite-deck or finite deck with 
		replacement.
		Analysis is fast and approximate.
	 */
	ANALYSIS_BASIC,
	/**
		Dynamic strategy, assumes the current card distribution,
		with replacement (so infinite in the same proportion).
		Analysis is moderately fast, more accurate.
	 */
	ANALYSIS_DYNAMIC,
	/**
		Dynamic strategy, assuming current card distribution,
		without replacement (finite, and removing cards).
		Analysis is slow, but *exact*.
	 */
	ANALYSIS_EXACT
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Bundle of analysis parameters.
 */
struct analysis_parameters {
	/**
		Analysis mode, depending on accuracy/speed.
	 */
	analysis_mode				mode;
	/**
		The cumulative probability to reach this state.
		This probability is used to dynamically switch modes.
		The least probable sub-trees can get away with
		lesser accuracy.
	 */
	probability_type			parent_prob;
	/**
		The threshold at which to switch from exact to
		dynamic analysis strategy.
	 */
	probability_type			dynamic_threshold;
//	probability_type			basic_threshold;

	/**
		Downgrade one level of analysis accuracy when
		probability falls below threshold.
	 */
	analysis_mode
	get_auto_mode(void) const {
		if (parent_prob < dynamic_threshold) {
			if (mode == ANALYSIS_EXACT)
				return ANALYSIS_DYNAMIC;
			else if (mode == ANALYSIS_DYNAMIC)
				return ANALYSIS_BASIC;
		}
		return mode;
	}
};	// end struct analysis_parameters

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Lookup key for cached outcome vector results.  
 */
struct dealer_situation_key_type {
	dealer_hand_base			dealer;
	perceived_deck_state			card_dist;

	dealer_situation_key_type(const size_t ds,
			const perceived_deck_state& d) :
		dealer(ds), card_dist(d) { }

	int
	compare(const dealer_situation_key_type& r) const {
		const int dc = dealer.compare(r.dealer);
		if (dc) return dc;
		return card_dist.compare(r.card_dist);
	}

	bool
	operator < (const dealer_situation_key_type& r) const {
		return compare(r) < 0;
	}

	ostream&
	dump(ostream&) const;

};	// end struct dealer_situation_key_type

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
typedef	std::map<dealer_situation_key_type, dealer_final_vector>
			dealer_outcome_cache_map_type;
typedef	std::map<dealer_hand_base, dealer_final_vector>
			dealer_outcome_basic_cache_map_type;

/**
	Collection of outcome vector caches.
	One for each accuracy setting.
 */
class dealer_outcome_cache_set {
	typedef	dealer_outcome_cache_map_type		map_type;
	typedef	dealer_outcome_basic_cache_map_type	basic_map_type;
	// mutable
	map_type				exact_cache;
	map_type				dynamic_cache;
	basic_map_type				basic_cache;

public:
	const dealer_final_vector&
	compute_dealer_final_distribution(
		const play_map&,	// for dealer H17 vs. S17
		const dealer_situation_key_type&,
		const analysis_parameters&);

	ostream&
	dump(ostream&) const;

private:
	const dealer_final_vector&
	__evaluate_exact(const play_map&,
		const dealer_situation_key_type&,
		const analysis_parameters&);

	const dealer_final_vector&
	__evaluate_dynamic(const play_map&,
		const dealer_situation_key_type&,
		const analysis_parameters&);

	const dealer_final_vector&
	__evaluate_basic(const play_map&,
		const dealer_hand_base&);

};	// end struct dealer_outcome_cache_set

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BOC2_ANALYSIS_HH__
