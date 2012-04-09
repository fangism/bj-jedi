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
using cards::state_machine;
using cards::counter;

typedef	std::pair<string, counter>		named_counter;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Tracks cards remaining in deck(s).
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

