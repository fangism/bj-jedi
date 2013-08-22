/**
	\file "bj/core/analysis/player_analysis.hh"
	Probability analysis routines.
	$Id: $
 */

#ifndef	__BJ_CORE_PLAYER_ANALYSIS_HH__
#define	__BJ_CORE_PLAYER_ANALYSIS_HH__

#include <iosfwd>
#include <map>
#include "bj/core/expectations.hh"
#include "bj/core/state/split_state.hh"
#include "bj/core/state/player_hand.hh"
#include "bj/core/analysis/dealer_analysis.hh"

namespace blackjack {
using std::ostream;
class play_map;
class player_outcome_cache_set;
class basic_split_situation_key_type;
class basic_strategy_analyzer;

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
		const player_situation_basic_key_type&, 
		const perceived_deck_state&);

	// this will call basic analysis on outcome cache
	// for non-split actions.
	const value_type&
	evaluate(const play_map&, const key_type&, const perceived_deck_state&);

	const value_type&
	evaluate(const play_map&, const player_situation_basic_key_type&, const perceived_deck_state&);

};	// end class player_split_basic_cache_type

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/// TODO: there's a lot of redundant information computed, due to
/// overlap in the action_masks, should take advantage of it somehow
/// is it correct to always compute for all actions, even illegal ones?

typedef	std::map<player_situation_key_type, expectations>
			player_outcome_cache_map_type;
typedef	std::map<player_situation_basic_key_type, expectations>
			player_outcome_basic_cache_map_type;

#if 0
/**
	Collection of outcome vector caches.
	One for each accuracy setting.
	NOTE: this class is being phased out in favor of
	having one distinct class per analysis strategy.
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

	edge_type
	__evaluate_split_basic(const play_map&,
		const player_situation_basic_key_type&);

};	// end struct player_outcome_cache_set
#endif

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
	/// really, only basic dealer outcome cache is needed
	/// using reference because this is shared with other analyzers
	basic_dealer_outcome_cache_set		dealer_cache;
public:

	basic_strategy_analyzer();

	const expectations&
	evaluate(const play_map&,
		const player_situation_basic_key_type&,
		const perceived_deck_state&);

	// assume standard distribution
	const expectations&
	evaluate(const play_map& p,
		const player_situation_basic_key_type& k);

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
		const perceived_deck_state&,
		expectations&);

// uncached computations
	edge_type
	__evaluate_stand(const play_map&,
		const player_situation_basic_key_type&,
		const perceived_deck_state&);

	edge_type
	__evaluate_hit(const play_map&,
		const player_situation_basic_key_type&,
		const perceived_deck_state&);

	edge_type
	__evaluate_double_down(const play_map&,
		const player_situation_basic_key_type&,
		const perceived_deck_state&);

};	// end class basic_strategy_analyzer

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	The dynamic strategy analysis takes into account the
	current distribution of cards remaining in the deck.
	This is really just a collection of basic_strategy_analyzers,
	one for each distribution.  
	Reminder that card removal is not modeled (i.e. with replacement).
 */
class dynamic_strategy_analyzer {
};	// end class dynamic_strategy_analyzer

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BJ_CORE_PLAYER_ANALYSIS_HH__
