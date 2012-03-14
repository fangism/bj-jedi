// "deck.hh"

#ifndef	__BOC2_DECK_HH__
#define	__BOC2_DECK_HH__

#include "util/array.hh"

/**
	Define to 1 to count/show face cards separately.
	Status: basically tested
 */
#define	FACE_CARDS				1


// namespace? global namespace?
namespace cards {

typedef	double			probability_type;
typedef	probability_type	edge_type;

enum {
	ACE = 0,		// index of Ace (card_odds)
	TEN = 9,		// index of 10 (T) (card_odds)
	card_values = 10,	// number of unique card values (TJQK = 10)
#if FACE_CARDS
	JACK = TEN+1,
	QUEEN = JACK+1,
	KING = QUEEN+1,
	card_symbols = KING+1	// number of unique card symbols, (+JQK)
#else
	// alias face card enums to 10
	JACK = TEN,
	QUEEN = TEN,
	KING = TEN
#endif
};
extern	const char		card_name[];	// use util::array?
extern	size_t		card_index(const char);	// reverse-map of card_name

extern	const size_t		reveal_print_ordering[];
#if FACE_CARDS
extern	const size_t		card_value_map[card_symbols];
extern	const size_t		extended_reveal_print_ordering[];
#endif

/**
	A deck is just modeled as a probability vector.  
	\invariant sum of probabilities should be 1.
 */
typedef	util::array<probability_type, card_values>	deck_distribution;
typedef	util::array<size_t, card_values>	deck_count_type;

#if FACE_CARDS
typedef	util::array<probability_type, card_symbols>	extended_deck_distribution;
typedef	util::array<size_t, card_symbols>	extended_deck_count_type;
#endif

// 13 card values, 2 through A
extern	const deck_count_type			standard_deck_count_reduced;
extern	const deck_count_type			standard_deck_count;
extern	const deck_distribution			standard_deck_distribution;

#if 0 && FACE_CARDS
extern	const extended_deck_count_type		extended_standard_deck_count_reduced;
extern	const extended_deck_count_type		extended_standard_deck_count;
extern	const extended_deck_distribution	extended_standard_deck_distribution;
#endif

}	// end namespace cards

#endif	// __BOC2_DECK_HH__

