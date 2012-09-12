// "hand.hh"

#ifndef	__BOC2_HAND_HH__
#define	__BOC2_HAND_HH__

#include <iosfwd>
#include <string>
#include "enums.hh"		// for player_blackjack
#include "player_action.hh"

namespace cards {
class state_machine;
}
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
	Use pair/tuple?
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

	player_hand_base(const size_t, const play_map&);

	player_hand_base(const size_t ps, const action_mask& am) :
		state(ps), player_options(am) { }

	bool
	has_blackjack(void) const {
		return state == player_blackjack;
	}

	bool
	player_busted(void) const {
		return state == player_bust;
	}

	// lexicographical key compare
	bool
	compare(const player_hand_base& r) const {
		if (state < r.state)
			return -1;
		else if (r.state < state)
			return 1;
		else if (player_options < r.player_options)
			return -1;
		else if (r.player_options < player_options)
			return 1;
		else	return 0;
	}

	// lexicographical key compare
	bool
	operator < (const player_hand_base& r) const {
		return compare(r) < 0;
	}

	ostream&
	dump(ostream&, const cards::state_machine&) const;

};	// end struct player_hand_base

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Essential information about the state of the dealer's hand.
	Use pair/tuple?
	This participates in key to dealer_final_state spread map.
	TODO: use bitfield or enum for peek status
 */
struct dealer_hand_base {
	size_t					state;
	peek_state_enum				peek_state;

	dealer_hand_base() : state(0),
		peek_state(NO_PEEK) { }

	explicit
	dealer_hand_base(const size_t d) : state(d), peek_state(NO_PEEK) { }

	void
	reveal_hole_card(void) {
		// reset peek status after hole card is revealed
		peek_state = NO_PEEK;
	}

	void
	peek_no_10(void) {
		peek_state = PEEKED_NO_10;
	}

	void
	peek_no_Ace(void) {
		peek_state = PEEKED_NO_ACE;
	}

	bool
	has_blackjack(void) const {
		return state == dealer_blackjack;
	}

	bool
	dealer_busted(void) const {
		return state == dealer_bust;
	}

	int
	compare(const dealer_hand_base& r) const {
		if (peek_state < r.peek_state)
			return -1;
		else if (r.peek_state < peek_state)
			return 1;
		else if (state < r.state)
			return -1;
		else if (r.state < state)
			return 1;
		else	return 0;
	}

	bool
	operator < (const dealer_hand_base& r) const {
		return compare(r) < 0;
	}

	ostream&
	dump(ostream&, const cards::state_machine&) const;

};	// end struct dealer_hand_base

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

	void
	split(player_hand&, const size_t, const size_t, const size_t);

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
/**
	The full dealer's hand includes the reveal card.
	The unknown hole card is kept elsewhere.
 */
struct dealer_hand : public dealer_hand_base, public hand_common {
	// dealer's revealed card
	size_t				reveal;

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

