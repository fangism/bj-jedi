/**
	\file "bj/core/analysis.hh"
	Probability analysis routines.
	$Id: $
 */

#ifndef	__BJ_CORE_ANALYSIS_HH__
#define	__BJ_CORE_ANALYSIS_HH__

#include <iosfwd>
#include <map>
#include "bj/core/outcome.hh"
#include "bj/core/hand.hh"
#include "bj/core/deck_state.hh"
#include "bj/core/expectations.hh"

namespace blackjack {
using std::ostream;
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

extern
ostream&
operator << (ostream& o, const analysis_mode);

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

	ostream&
	dump(ostream&) const;

};	// end struct analysis_parameters

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Lookup key for cached outcome vector results.  
 */
struct dealer_situation_key_type {
	dealer_hand_base			dealer;
	perceived_deck_state			card_dist;

	dealer_situation_key_type(const dealer_state_type ds,
			const perceived_deck_state& d) :
		dealer(ds), card_dist(d) { }

	dealer_situation_key_type(const dealer_hand_base& dh,
			const perceived_deck_state& d) :
		dealer(dh), card_dist(d) { }

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

	const dealer_final_vector&
	evaluate_dealer_basic(const play_map&,
		const dealer_hand_base&);

};	// end struct dealer_outcome_cache_set

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Just a pair.
 */
struct player_situation_base {
	player_hand_base			hand;
	split_state				splits;

	player_situation_base(const player_hand_base& h, 
		const split_state& s) : hand(h), splits(s) { }

	ostream&
	dump(ostream&) const;

	int
	compare(const player_situation_base& r) const {
		const int sc = splits.compare(r.splits);
		if (sc) return sc;
		return hand.compare(r.hand);
	}

	bool
	operator < (const player_situation_base& r) const {
		return compare(r) < 0;
	}

};	// end struct player_situation_base

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Lookup key for cached outcome vector results.  
 */
struct player_situation_key_type : public dealer_situation_key_type {
	typedef	dealer_situation_key_type	parent_type;
	player_situation_base			player;

	player_situation_key_type(const player_hand_base& ph,
			const split_state& ss,
			const dealer_hand_base& dh,
			const perceived_deck_state& d) :
		parent_type(dh, d), player(ph, ss) { }

	int
	compare(const player_situation_key_type& r) const {
		const int pc = player.compare(r.player);
		if (pc) return pc;
		return parent_type::compare(r);
	}

	bool
	operator < (const player_situation_key_type& r) const {
		return compare(r) < 0;
	}

	ostream&
	dump(ostream&, const play_map&) const;

};	// end struct player_situation_key_type

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	This key type excludes the deck_count information, 
	and is used for basic strategy.
 */
struct player_situation_basic_key_type {
	player_situation_base			player;
	dealer_hand_base			dealer;

	player_situation_basic_key_type(const player_situation_base& p, 
		const dealer_hand_base& d) : player(p), dealer(d) { }

	int
	compare(const player_situation_basic_key_type& r) const {
		const int pc = player.compare(r.player);
		if (pc) return pc;
		return dealer.compare(r.dealer);
	}

	bool
	operator < (const player_situation_basic_key_type& r) const {
		return compare(r) < 0;
	}

	ostream&
	dump(ostream&, const play_map&) const;

};	// end struct player_situation_basic_key_type

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/// TODO: there's a lot of redundant information computed, due to
/// overlap in the action_masks, should take advantage of it somehow
/// is it correct to always compute for all actions, even illegal ones?

typedef	std::map<player_situation_key_type, expectations>
			player_outcome_cache_map_type;
typedef	std::map<player_situation_basic_key_type, expectations>
			player_outcome_basic_cache_map_type;

/**
	Collection of outcome vector caches.
	One for each accuracy setting.
 */
class player_outcome_cache_set {
	typedef	player_outcome_cache_map_type		map_type;
	typedef	player_outcome_basic_cache_map_type	basic_map_type;
	// mutable
	map_type				exact_cache;
	map_type				dynamic_cache;
	basic_map_type				basic_cache;
	/**
		The player's strategy is coupled tightly to
		the dealer's final outcome spread (requires),
		so this class includes the dealer's computations.
	 */
	dealer_outcome_cache_set		dealer_outcome_cache;
public:
	ostream&
	dump(ostream&) const;

	// this dispatches one of the following evaluate methods
	const expectations&
	compute_player_expectation(
		const play_map&,	// for player H17 vs. S17
		const player_situation_key_type&,
		const analysis_parameters&);

	const expectations&
	evaluate_player_exact(const play_map&,
		const player_situation_key_type&,
		const analysis_parameters&);

	const expectations&
	evaluate_player_dynamic(const play_map&,
		const player_situation_key_type&,
		const analysis_parameters&);

	const expectations&
	evaluate_player_basic(const play_map&,
		const player_situation_basic_key_type&);

};	// end struct player_outcome_cache_set

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BJ_CORE_ANALYSIS_HH__
