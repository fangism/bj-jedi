// "strategy.hh"

#ifndef	__BOC2_STRATEGY_HH__
#define	__BOC2_STRATEGY_HH__

#include <utility>
#include "card_state.hh"
#include "expectations.hh"
#include "enums.hh"
#include "util/tokenize.hh"

namespace blackjack {
class variation;
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

/**
	Have strategy struct contain an array of calculated strategies,
	one per reveal card.
	Rationale: computation efficiency, incremental calculation.
	Goal: 1
	Status: tested, perm'd
 */
// #define	RESTRUCTURE_STRATEGY_BY_REVEAL_CARD		1


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Represents the edge of a given situation with probability
	of winning, losing, tying.
	Sum of probabilities should be 1.0.
 */
struct outcome_odds {
	probability_type		win;
	probability_type		push;
	probability_type		lose;

	// initialize ctor
	outcome_odds() : win(0.0), push(0.0), lose(0.0) { }

	probability_type
	edge(void) const { return win -lose; }

	// asymmetric edge when payoff is != investment
	probability_type
	weighted_edge(const probability_type& w,
			const probability_type& l) const {
		return w*win -l*lose;
	}

	// since win + lose <= 1, we may discard the tie (push) factor
	probability_type
	ratioed_edge(void) const { return (win -lose)/(win +lose); }

	void
	check(void);
};	// end struct outcome_odds

typedef	array<outcome_odds, player_states>	outcome_vector;

extern
ostream&
dump_outcome_vector(const outcome_vector&, ostream&);

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
	size_t					reveal_card;
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
	typedef	array<probability_type, dealer_states>
						dealer_final_vector;

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
	typedef	array<probability_type, player_states>
					player_final_state_probability_vector;
	typedef	array<player_final_state_probability_vector, p_action_states>
					player_final_state_probability_matrix;
	player_final_state_probability_matrix
					player_final_state_probability;

	static
	void
	player_final_state_probabilities(const probability_vector&, 
		player_final_state_probability_vector&);

	outcome_vector				player_stand_pre_peek;
	outcome_vector				player_stand_post_peek;

	/**
		For each entry in 'player_stand': win - lose = edge.
	 */
	typedef	array<probability_type, player_states>
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
	compute_showdown_odds(const dealer_final_vector&, const edge_type&,
		outcome_vector&, player_stand_edges_vector&);

	void
	compute_player_stand_odds(const edge_type);

	static
	void
	__compute_player_hit_stand_edges(
//		const size_t r,
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
		const double&,
		const bool d, const bool s, const bool r);

	// private:
	void
	compute_player_split_edges(
		const play_map&, const deck_distribution&,
		const bool d, const bool s);

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

	typedef	array<probability_type, player_states>
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
	evaluate(const size_t);

private:
	void
	set_card_distribution(const deck_distribution&);

	void
	update_player_initial_state_odds(void);

	void
	check_odds(void) const;

	ostream&
	dump_expectations(const size_t state, ostream&) const;

	ostream&
	dump_optimal_actions(const size_t state, ostream&, 
		const size_t, const char*) const;

	ostream&
	dump_optimal_edges(const size_t state, ostream&) const;

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
		outcome_vector reveal_strategy::*,
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
	lookup_pre_peek_initial_edge(const size_t, const size_t) const;

	const expectations&
	lookup_player_action_expectations(const size_t, const size_t) const;

	int
	command(const string_list&) const;

	// interactive menu commands
	int
	main(const char*) const;

};	// end class strategy

ostream&
operator << (ostream&, const outcome_odds&);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BOC2_STRATEGY_HH__

