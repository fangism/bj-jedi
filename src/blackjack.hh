// "blackjack.hh"

#ifndef	__BOC2_BLACKJACK_HH__
#define	__BOC2_BLACKJACK_HH__

#include "card_state.hh"
#include "util/array.hh"
#include <map>

// TODO: import packed_array class from my library
// is better for matrices and higher dimension structures
// and util::array or std::array

namespace blackjack {
using std::map;
using std::pair;
using std::istream;
using util::array;

/**
	The mathematical edge (scalar) type.  
 */
typedef	probability_type		edge_type;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Options for game variations.
	popularity comments on boolean variations:
	rare - almost never
	common - both variations are common
	usual - more often than not
	standard - almost always
 */
struct variation {
	/// if true, dealer must hit on soft 17, else must stand (common)
	bool			H17;
	/// option to surrender before checking for blackjack (rare)
	bool			surrender_early;
	/// option to surrender after checking for blackjack (common)
	bool			surrender_late;
	/// whether or not dealer checks for A hole card (usual)
	bool			peek_on_10;
	/// whether or not dealer checks for 10 hole card (standard)
	bool			peek_on_Ace;
	/// dealer wins ties (disasterous for player!) (rare)
	bool			ties_lose;
	/// player wins ties (rare)
	bool			ties_win;
	/// whether or not player may double on hard 9 (usual)
	bool			double_H9;
	/// whether or not player may double on hard 10 (standard)
	bool			double_H10;
	/// whether or not player may double on hard 11 (standard)
	bool			double_H11;
	/// whether or not player may double on other values (usual)
	bool			double_other;
	/// double-after-split? (common)
	bool			double_after_split;
	/// surrender-after-split? (never heard of it)
	/// allow splitting (standard)
	bool			split;
	/// allow resplitting, split-after-split (common)
	bool			resplit;

	/// most casinos allow only 1 card on each split ace (common)
	bool			one_card_on_split_aces;

	/// player's blackjack payoff
	double			bj_payoff;
	/// player's insurance payoff
	double			insurance;
	/// surrender loss, less than 0
	double			surrender_penalty;
	/// doubled-down multipler (other than 2.0 is rare)
	double			double_multiplier;

	// known unsupported options:
	// 5-card charlie -- would need expanded state table

	/// default ctor and options
	variation() : 
		H17(true), 
		surrender_early(false),
		surrender_late(true),
		peek_on_10(true),
		peek_on_Ace(true),
		ties_lose(false),
		ties_win(false),
		double_H9(true),
		double_H10(true),
		double_H11(true),
		double_other(true),
		double_after_split(true),
		split(true),
		resplit(false),
		one_card_on_split_aces(true),
		bj_payoff(1.5), 
		insurance(2.0),
		surrender_penalty(-0.5),
		double_multiplier(2.0) { }

	/// \return true if some form of doubling-down is allowed
	bool
	some_double(void) const {
		return double_H9 || double_H10 || double_H11 || double_other;
	}

	ostream&
	dump(ostream&) const;

};	// end struct variation

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Blackjack player strategy calculator class.
 */
class strategy : protected variation {
public:
	static	const deck		standard_deck_odds;
	static const size_t vals = 10;          // number of values, max value
	static const size_t goal = 21;
	static const size_t stop = 17;		// dealer stands hard or soft 17
	static const char		card_name[];
	enum {
		ACE = 0,		// index of Ace (card_odds)
		TEN = 9			// index of 10 (T) (card_odds)
	};
private:
	static const size_t bust = goal +1;     // table offset
	static const size_t soft = bust +1;     // table offset
	static const size_t soft_min = 1;       // 1-11
	static const size_t cols = goal -stop +2;
#if 0
	// states for the first card
	static const size_t initials = goal +vals +2;	// after split
	static const size_t seconds = intitials +vals;	// can double here
#endif
	static const size_t phit_states = goal +vals +2;
	static const size_t phit_pair_states = phit_states +vals;

	// mapping of initial card to initial state
	static const size_t		initial_card_states[vals];
	static const size_t		print_ordering[];
	static const char		player_final_states[][cols +1];

	/// maps player state index to player_final_outcome state
	static
	size_t
	player_final_state_map(const size_t);

	typedef	array<probability_type, cols+1>
					player_final_state_probability_vector;

	static
	void
	player_final_state_probabilities(const probability_vector&, 
		player_final_state_probability_vector&);

	/**
		the probability vector for card values, A through 10.
		entries are 0-indexed, with 0 = Ace, 1 = 2, etc.
	 */
	deck				card_odds;
	/// the dealer's fixed state machine, also action table
	state_machine			dealer;
	/// the player's state machine for hits (for calculation, not optimized)
	state_machine			player_hit;
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
	array<state_machine, vals>		player_opt;

	// TODO: support a mode where a player's strategy (however suboptimal)
	// can be mathematically evaluated (submit and score).

