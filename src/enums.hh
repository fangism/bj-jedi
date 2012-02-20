// "enums.hh"

#ifndef	__BOC2_ENUMS_HH__
#define	__BOC2_ENUMS_HH__

#include "card_state.hh"		// for card_values

namespace blackjack {
using cards::card_values;

enum player_choice {
	NIL = 0,	// invalid
	// useful values for analysis
	STAND = 1,
	HIT = 2,
	DOUBLE = 3,
	SPLIT = 4,
	SURRENDER = 5,
	// for interactive mode-only
	HINT = 6,
	OPTIM = 7
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Dealer vs. player showdown outcomes.  
 */
enum outcome {
	WIN,
	PUSH,
	LOSE
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	State machine table offsets.
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
	player_states = goal -stop +4,
	dealer_states = goal -stop +4
	// player action table offset
//	static const size_t p_pair_states = p_action_states +card_values,
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BOC2_ENUMS_HH__
