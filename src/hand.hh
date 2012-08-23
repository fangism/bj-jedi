// "hand.hh"

#ifndef	__BOC2_HAND_HH__
#define	__BOC2_HAND_HH__

#include <iosfwd>
#include <string>
#include "enums.hh"		// for player_blackjack
#include "player_action.hh"

namespace blackjack {
class play_map;
using std::string;
using std::ostream;

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
	A single player or dealer hand state.
 */
struct hand {
	const play_map*				play;
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
	/**
		A,2-9,T.
	 */
	typedef	string				player_cards;
	player_cards				cards;
	// player's actions may be limited by state
	action_mask				player_options;
	/**
		Enumerated state, from the state machine.
		Also encodes the value of the hand, the total.
	 */
	size_t					state;
	play_state				action;

	hand() : play(NULL), cards(),
		player_options(action_mask::all),
		state(0), action(LIVE) { }

	explicit
	hand(const play_map& p) : play(&p), cards(), state(0), action(LIVE) { }

	// initial deal
	void
	initial_card_player(const size_t);

	void
	initial_card_dealer(const size_t);

	void
	deal_player(const size_t, const size_t, const bool, const size_t);

	void
	deal_dealer(const size_t, const size_t);

	// hit state transition -- use this for double-down too
	void
	hit_player(const size_t);

	void
	hit_dealer(const size_t);

	void
	double_down(const size_t);

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

	bool
	splittable(void) const;

	void
	split(hand&, const size_t, const size_t, const size_t);

	bool
	doubleable(void) const {
		return cards.size() == 2;
	}
	bool
	surrenderable(void) const {
		return cards.size() == 2;
	}

	bool
	has_blackjack(void) const {
		return state == player_blackjack;
	}

	bool
	player_busted(void) const {
		return state == player_bust;
	}

	bool
	dealer_busted(void) const {
		return state == dealer_bust;
	}

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
	 */
	bool
	player_prompt(void) const {
		return !player_busted() && (action == LIVE) && (state != goal);
	}

	ostream&
	dump_dealer(ostream&) const;

	ostream&
	dump_player(ostream&) const;

};	// end struct hand

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BOC2_HAND_HH__