	/**
		This matrix is indexed: [reveal][finish-state]
		cell values are probabilities.
		Sum of each row should be 1.0.
	 */
	typedef	array<probability_type, cols>	dealer_final_vector_type;
	typedef	array<dealer_final_vector_type, vals>	dealer_final_matrix;
	dealer_final_matrix		dealer_final_given_revealed;
	/**
		Given:
		i - dealer's reveal card
		j - player's initial state
		k - player's final state (probability)
		assuming that the player hits until it is not advisable 
			(using player_opt)
	 */
	array<array<player_final_state_probability_vector, phit_states>, vals>
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

		// since win + lose <= 1, we may discard the tie (push) factor
		probability_type
		ratioed_edge(void) const { return (win -lose)/(win +lose); }

		void
		check(void);
	};

	enum player_choice {
		NIL = 0,	// invalid
		STAND = 1,
		HIT = 2,
		DOUBLE = 3,
		SPLIT = 4,
		SURRENDER = 5
	};
	static const char			action_key[];
	// order of preference
	typedef	player_choice			action_preference[4];

private:
	typedef	array<outcome_odds, cols+1>		outcome_vector;
	typedef	array<outcome_vector, vals>		outcome_matrix;

	/**
		Matrix is indexed: [reveal][player-state]
		cell values are probabilities that player will win/draw/lose.
		TODO: the size of this should be static, avoid alloc.
	 */
	outcome_matrix				player_stand;
	/**
		For each entry in 'player_stand': win - lose = edge.
	 */
	typedef	array<probability_type, cols+1>	player_stand_edges_vector;
	typedef	array<player_stand_edges_vector, vals>
						player_stand_edges_matrix;
	/**
		index i: dealer's reveal card
		index j: player's final value (including bust)
	 */
	player_stand_edges_matrix		player_stand_edges;

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
		static const edge_type		surrender;
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
		value(const player_choice c) const;

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
		optimize(void);

		// compare two choices
		edge_type
		margin(const player_choice f, const player_choice s) const {
			return value(f) -value(s);
		}

		ostream&
		dump_choice_actions(ostream&) const;
	};

	typedef	array<expectations, vals>	expectations_vector;
	typedef	array<expectations_vector, phit_states>	expectations_matrix;

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
	array<array<edge_type, vals>, phit_states>	player_hit_edges;
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

	edge_type (*player_split_edges)[vals] =
		&player_initial_edges[phit_states];
	 */
	typedef	array<edge_type, vals>		player_initial_edges_vector;
	array<player_initial_edges_vector, phit_pair_states>
						player_initial_edges;

	array<edge_type, phit_pair_states>	player_initial_state_odds;

	/**
		Given only the dealer's card, what is the player's
		expected outcome, given optimal playing?
		Computed using probability weighted sum over
		spread of initial states.  
	 */
	array<edge_type, vals>			player_edges_given_reveal;

	/**
		The overall player's edge, before any cards are dealt.  
		Before sitting down at the table.  :)
	 */
	edge_type				_overall_edge;
public:
	explicit
	strategy(const variation&);

	void
	set_card_distribution(const deck&);

	void
	evaluate(void);

private:
	void
	update_player_initial_state_odds(void);

	void
	set_dealer_policy(void);

	void
	compute_player_hit_state(void);

	void
	check_odds(void) const;

	void
	optimize_actions(void);

	void
	optimize_player_hit_tables(void);

	static
	ostream&
	dump_outcome_vector(const outcome_vector&, ostream&);

	static
	ostream&
	dump_expectations(const expectations_vector&, ostream&);

	void
	reset_split_edges(void);

	// account for player's blackjack
	void
	finalize_player_initial_edges(void);

	// TODO: privatize most compute methods
	void
	compute_dealer_final_table(void);

	void
	compute_player_stand_odds(void);

	void
	compute_action_expectations(void);

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
	dump_dealer_policy(ostream&) const;

	ostream&
	dump_player_hit_state(ostream&) const;

	ostream&
	dump_player_hit_tables(ostream&) const;

	ostream&
	dump_player_final_state_probabilities(ostream&) const;

	ostream&
	dump_dealer_final_table(ostream&) const;

	ostream&
	dump_player_stand_odds(ostream&) const;

	ostream&
	dump_action_expectations(ostream&) const;

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

};	// end class strategy

ostream&
operator << (ostream&, const strategy::outcome_odds&);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Classic game.  
 */
class simulator {
};	// end class simulator

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Tracks cards remaining in deck(s).
 */
struct counter {
	enum { bins = strategy::vals };
#if 0
	typedef	array<size_t, bins>		deck_type;
#else
	typedef	probability_vector		deck_type;
#endif
	deck_type				cards;

	counter(const size_t);

	void
	count(const size_t);
};	// end class counter

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	The user enters both the dealer and player's actions and outcomes.  
	The purposes is to characterize the odds of the game, grade the player's
	actions, and rate luck and skill.
	Input stream: history of cards seen and transactions.  
 */
class grader {
	strategy				S;
	counter					C;


};	// end class grader

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BOC2_BLACKJACK_HH__

