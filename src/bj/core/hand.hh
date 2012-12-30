// "bj/core/hand.hh"

#ifndef	__BJ_CORE_HAND_HH__
#define	__BJ_CORE_HAND_HH__

#include <iosfwd>
#include <string>

#include "config.h"
#if defined(HAVE_UNISTD_H)
#include <unistd.h>			// for <sys/types.h>: ssize_t
#elif defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>			// for ssize_t
#endif

#include "bj/core/enums.hh"		// for player_blackjack
#include "bj/core/num.hh"
#include "bj/core/player_action.hh"

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
	To compute the exact expected value of splitting and resplitting,
	the exact state of the hand must also capture:
	1) the number of splits remaining (depending on maximum)
	2) the number unplayed splittable hands
	Each time a hand is split, splits-remaining decrements.
	The number of splittable-hands is determined by
	the new upcards on the split hands (probability vector).  

	Consider a paired hand X,X.
	After splitting, there are new hands X,Y and X,Z.
	The order in which these are played does not affect the overall 
	expected outcome.
	Depending on Y and Z, the new state may have 0,1,2 newly 
	resplittable hands, but number of splits remaining is one less.
	See math text.
 */
struct split_state {
	ssize_t			splits_remaining;
	ssize_t			paired_hands;
	ssize_t			unpaired_hands;

	/// initialize to arbitrary values, should call initialize()
	split_state() : splits_remaining(1), paired_hands(0), 
		unpaired_hands(1) { }

	void
	initialize_default(void) {
		splits_remaining = 1;
		paired_hands = 0;
		unpaired_hands = 1;
	}

	void
	initialize(const ssize_t, const bool);

	bool
	is_splittable(void) const {
		return splits_remaining && paired_hands;
	}

	ssize_t
	total_hands(void) const { return paired_hands +unpaired_hands; }

	ostream&
	dump_code(ostream&) const;

	void
	post_split_states(split_state&, split_state&, split_state&) const;

	ssize_t
	nonsplit_pairs(void) const {
		if (paired_hands > splits_remaining) {
			return paired_hands -splits_remaining;
		}
		return paired_hands;
	}

	int
	compare(const split_state& r) const {
	{
		const ssize_t d = r.splits_remaining -splits_remaining;
		if (d) return d;
	}{
		const ssize_t d = r.paired_hands -paired_hands;
		if (d) return d;
	}
		return r.unpaired_hands -unpaired_hands;
	}

	bool
	operator < (const split_state& r) const {
		return compare(r) < 0;
	}

	ostream&
	generate_graph(ostream&) const;

};	// end struct split_state

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

	card_type
	pair_card(void) const {
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
	Essential information about the state of the dealer's hand.
	Use pair/tuple?
	This participates in key to dealer_final_state spread map.
	TODO: use bitfield or enum for peek status
 */
struct dealer_hand_base {
	dealer_state_type			state;
	peek_state_enum				peek_state;
	/**
		This is true if the dealer has only revealed first card so far.
		In a no-peek situation, if the second (hole) card makes 21, 
		it is considered a natural blackjack.
	 */
	bool					first_card;

	dealer_hand_base() : state(0),
		peek_state(NO_PEEK), first_card(true) { }

	explicit
	dealer_hand_base(const dealer_state_type d) :
		state(d), peek_state(NO_PEEK), first_card(true) { }

	void
	set_upcard(const card_type);

	void
	reveal_hole_card(void) {
		// reset peek status after hole card is revealed
		peek_state = NO_PEEK;
		first_card = false;
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

	void
	check_blackjack(const bool f) {
		first_card = false;
		if (f && (state == goal)) {
			state = dealer_blackjack;
		}
	}

	int
	compare(const dealer_hand_base& r) const {
		if (peek_state < r.peek_state)
			return -1;
		else if (r.peek_state < peek_state)
			return 1;
		const int sd = int(state -r.state);
		if (sd) return sd;
		const int fd = int(first_card) -int(r.first_card);
		return fd;
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
/**
	The full dealer's hand includes the reveal card.
	The unknown hole card is kept elsewhere.
 */
struct dealer_hand : public dealer_hand_base, public hand_common {
	// dealer's revealed card
	size_t				reveal;

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

#endif	// __BJ_CORE_HAND_HH__

