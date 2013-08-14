/**
	\file "bj/core/analysis/dealer_analysis.hh"
	Probability analysis routines.
	$Id: $
 */

#ifndef	__BJ_CORE_DEALER_ANALYSIS_HH__
#define	__BJ_CORE_DEALER_ANALYSIS_HH__

#include <map>
#include "bj/core/outcome.hh"
#include "bj/core/hand.hh"
#include "bj/core/deck_state.hh"
#include "bj/core/expectations.hh"
#include "bj/core/analysis.hh"

namespace blackjack {
using std::ostream;
class play_map;
#if 0
class player_outcome_cache_set;
class basic_split_situation_key_type;
class basic_strategy_analyzer;
#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Lookup key for cached outcome vector results.  
 */
struct dealer_situation_key_type {
	dealer_hand_base			dealer;
	perceived_deck_state			card_dist;

	dealer_situation_key_type() : dealer(), card_dist() { }

	dealer_situation_key_type(const dealer_state_type ds,
			const perceived_deck_state& d) :
		dealer(ds), card_dist(d) { }

	dealer_situation_key_type(const dealer_hand_base& dh,
			const perceived_deck_state& d) :
		dealer(dh), card_dist(d) { }

	void
	peek_no_Ace(void) {
		dealer.peek_no_Ace();
		card_dist.peek_not_Ace();
	}

	void
	peek_no_10(void) {
		dealer.peek_no_10();
		card_dist.peek_not_10();
	}

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
	dump(ostream&, const play_map&) const;

};	// end struct dealer_situation_key_type

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
typedef	std::map<dealer_situation_key_type, dealer_final_vector>
			dealer_outcome_cache_map_type;

/**
	This cache is really only valid for one given variation
	(play_map).
	When play_map is altered, this cache (and all others)
	should be invalidated.
 */
class basic_dealer_outcome_cache_set {
public:
	typedef	dealer_hand_base		key_type;
	typedef	dealer_final_vector		value_type;
private:
	typedef	std::map<key_type, value_type>
				dealer_outcome_basic_cache_map_type;
	typedef	dealer_outcome_basic_cache_map_type	basic_map_type;
	// basic map could be replaced with a table...
	basic_map_type				basic_cache;

public:
	ostream&
	dump(ostream&) const;

	void
	invalidate(void) { basic_cache.clear(); }

	const value_type&
	evaluate(const play_map&, const key_type&, const perceived_deck_state&);

	const value_type&
	evaluate(const play_map&, const key_type&);

};	// end class basic_dealer_outcome_cache_set

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Collection of outcome vector caches.
	One for each accuracy setting.
 */
class dealer_outcome_cache_set {
	typedef	dealer_outcome_cache_map_type		map_type;
	// mutable
	map_type				exact_cache;
	map_type				dynamic_cache;

public:
	ostream&
	dump(ostream&) const;

	// this dispatches one of the following evaluate methods
	const dealer_final_vector&
	compute_dealer_final_distribution(
		const play_map&,	// for dealer H17 vs. S17
		const dealer_situation_key_type&,
		const analysis_parameters&);

	const dealer_final_vector&
	evaluate_dealer_exact(const play_map&,
		const dealer_situation_key_type&,
		const analysis_parameters&);

	const dealer_final_vector&
	evaluate_dealer_dynamic(const play_map&,
		const dealer_situation_key_type&,
		const analysis_parameters&);


};	// end struct dealer_outcome_cache_set

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BJ_CORE_DEALER_ANALYSIS_HH__
