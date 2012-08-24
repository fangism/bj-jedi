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
	The essential information about a player's hand.
	Excludes exact composition.
	This is used as a key to the situation map for analysis.
 */
struct player_hand_base {
	/**
		Enumerated state, from the state machine.
		Also encodes the value of the hand, the total.
	 */
	size_t					state;
	// player's actions may be limited by state
	action_mask				player_options;
	// TODO: splits remaining (countdown to 0)
	// TODO: hand size (number of cards)

	player_hand_base() : state(0), player_options(action_mask::all) { }

	bool
	has_blackjack(void) const {
		return state == player_blackjack;
	}

	bool
	player_busted(void) const {
		return state == player_bust;
	}

};	// end struct player_hand_base

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
struct dealer_hand_base {
	size_t					state;
	// reveal_card?
	// influences drawing cards
	bool					peeked_no_10;
	bool					peeked_no_Ace;

	dealer_hand_base() : state(0),
		peeked_no_10(false), peeked_no_Ace(false) { }

	void
	revealed_hole_card(void) {
		// reset peek status after hole card is revealed
		peeked_no_10 = false;
		peeked_no_Ace = false;
	}

	bool
	has_blackjack(void) const {
		return state == dealer_blackjack;
	}

	bool
	dealer_busted(void) const {
		return state == dealer_bust;
	}

};	// end struct dealer_hand

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
struct hand_common {
	// reference to game variation and state transition maps
	const play_map*				play;
	/**
		A,2-9,T.
	 */
	typedef	string				card_array_type;
	card_array_type				cards;

	hand_common() : play(NULL), cards() { }

	explicit
	hand_common(const play_map& p) : play(&p), cards() { }

};	// end struct hand_common

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

	player_hand() : player_hand_base(), hand_common(), action(LIVE) { }

	explicit
	player_hand(const play_map& p) : player_hand_base(), hand_common(p),
		action(LIVE) { }

	// initial deal
	void
	initial_card_player(const size_t);

	void
	deal_player(const size_t, const size_t, const bool, const size_t);

	// hit state transition -- use this for double-down too
	void
	hit_player(const size_t);

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

#if 0
	bool
	splittable(void) const;
#endif

	void
	split(player_hand&, const size_t, const size_t, const size_t);

#if 0
	bool
	doubleable(void) const {
		return cards.size() == 2;
	}

	bool
	surrenderable(void) const {
		return cards.size() == 2;
	}
#endif

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
struct dealer_hand : public dealer_hand_base, public hand_common {

	dealer_hand() : dealer_hand_base(), hand_common() { }

	explicit
	dealer_hand(const play_map& m) : dealer_hand_base(), hand_common(m) { }

	void
	initial_card_dealer(const size_t);

	void
	deal_dealer(const size_t, const size_t);

	void
	hit_dealer(const size_t);

	ostream&
	dump_dealer(ostream&) const;

};	// end struct dealer_hand
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BOC2_HAND_HH__

