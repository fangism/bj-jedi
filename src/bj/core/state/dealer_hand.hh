// "bj/core/state/dealer_hand.hh"

#ifndef	__BJ_CORE_STATE_DEALER_HAND_HH__
#define	__BJ_CORE_STATE_DEALER_HAND_HH__

#include <iosfwd>
#include <string>

#include "bj/core/enums.hh"		// for player_blackjack
#include "bj/core/num.hh"
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

#endif	// __BJ_CORE_STATE_DEALER_HAND_HH__

