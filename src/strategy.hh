// "strategy.hh"

#ifndef	__BOC2_STRATEGY_HH__
#define	__BOC2_STRATEGY_HH__

#include <utility>
#include "card_state.hh"
#include "enums.hh"

namespace blackjack {
class variation;
class play_map;
using std::pair;
using std::istream;
using std::ostream;
using util::array;
using cards::card_values;
using cards::probability_type;
using cards::probability_vector;
using cards::deck_distribution;
using cards::deck_count_type;
using cards::state_machine;

/**
	The mathematical edge (scalar) type.  
 */
typedef	probability_type		edge_type;

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

	static
	void
	player_final_state_probabilities(const probability_vector&, 
		player_final_state_probability_vector&);

	/**
		the probability vector for card values, A through 10.
		entries are 0-indexed, with 0 = Ace, 1 = 2, etc.
	 */
	deck_distribution		card_odds;
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
	array<state_machine, card_values>		player_opt;

	// TODO: support a mode where a player's strategy (however suboptimal)
	// can be mathematically evaluated (submit and score).

	/**
		This matrix is indexed: [reveal][finish-state]
		cell values are probabilities.
		Sum of each row should be 1.0.
	 */
	typedef	array<probability_type, dealer_states>
						dealer_final_vector_type;
	typedef	array<dealer_final_vector_type, card_values>	dealer_final_matrix;
	/**
		Prior to peek, these are the odds of the dealer's final state
		given the reveal card.
	 */
	dealer_final_matrix		dealer_final_given_revealed;
	/**
		Post peek, these are the odds of the dealer's final state
		given the reveal card.
	 */
	dealer_final_matrix		dealer_final_given_revealed_post_peek;
	/**
		Given:
		i - dealer's reveal card
		j - player's initial state
		k - player's final state (probability)
		assuming that the player hits until it is not advisable 
			(using player_opt)
	 */
	typedef	array<array<player_final_state_probability_vector,
			p_action_states>, card_values>
					player_final_states_probability_matrix;
	player_final_states_probability_matrix
					player_final_state_probability;

public:
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
	};

	static const char			action_key[];
	// order of preference
	typedef	player_choice			action_preference[4];

private:
	typedef	array<outcome_odds, player_states>	outcome_vector;
	typedef	array<outcome_vector, card_values>		outcome_matrix;

	/**
		Matrix is indexed: [reveal][player-state]
		cell values are probabilities that player will win/draw/lose.
	 */
	outcome_matrix				player_stand;
	outcome_matrix				player_stand_post_peek;
	/**
		For each entry in 'player_stand': win - lose = edge.
	 */
	typedef	array<probability_type, player_states>
						player_stand_edges_vector;
	typedef	array<player_stand_edges_vector, card_values>
						player_stand_edges_matrix;
	/**
		index i: dealer's reveal card
		index j: player's final value (including bust)
	 */
	player_stand_edges_matrix		player_stand_edges;
	player_stand_edges_matrix		player_stand_edges_post_peek;

public:
	/**
		Each field's value is the expected outcome of the decision, 
		with -1 being a loss, +1 being a win, etc...
		'hit' is initialized -1 for the sake of computing
		optimal decisions.  
	 */
	struct expectations {
		edge_type			stand;
		edge_type			hit;
		edge_type			double_down;
		edge_type			split;
#if 0
		static const edge_type		surrender;
#endif
		action_preference		actions;

		expectations() : stand(0.0), hit(-1.0),
			double_down(0.0), 
			split(-2.0) {
		actions[0] = actions[1] = actions[2] = actions[3] = NIL;
		}

		expectations(const edge_type s, const edge_type h, 
			const edge_type d, const edge_type p) :
			stand(s), hit(h), double_down(d), split(p) {
		}

		const edge_type&
		value(const player_choice c, const edge_type& s) const;

		pair<player_choice, player_choice>
		best_two(const bool d = true,
			const bool s = true,
			const bool r = true) const;

		player_choice
		best(const bool d = true,
			const bool s = true,
			const bool r = true) const {
			return best_two(d, s, r).first;
		}

		expectations
		operator + (const expectations& e) const {
			return expectations(stand +e.stand, 
				hit +e.hit, double_down +e.double_down, 
				split +e.split);
		}

		void
		optimize(const edge_type& r);

		// compare two choices
		edge_type
		margin(const player_choice& f, const player_choice& s, 
				const edge_type& r) const {
			return value(f, r) -value(s, r);
		}

		ostream&
		dump_choice_actions(ostream&, const edge_type&) const;

		static
		ostream&
		dump_choice_actions_2(ostream&, const expectations&,
			const expectations&, const edge_type&);

	};	// end struct expectations

