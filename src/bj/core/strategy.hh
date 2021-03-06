// "bj/core/strategy.hh"

#ifndef	__BJ_CORE_STRATEGY_HH__
#define	__BJ_CORE_STRATEGY_HH__

#include <utility>
#include "bj/core/card_state.hh"
#include "bj/core/expectations.hh"
#include "bj/core/enums.hh"
#include "bj/core/outcome.hh"
#include "util/tokenize.hh"

namespace blackjack {
struct variation;
class play_map;
using std::pair;
using std::istream;
using std::ostream;
using util::array;
using util::string_list;
using cards::card_values;
using cards::probability_type;
using cards::probability_vector;
using cards::deck_distribution;
using cards::extended_deck_count_type;
using cards::deck_count_type;
using cards::state_machine;
using cards::card_type;
using cards::player_state_type;

/**
	Have strategy struct contain an array of calculated strategies,
	one per reveal card.
	Rationale: computation efficiency, incremental calculation.
	Goal: 1
	Status: tested, perm'd
 */
// #define	RESTRUCTURE_STRATEGY_BY_REVEAL_CARD		1

typedef	array<probability_type, p_action_states>
					player_state_probability_vector;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Computed strategy, given a single dealer's reveal card.
 */
struct reveal_strategy {
	/**
		The index of the revealed card.
	 */
	card_type				reveal_card;
	/**
		"Hit until it's not good to hit..."
		One player decision state machine for each dealer reveal card
		Each state machine is generated after hit/stand choices
		have been evaluated in the expectations matrix.
		Each matrix can then be used to solve the probability
		distribution of terminal states.  
		Having evaluated splits is not necessary, however, 
		Some of the variation variables affect the outcome of 
		this matrix (e.g. double-after-split).  
		NOTE: double-downs need to be computed separately, 
			using a single convolve step; double-downs are not
			included, they should be looked up.
	 */
	state_machine				player_opt_hit;

	/**
		Prior to peek, these are the odds of the dealer's final state
		given the reveal card.
	 */
	dealer_final_vector		dealer_final_given_revealed_pre_peek;
	/**
		Post-peek, these are the odds of the dealer's final state
		given the reveal card.
		This will only differ for ACE,TEN.
	 */
	dealer_final_vector		dealer_final_given_revealed_post_peek;
	/**
		Given:
		i - player's initial state
		j - player's final state (probability)
		assuming that the player hits until it is not advisable 
			(using player_opt_hit)
	 */
	typedef	array<probability_type, p_final_states>
					player_final_state_probability_vector;
	typedef	array<player_final_state_probability_vector, p_action_states>
					player_final_state_probability_matrix;
	player_final_state_probability_matrix
					player_final_state_probability;

	static
	void
	player_final_state_probabilities(const probability_vector&, 
		player_final_state_probability_vector&);

	player_final_outcome_vector				player_stand_pre_peek;
	player_final_outcome_vector				player_stand_post_peek;

	/**
		For each entry in 'player_stand': win - lose = edge.
	 */
	typedef	array<probability_type, p_final_states>
						player_stand_edges_vector;
	/**
		index: player's final value (including bust)
	 */
	player_stand_edges_vector		player_stand_edges_pre_peek;
	player_stand_edges_vector		player_stand_edges_post_peek;

	typedef	array<expectations, p_action_states>	expectations_vector;

	/**
		Matrix is indexed: [player-state]
		NOTE: transposed from old way.
		values are expected outcome values.
	 */
	expectations_vector			player_actions;

	/**
		The edge (advantage) values for a player
		in "hit-or-stand-mode", using player_final_state_probabilities.
		Computed using probability-weighted sum of 
		final-state-spread and final-state-odds 
		(when standing or in terminal state).
		This table is not applicable to doubling, splitting, surrender.
	 */
	typedef	array<edge_type, p_action_states>
						player_hit_stand_edges_vector;
	/**
		This table represents the optimal edges given only a 
		hit or stand choice.
		Hit/stand is only revelant post-peek, 
		don't bother with pre-peek (until European style).
	 */
	player_hit_stand_edges_vector		player_hit_stand_edges;
//	player_hit_stand_edges_vector		player_hit_stand_edges_post_peek;
	/**
		Spread of player edges, given initial state and reveal card.
		Each cell is taken as the max between stand, hit-mode, 
		double, and split if applicable.
		Assumes already dealer already peeked for blackjack, 
		if applicable.  
		This table needs to be computed iteratively with
		player_split_edges, below,
		depending on double-after-split and resplit.
		The first iteration will exclude splitting.

		Separate table for computing only split expectations, 
		as a workspace for evaluating.
		Computed iteratively, based on player_initial_edges, 
		first without resplits or double-after-split, 
		then with doubles, if the variation uses it.
		After computing this, player_actions::split can 
		and should be updated to recompute player_initial_edges.
		Index: player's pair, dealer's reveal card

	edge_type (*player_split_edges)[card_values] =
		&player_initial_edges[p_action_states];
	 */
	typedef	array<edge_type, p_action_states>	
						player_initial_edges_vector;
	/**
		player's edges, given reveal card, post-peek.
	 */
//	player_initial_edges_vector		player_initial_edges_pre_peek;
	player_initial_edges_vector		player_initial_edges_post_peek;

