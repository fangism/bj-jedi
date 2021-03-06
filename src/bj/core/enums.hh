// "bj/core/enums.hh"

#ifndef	__BJ_CORE_ENUMS_HH__
#define	__BJ_CORE_ENUMS_HH__

#include "bj/core/card_state.hh"		// for card_values

namespace blackjack {
using cards::card_values;

// keep consistent with blackjack.cc:action_names
enum player_choice {
	NIL = 0,	// invalid
	// useful values for analysis
	STAND = 1,
	HIT = 2,
	DOUBLE = 3,
	SPLIT = 4,
	__FIRST_EVAL_ACTION = STAND,		// index offset into array
	__NUM_EVAL_ACTIONS = SPLIT -STAND +1,
	SURRENDER = 5,
	// for interactive mode-only
	BOOKMARK = 6,	// save away
	COUNT = 7,	// show count
	HINT = 8,	// analyze
	OPTIM = 9	// auto-optimal
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	State machine table offsets.
	TODO: hide these details
	TODO: allow some of these to be variables to analyze game variants
	TODO: split these into player enum and dealer enum, 
		instead of passing around size_t's
 */
enum table_offsets {
	goal = 21,
	stop = 17,		// dealer stands hard or soft 17
	// table offsets for state_machine states
	player_blackjack = goal +1,
	dealer_blackjack = goal +1,
//	bust = goal +1,
	player_bust = player_blackjack +1,
	dealer_bust = dealer_blackjack +1,
	// added one for dealer's "push22" for switch variation
	dealer_push = dealer_bust +1,
	dealer_soft = dealer_push +1,	// for push22
	player_soft = player_bust +1,
// private:
	soft_min = 1,       // 1-11
// only player may split
	pair_offset = player_soft +card_values +1,
	p_action_states = pair_offset +card_values,
	d_action_states = dealer_soft +card_values +1,
	p_final_states = goal -stop +4,
	d_final_states = goal -stop +4
	// player action table offset
//	static const size_t p_pair_states = p_action_states +card_values,
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	These peek states are mutually exclusive.
	The peek state influences drawing cards and analysis.
 */
enum peek_state_enum {
	NO_PEEK,
	PEEKED_NO_10,
	PEEKED_NO_ACE
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BJ_CORE_ENUMS_HH__