private:
	typedef	array<expectations, card_values>	expectations_vector;
	typedef	array<expectations_vector, p_action_states>
						expectations_matrix;

	/**
		Matrix is indexed: [player-state][reveal]
		NOTE: transposed.
		cell values are expected outcome values.
	 */
	expectations_matrix			player_actions;

	/**
		The edge (advantage) values for a player
		in "hit-mode", using player_final_state_probabilities.
		Computed using probability-weighted sum of 
		final-state-spread and final-state-odds 
		(when standing or in terminal state).
		This table is not applicable to doubling, splitting, surrender.
	 */
	typedef	array<array<edge_type, card_values>, p_action_states>
						player_hit_edges_matrix;
	player_hit_edges_matrix			player_hit_edges;
//	player_hit_edges_matrix			player_hit_edges_post_peek;
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
	typedef	array<edge_type, card_values>		player_initial_edges_vector;
	typedef	array<player_initial_edges_vector, p_action_states>
						player_initial_edges_matrix;
	/**
		player's edges, given reveal card, post-peek.
	 */
//	player_initial_edges_matrix		player_initial_edges_pre_peek;
	player_initial_edges_matrix		player_initial_edges_post_peek;

	array<probability_type, p_action_states>
						player_initial_state_odds;

	/**
		Given only the dealer's card, what is the player's
		expected outcome, given optimal playing?
		Computed using probability weighted sum over
		spread of initial states.  
	 */
	player_initial_edges_vector		player_edges_given_reveal_pre_peek;
	player_initial_edges_vector		player_edges_given_reveal_post_peek;

	/**
		The overall player's edge, before any cards are dealt.  
		Before sitting down at the table.  :)
	 */
	edge_type				_overall_edge;
public:
	explicit
	strategy(const play_map&);

	void
	set_card_distribution(const deck_distribution&);

	void
	set_card_distribution(const deck_count_type&);

	void
	evaluate(void);

private:
	void
	update_player_initial_state_odds(void);

	void
	check_odds(void) const;

	void
	optimize_actions(void);

	void
	optimize_player_hit_tables(void);

	static
	ostream&
	dump_outcome_vector(const outcome_vector&, ostream&);

	ostream&
	dump_expectations(const expectations_vector&, ostream&) const;

	ostream&
	dump_optimal_actions(const expectations_vector&, ostream&, 
		const size_t, const char*) const;

	ostream&
	dump_optimal_edges(const expectations_vector&, ostream&) const;

	void
	reset_split_edges(void);

#if 0
	// account for player's blackjack
	void
	finalize_player_initial_edges(void);
#endif

	// TODO: privatize most compute methods
	void
	compute_dealer_final_table(void);

	static
	void
	compute_showdown_odds(const dealer_final_matrix&, const edge_type&,
		outcome_matrix&, player_stand_edges_matrix&);

	void
	compute_player_stand_odds(void);

	void
	compute_action_expectations(void);

	static
	void
	__compute_player_hit_edges(const player_stand_edges_matrix&, 
		const player_final_states_probability_matrix&,
		const expectations_matrix&,
		const edge_type&, player_hit_edges_matrix&);

	void
	compute_player_hit_edges(void);

	void
	compute_player_initial_edges(
		const bool d, const bool s, const bool r);

	// private:
	void
	compute_player_split_edges(const bool d, const bool s);

	void
	compute_reveal_edges(void);

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

	static
	ostream&
	__dump_player_stand_odds(ostream&,
		const outcome_matrix&, 
		const state_machine&);

	static
	ostream&
	__dump_player_stand_edges(ostream&,
		const player_stand_edges_matrix&, 
		const state_machine&);

	ostream&
	dump_player_stand_odds(ostream&) const;

	ostream&
	dump_action_expectations(ostream&) const;

	ostream&
	dump_optimal_actions(ostream&) const;

	ostream&
	dump_player_hit_edges(ostream&) const;

	ostream&
	dump_player_initial_edges(ostream&) const;

	ostream&
	dump_player_split_edges(ostream&) const;

	ostream&
	dump_reveal_edges(ostream&) const;

	ostream&
	dump(ostream&) const;


	const expectations&
	lookup_player_action_expectations(const size_t, const size_t) const;
#if 0
	ostream&
	dump_variation(ostream& o) const {
		return var.dump(o);
	}
#endif

};	// end class strategy

ostream&
operator << (ostream&, const strategy::outcome_odds&);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BOC2_STRATEGY_HH__