	/**
		Given only the dealer's card, what is the player's
		expected outcome, given optimal playing?
		Computed using probability weighted sum over
		spread of initial states.  
	 */
	edge_type			player_edge_given_reveal_pre_peek;
	edge_type			player_edge_given_reveal_post_peek;

	/**
		Semaphore for signaling need to re-calculate
		after distribution has been updated.  
	 */
	bool					need_update;
public:
	// default ctor,dtor
	reveal_strategy();

private:
	void
	compute_dealer_final_table(const play_map&, const deck_distribution&);

	static
	void
	compute_showdown_odds(const play_map&,
		const dealer_final_vector&, const edge_type&,
		player_final_outcome_vector&, player_stand_edges_vector&);

	void
	compute_player_stand_odds(const play_map&, const edge_type);

	static
	void
	__compute_player_hit_stand_edges(
//		const card_type r,
		const player_stand_edges_vector&, 
		const player_final_state_probability_matrix&,
		const expectations_vector&,
		const edge_type&, player_hit_stand_edges_vector&);

	void
	compute_action_expectations(const play_map&, const deck_distribution&);

	void
	optimize_actions(const double&);

	void
	optimize_player_hit_tables(const play_map&, const deck_distribution&);

	void
	compute_player_hit_stand_edges(const double&);

	void
	reset_split_edges(const play_map&);

	void
	compute_player_initial_nonsplit_edges(
		const play_map&, const action_mask&);

	// private:
	void
	compute_player_split_edges(
		const play_map&, const deck_distribution&,
		const action_mask&);

	void
	compute_reveal_edges(const deck_distribution&, 
		const player_state_probability_vector&);

public:
	void
	evaluate(const play_map&, const deck_distribution&, 
		const player_state_probability_vector&);

	ostream&
	dump_player_final_state_probabilities(ostream&) const;


};	// end struct reveal_strategy

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Blackjack player strategy calculator class.
	All expectations, probabilities, edges, and optimal actions
	are evaluated here.
 */
class strategy {
public:
	const variation&		var;
	const play_map&			play;
private:

	typedef	array<probability_type, p_final_states>
					player_final_state_probability_vector;

	/**
		Precise distribution of cards (integer) remaining.
	 */
	deck_count_type			card_count;
	/**
		This is just a cached set of probabilities based on
		the card_count.
		the probability vector for card values, A through 10.
		entries are 0-indexed, with 0 = Ace, 1 = 2, etc.
	 */
	deck_distribution		card_odds;

public:

	static const char			action_key[];
	// order of preference
	typedef	player_choice			action_preference[4];

private:
	typedef	array<edge_type, card_values>		player_initial_edges_vector;

	typedef	array<reveal_strategy, card_values>	reveal_array_type;
	reveal_array_type				reveal;

	player_state_probability_vector		player_initial_state_odds;

	/**
		The overall player's edge, before any cards are dealt.  
		Before sitting down at the table.  :)
	 */
	edge_type				_overall_edge;

	/**
		Semaphore for signaling need to re-calculate
		after distribution has been updated.  
	 */
	bool					initial_need_update;
	bool					overall_need_update;
public:
	explicit
	strategy(const play_map&);

	void
	set_card_distribution(const deck_count_type&);

	void
	set_card_distribution(const extended_deck_count_type&);

	void
	evaluate(void);

	void
	evaluate(const card_type);

private:
	void
	set_card_distribution(const deck_distribution&);

	void
	update_player_initial_state_odds(void);

	void
	check_odds(void) const;

	ostream&
	dump_expectations(const player_state_type state, ostream&) const;

	ostream&
	dump_optimal_actions(const player_state_type state, ostream&, 
		const size_t, const char*) const;

	ostream&
	dump_optimal_edges(const player_state_type state, ostream&) const;

	void
	compute_overall_edge(void);

public:
	edge_type
	overall_edge(void) const { return _overall_edge; }

	ostream&
	dump_player_initial_state_odds(ostream&) const;

	ostream&
	dump_player_hit_tables(ostream&) const;

	ostream&
	dump_player_final_state_probabilities(ostream&) const;

	ostream&
	dump_dealer_final_table(ostream&) const;

	ostream&
	__dump_player_stand_odds(ostream&,
		player_final_outcome_vector reveal_strategy::*,
		const state_machine&) const;

	ostream&
	__dump_player_stand_edges(ostream&,
		reveal_strategy::player_stand_edges_vector reveal_strategy::*,
		const state_machine&) const;

	ostream&
	dump_player_stand_odds(ostream&) const;

	ostream&
	dump_action_expectations(ostream&) const;

	ostream&
	dump_optimal_actions(ostream&) const;

	ostream&
	dump_player_hit_stand_edges(ostream&) const;

	ostream&
	dump_player_initial_edges(ostream&) const;

	ostream&
	dump_player_split_edges(ostream&) const;

	ostream&
	dump_reveal_edges(ostream&) const;

	ostream&
	dump(ostream&) const;

	edge_type
	lookup_pre_peek_initial_edge(const player_state_type, const card_type) const;

	const expectations&
	lookup_player_action_expectations(const player_state_type, const card_type) const;

	int
	command(const string_list&) const;

	// interactive menu commands
	int
	main(const char*) const;

};	// end class strategy

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BJ_CORE_STRATEGY_HH__

