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
class player_outcome_cache_set;
class basic_split_situation_key_type;
class basic_strategy_analyzer;

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
	TODO: use independent settings for dealer calculation
	and player calculation.
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
#if 0
	/**
		Control the verbosity of the analysis.
	 */
	bool					verbose;
#endif

	analysis_parameters() : mode(ANALYSIS_BASIC),
		parent_prob(1.0),
		dynamic_threshold(1e-4) { }

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
/**
	Just a pair.
 */
struct player_situation_base {
	player_hand_base			hand;
	split_state				splits;

	player_situation_base() : hand(), splits() { }

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

	player_situation_key_type() : dealer_situation_key_type(), player() { }

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

	// explicit -- drops the card_dist from the key
	player_situation_basic_key_type(const player_situation_key_type& s) :
		player(s.player), dealer(s.dealer) { }

	player_situation_basic_key_type(const basic_split_situation_key_type&, 
		const card_type);

	// default copy-ctor

	int
	compare(const player_situation_basic_key_type& r) const {
		const int pc = player.compare(r.player);
		if (pc) return pc;
		const int dc = dealer.compare(r.dealer);
		return dc;
	}

	bool
	operator < (const player_situation_basic_key_type& r) const {
		return compare(r) < 0;
	}

	ostream&
	dump(ostream&, const play_map&) const;

};	// end struct player_situation_basic_key_type

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	A split situation is uniquely identified by the 
	composition of multiple hands (or single paired hand)
	and the permissible actions.
	Almost the same as player_situation_basic_key_type,
	but without player.hand.state.
	Alternatively, could just derive from player_situation_basic_key_type
	and just *not* compare player.hand.state.
 */
struct basic_split_situation_key_type {
	/// state of (possibly) multiple paired hands
	split_state				splits;
	/// permissible actions
	action_mask				actions;
	/// dealer's reveal
	dealer_hand_base			dealer;

	basic_split_situation_key_type(const split_state& s, 
		const action_mask& m, const dealer_hand_base& d) :
		splits(s), actions(m), dealer(d) { }

	explicit
	basic_split_situation_key_type(const player_situation_basic_key_type&);

	int
	compare(const basic_split_situation_key_type& r) const {
		const int sc = splits.compare(r.splits);
		if (sc) return sc;
		const int ac = actions.compare(r.actions);
		if (ac) return ac;
		const int dc = dealer.compare(r.dealer);
		return dc;
	}

	bool
	operator < (const basic_split_situation_key_type& r) const {
		return compare(r) < 0;
	}
};	// end struct basic_split_situation_key_type

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	For a given split-card, this structure tracks the 
	partial expectations of outcomes for split actions.
	There will be one of these structures per split-card.
	There's no need for a cache to cross-reference another
	entry with a different split-card.
	key: [split_state]
	Q: action_mask needed?
	value: expectations structure
 */
class player_split_basic_cache_type {
	friend class basic_strategy_analyzer;
	typedef	player_split_basic_cache_type	this_type;
	/// self-referential index to this entry
	typedef	basic_split_situation_key_type	key_type;
	typedef	edge_type			value_type;
//	typedef	expectations			value_type;
	typedef	std::map<key_type, value_type>	map_type;
public:
	card_type				split_card;
private:
	// back-reference to overall outcome cache (for non-split results)
	basic_strategy_analyzer*		_basic_analyzer;
	map_type				_map;
	// since card distribution is not evaluated
	// we can pre-compute the basic expectations of 
	// the terminal hands: non-splittable pairs, and unpaired hands
	// these should be computed once and cached (X and Y)
	// this will become a map for dynamic cache type (distribution key)
	edge_type				nonsplit_pair_exp;
	edge_type				unpaired_exp;
	bool					nonsplit_exp_valid;

private:
	// non-copyable, due to bare-pointer
	explicit
	player_split_basic_cache_type(const this_type&);

	this_type&
	operator = (const this_type&);

public:
	// default constructor -- split_card will be set by caller
	player_split_basic_cache_type() :
		_basic_analyzer(NULL), _map(), 
		nonsplit_pair_exp(0.0), unpaired_exp(0.0), 
		nonsplit_exp_valid(false) { }

	// default destructor

	void
	update_nonsplit_cache_outcome(
		const play_map&,
		const player_situation_basic_key_type&);

	// this will call basic analysis on outcome cache
	// for non-split actions.
	const value_type&
	evaluate(const play_map&, const key_type&);

	const value_type&
	evaluate(const play_map&, const player_situation_basic_key_type&);

};	// end class player_split_basic_cache_type

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if 0
class player_split_basic_cache_array {
	player_split_basic_cache_type		split_cache[card_values];
};	// end class player_split_basic_cache_array
#endif

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

#if 0
private:
	void
	__evaluate_player_basic_single(const play_map&,
		const player_situation_basic_key_type&, 
		expectations&);

	void
	__evaluate_player_basic_multi(const play_map&,
		const player_situation_basic_key_type&, 
		expectations&);
#endif

	edge_type
	__evaluate_split_basic(const play_map&,
		const player_situation_basic_key_type&);

};	// end struct player_outcome_cache_set

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	This contains the single-hand evaluator and the split-hand
	evaluator.
	TODO: basic strategy could be organized as a 2d-table
	instead of a map because it is small.
 */
class basic_strategy_analyzer {
	typedef	player_outcome_basic_cache_map_type	basic_map_type;
	// play_map				play;	// ?
	basic_map_type				basic_cache;
	player_split_basic_cache_type		split_cache[card_values];
//	player_split_basic_cache_array		split_cache;
	/// really, only basic dealer outcome cache is needed
	/// using reference because this is shared with other analyzers
	basic_dealer_outcome_cache_set		dealer_cache;
public:

	basic_strategy_analyzer();

	const expectations&
	evaluate_player_basic(const play_map&,
		const player_situation_basic_key_type&);

private:
	// non-copyable
	explicit
	basic_strategy_analyzer(const basic_strategy_analyzer&);

	// non-assignable
	basic_strategy_analyzer&
	operator = (const basic_strategy_analyzer&);

private:
	void
	__evaluate_player_basic_single(const play_map&,
		const player_situation_basic_key_type&, 
		expectations&);

#if 0
	void
	__evaluate_player_basic_multi(const play_map&,
		const player_situation_basic_key_type&, 
		expectations&);
#endif

#if 0
	edge_type
	__evaluate_split_basic(const play_map&,
		const player_situation_basic_key_type&);
#endif

};	// end class basic_strategy_analyzer

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BJ_CORE_ANALYSIS_HH__
