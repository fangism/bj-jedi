// "deck_state.hh"

#ifndef	__BOC2_DECK_STATE_HH__
#define	__BOC2_DECK_STATE_HH__

#include <utility>
#include <string>
#include "card_state.hh"
#include "counter.hh"

namespace blackjack {
class variation;
using std::istream;
using std::ostream;
using std::string;
using cards::probability_vector;
using cards::extended_deck_distribution;
using cards::extended_deck_count_type;
using cards::deck_count_type;
using cards::state_machine;
using cards::counter;

typedef	std::pair<string, counter>		named_counter;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	This maintains the effective count from the player's perspective.
	The observed count may differ slightly from the actual count, 
	due to partial information about discards.  
	The deck_state maintains the actual count of remaining cards
	unbeknownst to the player.
	These counts may differ due to discarding of peeked hole cards.
	This distribution information is used to compute the
	odds as seen by the player, using Bayesian conditional weights.
 */
class perceived_deck_state {
protected:
	/**
		All 10 cards are lumped together, no distinction required.
		This contains certain the number of cards.
		Unknown discards are not counted.
	 */
	deck_count_type				remaining;
	/**
		Partial information.
		Number of discarded peeked cards that are not 10s.
		Revealed cards no longer count as peeked.
	 */
	size_t					peeked_not_10s;
	/**
		Partial information.
		Number of discarded peeked cards that are not Aces.
		Revealed cards no longer count as peeked.
	 */
	size_t					peeked_not_Aces;
#if 0
	// non-peeked discards offer no information, and need not be counted
	size_t					discards;
#endif
	/**
		property: remaining.sum().
	 */
	size_t					remaining_total;

public:
	perceived_deck_state();

	void
	initialize(const extended_deck_count_type&);

	void
	remove(const size_t);	// translate with card_value_map

	void
	peek_not_10(void);

	void
	peek_not_Ace(void);

	void
	reveal_peek_10(const size_t);

	void
	reveal_peek_Ace(const size_t);

	/**
		Peeked unknown discards are not counted as removed
		from the remaining_total count.
	 */
	size_t
	actual_remaining(void) const {
		return remaining_total -peeked_not_10s -peeked_not_Aces;
	}

	void
	distribution_weight_adjustment(deck_count_type&) const;

	void
	distribution_weight_adjustment(extended_deck_count_type&) const;

	// re-weighted distribution using peek information, see math paper
	void
	effective_distribution(deck_count_type&) const;

	void
	effective_distribution(extended_deck_count_type&) const;

	bool
	operator < (const perceived_deck_state&) const;

};	// end class perceived_deck_state

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Tracks actual number of cards remaining in deck(s).
	Also the configuration of the game as far as deck size
	and reshuffle policy.
	Retains a hole card.
	When a hole card is drawn, the count is NOT updated
	because the value is unknown.
	When it is the dealer's turn to play, the hole card is
	then revealed and the count is updated.
	TODO: with_replacement mode to simulate infinite deck.
 */
class deck_state {
public:
	typedef	extended_deck_count_type	_deck_count_type;
	typedef	extended_deck_distribution	_deck_distribution;
private:
	/**
		This is needed to replenish deck when reshuffling.
	 */
	size_t					num_decks;
	/**
		Already seen cards (discarded)
	 */
	_deck_count_type			used_cards;
	/**
		Sequence of integers representing count of cards 
		remaining in deck or shoe.
	 */
	_deck_count_type			cards;
	/**
		Dealer is dealt one hole card (face-down).
	 */
	size_t					hole_card;
	/**
		When a hole card is drawn reserved, the remaining
		cards needs to be updated for random drawing purpose,
		but not for analysis purposes.
	 */
	bool					hole_reserved;
	/**
		Countdown of cards remaining.
	 */
	size_t					cards_remaining;
	size_t					cards_spent;
	/**
		When cards_remaining falls below this, reshuffle.
		Maximum allowed should be .90 (90%)
	 */
	size_t					maximum_penetration;

	/// TODO: support extendable vector of various counters
	named_counter				hi_lo_counter;
public:
	explicit
	deck_state(const variation&);

//	void
//	count(const size_t);

	const _deck_count_type&
	get_card_counts(void) const {
		return cards;		// remaining
	}

	double
	draw_ten_probability(void) const;

	void
	magic_draw(const size_t);

	size_t
	quick_draw(void);

	size_t
	quick_draw_uncounted(void);

	size_t
	quick_draw_uncounted_except(const size_t);

	// TODO: support post-peek conditions for drawing probabilities

	size_t
	option_draw_uncounted(const bool, istream&, ostream&, const bool);

	size_t
	option_draw(const bool, istream&, ostream&);

	size_t
	draw(void);

	void
	draw_hole_card(void);

	void
	option_draw_hole_card(const bool, istream&, ostream&);

	size_t
	peek_hole_card(void) const {
		return hole_card;
	}

	size_t
	reveal_hole_card(void);

	void
	replace_hole_card(void);

	bool
	needs_shuffle(void) const;

	void
	reshuffle(void);

	bool
	reshuffle_auto(void);

	ostream&
	show_count(ostream&, const bool, const bool) const;

	void
	quiz_count(istream&, ostream&) const;

#if 0
	ostream&
	show_simple_count(ostream& o) const {
		return show_count(o, false, true);
	}

	ostream&
	show_extended_count(ostream& o) const {
		return show_count(o, true, true);
	}
#endif

	bool
	edit_deck(const size_t, const int);

	bool
	edit_deck_add(const size_t, const int);

	bool
	edit_deck_sub(const size_t, const int);

	bool
	edit_deck_all(istream&, ostream&);

private:
	static
	bool
	__check_edit_deck_args(const size_t, const int);

};	// end class deck_state

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BOC2_DECK_STATE_HH__

