// "bj/core/hand.hh"

#ifndef	__BJ_CORE_STATE_PLAYER_HAND_HH__
#define	__BJ_CORE_STATE_PLAYER_HAND_HH__

#include <iosfwd>
#include <string>

#include "bj/core/enums.hh"		// for player_blackjack
#include "bj/core/num.hh"
#include "bj/core/player_action.hh"
#include "bj/core/state/hand_common.hh"

namespace cards {
class state_machine;
}
namespace blackjack {
class play_map;
using std::string;
using std::ostream;
using cards::card_type;
using cards::player_state_type;
using cards::dealer_state_type;

/**
	Define to 1 to keep player_options action_mask with hand struct.
	Rationale: a 'hand' represents the player's situation, 
		including available choices.
	Goal: 1 (moved here from struct bookmark)
	Status: tested, perm'd
 */
// #define	HAND_PLAYER_OPTIONS		1

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	The essential information about a player's hand.
	Excludes exact composition.
	This is used as a key to the situation map for analysis.
	Use pair/tuple?
 */
struct player_hand_base {
	/**
		Enumerated state, from the state machine.
		Also encodes the value of the hand, the total.
	 */
	player_state_type				state;
	// player's actions may be limited by state
	action_mask				player_options;
	// TODO: splits remaining (countdown to 0)
	// TODO: hand size (number of cards)

	player_hand_base() : state(0), player_options(action_mask::all) { }

	player_hand_base(const player_state_type, const play_map&);

	player_hand_base(const player_state_type ps, const action_mask& am) :
		state(ps), player_options(am) { }

	bool
	has_blackjack(void) const {
		return state == player_blackjack;
	}

	bool
	player_busted(void) const {
		return state == player_bust;
	}

	bool
	is_paired(void) const {
		return state >= pair_offset && state < pair_offset +card_values;
	}

	void
	maximize_options(void) {
		player_options = is_paired() ? action_mask::all
			: action_mask::all_but_split;
	}

	// if hand is a paired hand, return the paired card
	card_type
	pair_card(void) const {
		// assert(is_paired());
		return state - pair_offset;
	}

	void
	hit(const play_map&, const card_type);

	// split without taking next card
	void
	split(const play_map&);

	// split and take next card
	void
	split(const play_map&, const card_type);

	// split and take next card
	void
	final_split(const play_map&, const card_type);

	// lexicographical key compare
	int
	compare(const player_hand_base& r) const {
		const int sc = state -r.state;
		if (sc) return sc;
		const int od = player_options.compare(r.player_options);
		return od;
	}

	// lexicographical key compare
	bool
	operator < (const player_hand_base& r) const {
		return compare(r) < 0;
	}

	ostream&
	dump_state_only(ostream&, const cards::state_machine&) const;

	ostream&
	dump(ostream&, const cards::state_machine&) const;

};	// end struct player_hand_base

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	A single player or dealer hand state.
 */
struct player_hand : public player_hand_base, public hand_common {
	/**
		Mutually exclusive states.
		Can't be doubled-down and surrendered.
		Could be doubled-down and busted though...
		The dealer's state is only ever live (or busted).
	 */
	enum play_state {
		LIVE,				// or busted
		STANDING,
		DOUBLED_DOWN,
		SURRENDERED
		// BUSTED
	};
	play_state				action;

	explicit
	player_hand(const play_map& p) : player_hand_base(), hand_common(p),
		action(LIVE) { }

	// initial deal
	void
	initial_card_player(const card_type);

	void
	deal_player(const card_type, const card_type, const bool, const card_type);

	// hit state transition -- use this for double-down too
	void
	hit_player(const card_type);

	void
	double_down(const card_type);

	void
	stand(void) {
		action = STANDING;
	}

	void
	surrender(void) {
		action = SURRENDERED;
	}

	bool
	surrendered(void) const {
		return action == SURRENDERED;
	}
	bool
	doubled_down(void) const {
		return action == DOUBLED_DOWN;
	}

	void
	split(player_hand&, const card_type, const card_type, const card_type);

	/**
		A hand is considered 'live' if it 
		has not busted (terminal), nor surrendered.
		Doubled-down is still considered live.  
	 */
	bool
	player_live(void) const {
		return !player_busted() && !surrendered();
	}

	/**
		Player should be prompt while alive, 
		and not doubled down, and not surrendered.
		Also, if 21, don't bother prompting.
		TODO: just use state of player_actions mask
	 */
	bool
	player_prompt(void) const {
		return !player_busted() && (action == LIVE) && (state != goal);
	}

	ostream&
	dump_player(ostream&) const;

};	// end struct player_hand

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BJ_CORE_STATE_PLAYER_HAND_HH__

