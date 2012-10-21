// "bj/core/deck_state.hh"

#ifndef	__BJ_CORE_DECK_STATE_HH__
#define	__BJ_CORE_DECK_STATE_HH__

#include "bj/core/card_state.hh"
#include "bj/core/counter.hh"

namespace blackjack {
struct variation;
using std::istream;
using std::ostream;
using std::string;
using cards::probability_vector;
using cards::extended_deck_distribution;
using cards::extended_deck_count_type;
using cards::deck_count_type;
using cards::state_machine;
using cards::counter;
using cards::count_type;
using cards::card_type;

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

	Technically, card-counting should be based on perceived count.
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
	count_type				peeked_not_10s;
	/**
		Partial information.
		Number of discarded peeked cards that are not Aces.
		Revealed cards no longer count as peeked.
	 */
	count_type				peeked_not_Aces;
#if 0
	// non-peeked discards offer no information, and need not be counted
	count_type				discards;
#endif
	/**
		property: remaining.sum().
	 */
	count_type				remaining_total;
public:
	perceived_deck_state();

	explicit
	perceived_deck_state(const size_t n);

	// remove card from dist, and copy
	perceived_deck_state(const perceived_deck_state&, const card_type);

	void
	initialize(const extended_deck_count_type&);

	void
	initialize_num_decks(const size_t);

	void
	scale_cards(const size_t, const size_t);

	const deck_count_type&
	get_counts(void) const {
		return remaining;
	}

	void
	add(const card_type, const count_type n = 1);	// translate with card_value_map

	void
	remove(const card_type);	// translate with card_value_map

	bool
	remove_if_any(const card_type);	// translate with card_value_map

	void
	remove_all(const card_type);	// translate with card_value_map

	/**
		Increment number of peeked non-10 cards.
		if peeked card is discarded, it may not be revealed!
	 */
	void
	peek_not_10(void) {
		++peeked_not_10s;
	}

	void
	peek_not_Ace(void) {
		++peeked_not_Aces;
	}

	void
	unpeek_not_10(void) {
		--peeked_not_10s;
	}

	void
	unpeek_not_Ace(void) {
		--peeked_not_Aces;
	}

	void
	reveal_peek_10(const card_type);

	void
	reveal_peek_Ace(const card_type);

	void
	reveal(const peek_state_enum, const card_type);

	/**
		Peeked unknown discards are not counted as removed
		from the remaining_total count.
	 */
	count_type
	actual_remaining(void) const {
		return remaining_total -peeked_not_10s -peeked_not_Aces;
	}

	count_type
	get_remaining_total(void) const {
		return remaining_total;
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

	int
	compare(const perceived_deck_state&) const;

	bool
	operator < (const perceived_deck_state&) const;

	ostream&
	show_count_brief(ostream&) const;

	ostream&
	show_count(ostream&) const;

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
	card_type				hole_card;
	/**
		When a hole card is drawn reserved, the remaining
		cards needs to be updated for random drawing purpose,
		but not for analysis purposes.
	 */
	bool					hole_reserved;
	/**
		Countdown of cards remaining.
		This is the actual count, which may include
		discarded cards that the player never sees.
	 */
	count_type				cards_remaining;
	count_type				cards_spent;
	/**
		When cards_remaining falls below this, reshuffle.
		Maximum allowed should be .90 (90%)
	 */
	count_type				maximum_penetration;
public:
	explicit
	deck_state(const variation&);

//	void
//	count(const size_t);

	const _deck_count_type&
	get_card_counts(void) const {
		return cards;		// remaining
	}

	count_type
	get_cards_remaining(void) const {
		return cards_remaining;
	}

	double
	draw_ten_probability(void) const;

	void
	magic_draw(const card_type);

	card_type
	quick_draw(void);

	card_type
	quick_draw_uncounted(void);

	card_type
	quick_draw_uncounted_except(const card_type);

	card_type
	option_draw_uncounted(const bool, istream&, ostream&, const bool);

	card_type
	option_draw(const bool, istream&, ostream&);

	card_type
	draw(void);

	void
	draw_hole_card(void);

	void
	option_draw_hole_card(const bool, istream&, ostream&);

	card_type
	peek_hole_card(void) const {
		return hole_card;
	}

	card_type
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
	edit_deck(const card_type, const int);

	bool
	edit_deck_add(const card_type, const int);

	bool
	edit_deck_sub(const card_type, const int);

	bool
	edit_deck_all(istream&, ostream&);

private:
	static
	bool
	__check_edit_deck_args(const card_type, const int);

};	// end class deck_state

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BJ_CORE_DECK_STATE_HH__

