// "deck.hh"

#ifndef	__BOC2_DECK_HH__
#define	__BOC2_DECK_HH__

#include "num.hh"
#include "util/array.hh"

namespace cards {

typedef	double			probability_type;
typedef	probability_type	edge_type;

enum {
	ACE = 0,		// index of Ace (card_odds)
	TEN = 9,		// index of 10 (T) (card_odds)
	card_values = 10,	// number of unique card values (TJQK = 10)
	// analysis only needs to use number of unique card_values
	// but game-play and simulation will use number of card_symbols
	JACK = TEN+1,
	QUEEN = JACK+1,
	KING = QUEEN+1,
	card_symbols = KING+1	// number of unique card symbols, (+JQK)
};
extern	const char		card_name[];	// use util::array?
extern	size_t		card_index(const char);	// reverse-map of card_name

extern	const size_t		reveal_print_ordering[];
extern	const size_t		card_value_map[card_symbols];
extern	const size_t		extended_reveal_print_ordering[];

/**
	A deck is just modeled as a probability vector.  
	\invariant sum of probabilities should be 1.
 */
typedef	util::array<probability_type, card_values>	deck_distribution;
typedef	util::array<size_t, card_values>	deck_count_type;

typedef	util::array<probability_type, card_symbols>	extended_deck_distribution;
typedef	util::array<size_t, card_symbols>	extended_deck_count_type;

// 13 card values, 2 through A
extern	const deck_count_type			standard_deck_count_reduced;
// the no_10 and no_Ace counts are used for post-peek conditions only
extern	const deck_count_type			standard_deck_count_reduced_no_10;
extern	const deck_count_type			standard_deck_count_reduced_no_Ace;

/// standard_deck_count.sum() == 52
extern	const deck_count_type			standard_deck_count;
/// standard_deck_distribution.sum() == 1.0
extern	const deck_distribution			standard_deck_distribution;

extern
void
simplify_deck_count(const extended_deck_count_type&, deck_count_type&);

}	// end namespace cards

#endif	// __BOC2_DECK_HH__

