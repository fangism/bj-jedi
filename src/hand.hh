// "hand.hh"

#ifndef	__BOC2_HAND_HH__
#define	__BOC2_HAND_HH__

#include <iosfwd>
#include <string>
#include "enums.hh"		// for player_blackjack

namespace blackjack {
class play_map;
using std::string;
using std::ostream;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	A single player or dealer hand state.
 */
struct hand {
	/**
		A,2-9,T.
	 */
	typedef	string				player_cards;
	player_cards				cards;
	/**
		Enumerated state, from the state machine.
	 */
	size_t					state;
	/**
		Whether or not this hand was doubled-down.
	 */
	bool					doubled_down;
	/**
		Whether or not this hand was surrendered.
	 */
	bool					surrendered;

	hand() : cards(), state(0),
		doubled_down(false), surrendered(false) { }

	// initial deal
	void
	initial_card_player(const play_map&, const size_t);

	void
	initial_card_dealer(const play_map&, const size_t);

	void
	deal_player(const play_map&, const size_t, const size_t, const bool);

	void
	deal_dealer(const play_map&, const size_t, const size_t);

	// hit state transition -- use this for double-down too
	void
	hit_player(const play_map&, const size_t);

	void
	hit_dealer(const play_map&, const size_t);

	void
	presplit(const play_map&);

	void
	split(const play_map&, const size_t);

	bool
	splittable(void) const;

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

	ostream&
	dump_dealer(ostream&, const play_map&) const;

	ostream&
	dump_player(ostream&, const play_map&) const;

};	// end struct hand

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BOC2_HAND_HH__

