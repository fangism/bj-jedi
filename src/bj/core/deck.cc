// "bj/core/deck.cc"

#include <algorithm>
#include <numeric>
#include "bj/core/deck.hh"
#include "util/array.tcc"

namespace cards {
using std::copy;
using std::accumulate;

//-----------------------------------------------------------------------------
// enumeration-dependent interface
const char card_name[card_symbols+1] = "A23456789TJQK";

// really should just be a public function
card_type
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
		// this is based on the enumeration in "deck.hh"
		return c -'1';
	}
	return card_type(INVALID_CARD);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// maps JACK-KING to TEN
const card_type
card_value_map[card_symbols] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 9, 9 };

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Permute the ordering of columns for the dealer's reveal card.
	This ordering is standard in the literature, and is easier
	to read when considering favorable vs. unfavorable cards.
	index: sequence index
	value: card index (re-ordered)
 */
const card_type
reveal_print_ordering[card_values] =
	{ 1, 2, 3, 4, 5, 6, 7, 8, TEN, ACE};

const card_type
extended_reveal_print_ordering[card_symbols] =
	{ 1, 2, 3, 4, 5, 6, 7, 8, TEN, JACK, QUEEN, KING, ACE};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static const count_type __standard_deck_count_reduced[card_values] = {
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

static const count_type __standard_deck_count_reduced_no_10[card_values] = {
1,		// A
1,		// 2
1,		// 3
1,		// 4
1,		// 5
1,		// 6
1,		// 7
1,		// 8
1,		// 9
0		// 10,J,Q,K
};

const deck_count_type
standard_deck_count_reduced_no_10(__standard_deck_count_reduced_no_10);

static const count_type __standard_deck_count_reduced_no_Ace[card_values] = {
0,		// A
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
standard_deck_count_reduced_no_Ace(__standard_deck_count_reduced_no_Ace);

static const count_type __standard_deck_count[card_values] = {
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

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Lumps all 10-valued cards together.
	Goes from size 13 array to size 10.
 */
void
simplify_deck_count(const extended_deck_count_type& d, deck_count_type& rd) {
	const extended_deck_count_type::const_iterator t(d.begin() +card_values);
	copy(d.begin(), t, rd.begin());
	rd.back() += accumulate(t, d.end(), 0);
}

}	// end namespace cards

