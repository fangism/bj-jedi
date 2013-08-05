// "bj/core/blackjack.hh"

#ifndef	__BJ_CORE_BLACKJACK_HH__
#define	__BJ_CORE_BLACKJACK_HH__

#include <vector>
#include <string>
#include <map>
#include "bj/core/outcome.hh"
#include "bj/core/card_state.hh"
#include "bj/core/variation.hh"
#include "bj/core/player_action.hh"

/**
	Define to 1 to use per-state actions masks.
	Initial action mask should be the intersection (AND)
	of the allowable actions given the hand and 
	allowable actions given dealer's reveal card.
	Actions may be further restricted by other rule variations.
	Goal: 1
	Rationale: cleaner definition of state transitions for 
		analysis and interactive game.
	Status: tested, perm'd
 */
// #define	ACTION_MASKS_GIVEN_STATE		1

namespace blackjack {
using std::map;
using std::pair;
using std::istream;
using std::ostream;
using std::string;
using std::vector;
using util::array;
using cards::card_values;
using cards::probability_vector;
using cards::deck_distribution;
using cards::deck_count_type;
using cards::state_machine;
using cards::card_type;
using cards::state_type;
using cards::player_state_type;
using cards::dealer_state_type;

extern
const char* action_names[];

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Based on the rules, the game state transitions for the 
	dealer and the player.
	Edges are NOT computed here.  
 */
class play_map {
public:
	// mapping of initial card to initial state
	// for both dealer AND player
	// these could go into a struct for rules
	static const player_state_type		p_initial_card_map[card_values];
	static const dealer_state_type		d_initial_card_map[card_values];
private:
	static const char		player_final_states[][p_final_states];
	static const char		dealer_final_states[][d_final_states];

public:
	const variation&		var;
// some of these could be split into a rules struct
	/// the dealer's fixed state machine, also action table
	state_machine			dealer_hit;
	/// the player's state machine for hits (for calculation, not optimized)
	state_machine			player_hit;
	/// transition table for splits, with possible re-split
	state_machine			player_resplit;
	/// transition table for splits, with no re-split
	state_machine			last_split;

	typedef array<action_mask, p_action_states>
					initial_actions_per_state_type;
	typedef	array<action_mask, card_values>
					initial_actions_given_dealer_type;
	initial_actions_per_state_type	initial_actions_per_state;
	initial_actions_given_dealer_type
					initial_actions_given_dealer;
	/// after a player hits, actions options are restricted
	action_mask			post_hit_actions;
	/// after a player doubles-down, actions options are restricted
	action_mask			post_double_down_actions;
	/// after a player splits, action options are restricted
	action_mask			post_split_actions;

	/// calculated: actions that are always final
	action_mask			terminal_actions;

private:
	/// in the dealer/player final states, who wins
	typedef	array<outcome, d_final_states>		outcome_array_type;
	typedef	array<outcome_array_type, p_final_states>
							outcome_matrix_type;
	// TODO: support rule variations in outcome matrix
	// e.g., player-loses-ties, player-blackjack-always-wins
	outcome_matrix_type		outcome_matrix;

private:
	static vector<state_type>		__reverse_topo_order;
	static const int			init_reverse_topo_order;
public:
	static const vector<state_type>&	reverse_topo_order;
public:
	explicit
	play_map(const variation&);

	/// maps player state index to player_final_outcome state
	static
	player_state_type
	player_final_state_map(const player_state_type);

	void
	reset(void);

private:
	void
	initialize_action_masks(void);

	void
	set_dealer_policy(void);

	void
	compute_player_hit_state(void);

	void
	compute_player_split_state(void);

	void
	initialize_outcome_matrix(void);

	static
	int
	initialize_reverse_topo_order(void);

public:
	void
	compute_player_final_outcome(const player_state_type p,
		const dealer_final_vector&, 
		outcome_odds&) const;

	bool
	is_player_terminal(const player_state_type) const;

	bool
	is_dealer_terminal(const dealer_state_type) const;

	bool
	is_player_pair(const player_state_type) const;

	static
	player_state_type
	paired_state(const card_type i) {
		return pair_offset +i;
	}

	// action transitions
	player_state_type
	initial_card_player(const card_type) const;

	dealer_state_type
	initial_card_dealer(const card_type) const;

	player_state_type
	deal_player(const card_type, const card_type, const bool) const;

	player_state_type
	hit_player(const player_state_type, const card_type) const;

	player_state_type
	split_player(const player_state_type, const card_type) const;

	player_state_type
	final_split_player(const player_state_type, const card_type) const;

	dealer_state_type
	hit_dealer(const dealer_state_type, const card_type) const;

	const outcome&
	lookup_outcome(const player_state_type p, const dealer_state_type d) const;

	void
	compute_player_final_outcome_vector(const dealer_final_vector&,
		player_final_outcome_vector&) const;

	ostream&
	dump_dealer_policy(ostream&) const;

	ostream&
	dump_player_hit_state(ostream&) const;

	ostream&
	dump_player_split_state(ostream&) const;

	ostream&
	dump_final_outcomes(ostream&) const;

	ostream&
	dump_variation(ostream& o) const {
		return var.dump(o);
	}

	static
	ostream&
	dealer_final_table_header(ostream&);

	static
	ostream&
	player_final_table_header(ostream&);

};	// end class play_map

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BJ_CORE_BLACKJACK_HH__

