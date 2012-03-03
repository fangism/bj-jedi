// "deck_state.hh"

#ifndef	__BOC2_DECK_STATE_HH__
#define	__BOC2_DECK_STATE_HH__

#include "card_state.hh"

namespace blackjack {
class variation;
using std::istream;
using std::ostream;
using cards::probability_vector;
using cards::deck_distribution;
using cards::deck_count_type;
using cards::state_machine;

/**
	Continuously track real-valued probabilities as cards
	are drawn from deck.
	Just get rid of this, no need for it.
 */
#define	DECK_PROBABILITIES			0

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
 */
class deck_state {
	size_t					num_decks;
	/**
		Already seen cards (discarded)
	 */
	deck_count_type				used_cards;
	/**
		Sequence of integers representing count of cards 
		remaining in deck or shoe.
	 */
	deck_count_type				cards;
#if DECK_PROBABILITIES
	/**
		This is updated everytime cards change, e.g. when
		a single card is drawn.
	 */
	deck_distribution			card_probabilities;
#endif
	/**
		Dealer is dealt one hole card (face-down).
	 */
	size_t					hole_card;
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
#if DECK_PROBABILITIES
	/**
		Flag that determines whether or not card_probabilities
		need to be updated (after cards were drawn).  
	 */
	bool					need_update;
#endif
public:
	explicit
	deck_state(const variation&);

//	void
//	count(const size_t);

	const deck_count_type&
	get_card_counts(void) const {
		return cards;		// remaining
	}

#if DECK_PROBABILITIES
	const deck_distribution&
	get_card_probabilities(void) const {
		return card_probabilities;
	}
#endif
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
	reshuffle(void);

	bool
	reshuffle_auto(void);

	ostream&
	show_count(ostream&) const;

#if DECK_PROBABILITIES
	void
	update_probabilities(void);
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

