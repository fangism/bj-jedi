// "deck.hh"

#ifndef	__BOC2_DECK_HH__
#define	__BOC2_DECK_HH__

#include "util/array.hh"

// namespace? global namespace?
namespace cards {

typedef	double			probability_type;
typedef	probability_type	edge_type;

enum {
	ACE = 0,		// index of Ace (card_odds)
	TEN = 9,		// index of 10 (T) (card_odds)
	card_values = 10	// number of unique card values (TJQK = 10)
};
extern	const char		card_name[];	// use util::array?
extern	size_t		card_index(const char);	// reverse-map of card_name

extern	const size_t		reveal_print_ordering[];

/**
	A deck is just modeled as a probability vector.  
	\invariant sum of probabilities should be 1.
 */
typedef	util::array<probability_type, card_values>	deck_distribution;
typedef	util::array<size_t, card_values>	deck_count_type;

// 13 card values, 2 through A
extern	const deck_count_type			standard_deck_count_reduced;
extern	const deck_count_type			standard_deck_count;
extern	const deck_distribution			standard_deck_distribution;

}	// end namespace cards

#endif	// __BOC2_DECK_HH__

