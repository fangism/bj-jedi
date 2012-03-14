// "deck.cc"

#include "deck.hh"
#include "util/array.tcc"

namespace cards {
//-----------------------------------------------------------------------------
// enumeration-dependent interface
const char card_name[card_symbols+1] = "A23456789TJQK";

// really should just be a public function
size_t
card_index(const char c) {
	if (std::isalpha(c)) {
		switch (std::toupper(c)) {
		case 'A': return ACE;
		case 'T': return TEN;
		case 'J': return JACK;
		case 'Q': return QUEEN;
		case 'K': return KING;
		default: break;
		}
	} else if (c >= '2' && c <= '9') {
		return c -'1';
	}
	return size_t(-1);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// maps JACK-KING to TEN
const size_t
card_value_map[card_symbols] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 9, 9 };

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Permute the ordering of columns for the dealer's reveal card.
	This ordering is standard in the literature, and is easier
	to read when considering favorable vs. unfavorable cards.
	index: sequence index
	value: card index (re-ordered)
 */
const size_t
reveal_print_ordering[card_values] =
	{ 1, 2, 3, 4, 5, 6, 7, 8, TEN, ACE};

const size_t
extended_reveal_print_ordering[card_symbols] =
	{ 1, 2, 3, 4, 5, 6, 7, 8, TEN, JACK, QUEEN, KING, ACE};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static const size_t __standard_deck_count_reduced[card_values] = {
1,		// A
1,		// 2
1,		// 3
1,		// 4
1,		// 5
1,		// 6
1,		// 7
1,		// 8
1,		// 9
4		// 10,J,Q,K
};

const deck_count_type
standard_deck_count_reduced(__standard_deck_count_reduced);

static const size_t __standard_deck_count[card_values] = {
4*1,		// A
4*1,		// 2
4*1,		// 3
4*1,		// 4
4*1,		// 5
4*1,		// 6
4*1,		// 7
4*1,		// 8
4*1,		// 9
4*4		// 10,J,Q,K
};

const deck_count_type
standard_deck_count(__standard_deck_count);

static const probability_type __standard_deck_distribution[card_values] = {
1.0/13.0,		// A
1.0/13.0,		// 2
1.0/13.0,		// 3
1.0/13.0,		// 4
1.0/13.0,		// 5
1.0/13.0,		// 6
1.0/13.0,		// 7
1.0/13.0,		// 8
1.0/13.0,		// 9
4.0/13.0		// 10,J,Q,K
};

const deck_distribution
standard_deck_distribution(__standard_deck_distribution);

}	// end namespace cards

