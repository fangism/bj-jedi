// "blackjack.hh"

#ifndef	__BOC2_BLACKJACK_HH__
#define	__BOC2_BLACKJACK_HH__

#include <vector>
#include <string>
#include <map>
#include "enums.hh"
#include "variation.hh"

namespace blackjack {
using std::map;
using std::pair;
using std::istream;
using std::ostream;
using std::string;
using util::array;
using cards::card_values;
using cards::probability_vector;
using cards::deck_distribution;
using cards::deck_count_type;
using cards::state_machine;

// defined in grader.hh
extern
player_choice
prompt_player_action(istream&, ostream&,
	const bool d, const bool p, const bool r, const bool a = false);

extern
const char* action_names[];

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Based on the rules, the game state transitions for the 
	dealer and the player.
	Edges are NOT computed here.  
 */
struct play_map {
public:
	// mapping of initial card to initial state
	// for both dealer AND player
	// these could go into a struct for rules
	static const size_t		p_initial_card_map[card_values];
	static const size_t		d_initial_card_map[card_values];
// private:
	static const char		player_final_states[][player_states];
	static const char		dealer_final_states[][dealer_states];

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

	/// in the dealer/player final states, who wins
	typedef	array<outcome, dealer_states>		outcome_array_type;
	typedef	array<outcome_array_type, player_states>
							outcome_matrix_type;
	static const outcome_matrix_type&	outcome_matrix;
private:
	static outcome_matrix_type	__outcome_matrix;
	static const int		init_outcome_matrix;

public:
	explicit
	play_map(const variation&);

	/// maps player state index to player_final_outcome state
	static
	size_t
	player_final_state_map(const size_t);

private:
	void
	set_dealer_policy(void);

	void
	compute_player_hit_state(void);

	void
	compute_player_split_state(void);

	static
	int
	compute_final_outcomes(void);

public:
	bool
	is_player_terminal(const size_t) const;

	bool
	is_dealer_terminal(const size_t) const;

	// action transitions
	size_t
	initial_card_player(const size_t) const;

	size_t
	initial_card_dealer(const size_t) const;

	size_t
	deal_player(const size_t, const size_t, const bool) const;

	size_t
	hit_player(const size_t, const size_t) const;

	size_t
	hit_dealer(const size_t, const size_t) const;

	ostream&
	dump_dealer_policy(ostream&) const;

	ostream&
	dump_player_hit_state(ostream&) const;

	ostream&
	dump_player_split_state(ostream&) const;

	static
	ostream&
	dump_final_outcomes(ostream&);

	ostream&
	dump_variation(ostream& o) const {
		return var.dump(o);
	}
};	// end class play_map

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BOC2_BLACKJACK_HH__

