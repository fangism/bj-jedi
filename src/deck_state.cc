// "deck_state.cc"

#include <string>
#include <iostream>
#include <iomanip>

#include "deck_state.hh"
#include "variation.hh"
#include "util/array.tcc"
#include "util/probability.tcc"
#include "util/value_saver.hh"
#include "util/string.tcc"		// for string_to_num
#include "util/tokenize.hh"

namespace blackjack {
using std::string;
using std::setw;
using std::cerr;
using std::endl;
#if DECK_PROBABILITIES
using util::normalize;
#endif
using util::string_list;
using util::tokenize;
using util::strings::string_to_num;
using cards::ACE;
using cards::TEN;
using cards::card_values;
using cards::card_name;
using cards::standard_deck_distribution;
using cards::reveal_print_ordering;
using cards::card_index;

//=============================================================================
// class deck_state method definitions

/**
	Initializes to default.
 */
deck_state::deck_state(const variation& v) : 
		num_decks(v.num_decks)
#if DECK_PROBABILITIES
		, card_probabilities(card_values), 
		need_update(true)
#endif
		{
	reshuffle();		// does most of the initializing
	// default penetration before reshuffling: 75%
	maximum_penetration = (1.0 -v.maximum_penetration) * num_decks *52;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
deck_state::reshuffle(void) {
	// ACE is at index 0
	cards_remaining = num_decks*52;
	cards_spent = 0;
	const size_t f = num_decks*4;
	std::fill(used_cards.begin(), used_cards.end(), 0);
	std::fill(cards.begin(), cards.end() -1, f);
	cards[TEN] = f*4;	// T, J, Q, K
#if DECK_PROBABILITIES
	need_update = true;
	update_probabilities();
#endif
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\return true if deck/shoe was shuffled.
 */
bool
deck_state::reshuffle_auto(void) {
	if (cards_remaining < maximum_penetration) {
		reshuffle();
		return true;
	}
	return false;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double
deck_state::draw_ten_probability(void) const {
	return double(cards[TEN])/double(cards_remaining);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if DECK_PROBABILITIES
void
deck_state::update_probabilities(void) {
	if (need_update) {
		assert(card_probabilities.size() == cards.size());
		normalize(card_probabilities, cards);
		need_update = false;
	}
}
#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if 0
/**
	\param d the card index of the counter to decrement.
 */
void
deck_state::count(const size_t c) {
	assert(cards[c]);
	--cards[c];
}
#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Draws the user-determined card, useful for setting up
	and querying specific scenarios.
	Counts specified card as drawn from deck.
 */
void
deck_state::magic_draw(const size_t r) {
	// TODO: safe-guard against drawing non-existent
	assert(cards[r]);
	++used_cards[r];
	--cards[r];
	++cards_spent;
	--cards_remaining;
#if DECK_PROBABILITIES
	need_update = true;
#endif
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Removes one card from remaining deck at random.
	Does NOT re-compute probabilities.
 */
size_t
deck_state::quick_draw(void) {
	const size_t ret = quick_draw_uncounted();
	magic_draw(ret);
	return ret;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Peeks at one card from remaining deck at random.
	Does NOT re-compute real-valued probabilities; just uses integers.
 */
size_t
deck_state::quick_draw_uncounted(void) {
#if 0
	return util::random_draw_from_real_pdf(card_probabilities);
#else
	return util::random_draw_from_integer_pdf(cards);
#endif
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Zeros out the card indexed i, for the purposes of 
	random drawing, knowing that the card cannot be 
	of a particular value.  
	This is useful after peeking for blackjack.  
	\return index of a random card.
 */
size_t
deck_state::quick_draw_uncounted_except(const size_t i) {
	const util::value_saver<size_t> __tmp(cards[i], 0);
	return quick_draw_uncounted();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Gives user to choose her own card.
	Useful for specific scenario testing.
	\param m if true, prompt user to choose a specific card.
	\param s if true, show result of random draw.
 */
size_t
deck_state::option_draw_uncounted(const bool m, istream& i, ostream& o,
		const bool s) {
if (m) {
	string line;
	do {
		o << "card? ";
		i >> line;
	} while (line.empty() && i);
	const size_t c = card_index(line[0]);
	if (c != size_t(-1)) {
		assert(c < card_values);
		assert(cards[c]);		// count check
		return c;
	} else {
		const size_t ret = quick_draw_uncounted();
		o << "drawing randomly... ";
		if (s) {
			o << card_name[ret];
		}
		o << endl;
		return ret;
	}
} else {
	return quick_draw_uncounted();
}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t
deck_state::option_draw(const bool m, istream& i, ostream& o) {
	const size_t ret = option_draw_uncounted(m, i, o, true);
	magic_draw(ret);
	return ret;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Removes one card from remaining deck at random.
 */
size_t
deck_state::draw(void) {
	const size_t ret = quick_draw();
#if DECK_PROBABILITIES
	update_probabilities();
#endif
	return ret;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
deck_state::draw_hole_card(void) {
	hole_card = quick_draw_uncounted();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
deck_state::option_draw_hole_card(const bool m, istream& i, ostream& o) {
	hole_card = option_draw_uncounted(m, i, o, false);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	This should only be called once per hole card draw.
 */
size_t
deck_state::reveal_hole_card(void) {
	magic_draw(hole_card);
	return hole_card;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
deck_state::show_count(ostream& o) const {
	const size_t p = o.precision(2);
	size_t i = 0;
	o << "card:\t";
	for (i=0 ; i<card_values; ++i) {
		const size_t j = reveal_print_ordering[i];
		o << "   " << card_name[j];
	}
	o << "\ttotal\t%" << endl;
	o << "used:\t";
	for (i=0 ; i<card_values; ++i) {
		const size_t j = reveal_print_ordering[i];
		o << setw(4) << used_cards[j];
	}
	o << "\t" << cards_spent << "\t(" <<
		double(cards_spent) *100.0 / (num_decks *52) << "%)\n";
	o << "rem:\t";
	for (i=0; i<card_values; ++i) {
		const size_t j = reveal_print_ordering[i];
		o << setw(4) << cards[j];
	}
	// note: altered decks will have nonsense values
	o << "\t" << cards_remaining << "\t(" <<
		double(cards_remaining) *100.0 / (num_decks *52) << "%)\n";
	// hi-lo count summary, details of true count, running count
	int hi_lo_count = 0;
	// more accurately, based on remaining cards, not used cards
	// this allows meaningful counting for altered decks
	hi_lo_count -= cards[1] +cards[2] +cards[3]
		+cards[4] +cards[5];	// 2 through 6
	hi_lo_count += cards[ACE] +cards[TEN];
	double true_count = (double(hi_lo_count) * 52)
		/ double(cards_remaining);
	o << "hi-lo: " << hi_lo_count <<
	// true count adjusts for number of cards remaining
	// normalized to 1 deck
		", true-count: " << true_count;
	// adjusted count accounts for the house edge based on rules
	o.precision(p);
	return o << endl;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool
deck_state::__check_edit_deck_args(const size_t c, const int n) {
	if (c >= card_values) {
		cerr << "error: invalid card index " << c << endl;
		return true;
	}
	if (n < 0) {
		cerr << "error: resulting quantity must be non-negative"
			<< endl;
		return true;
	}
	return false;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\param c the card index
	\param n the number of those cards to keep in deck
 */
bool
deck_state::edit_deck(const size_t c, const int n) {
	if (__check_edit_deck_args(c, n)) {
		return true;
	}
	const int d = n -int(cards[c]);
	cards[c] = n;
	cards_remaining += d;
	// leave used_cards and cards_spent alone
#if DECK_PROBABILITIES
	need_update = true;
#endif
	return false;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool
deck_state::edit_deck_add(const size_t c, const int n) {
	if (c >= card_values) {
		cerr << "error: invalid card index " << c << endl;
		return true;
	}
	return edit_deck(c, cards[c] +n);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool
deck_state::edit_deck_sub(const size_t c, const int n) {
	if (c >= card_values) {
		cerr << "error: invalid card index " << c << endl;
		return true;
	}
	return edit_deck(c, cards[c] -n);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Interactively edit deck distribution for each card.
	\return true if there is an error.
 */
bool
deck_state::edit_deck_all(istream& i, ostream& o) {
	o << "Enter quantity of each card.\n"
"\t. for unchanged\n"
"\t[+|-] value : for relative count change" << endl;
	size_t j = 0;
for ( ; j<card_values; ++j) {
	const size_t k = reveal_print_ordering[j];
	bool accept = true;
	do {
	o << "card: " << card_name[k] << " (" << cards[k] << "): ";
	string line;
	getline(i, line);
	string_list args;
	tokenize(line, args);
	switch (args.size()) {
	case 1: {
		const string& qs(args.front());
		int q;
		if (qs == ".") {
			// accepted, unchanged
		} else if (string_to_num(qs, q)) {
			o << "error: invalid number" << endl;
			accept = false;
		} else if (edit_deck(k, q)) {
			// there was error, prompt again
			accept = false;
		}
		break;
	}
	case 2: {
		int q;
		const string& sn(args.front());
		const string& qs(args.back());
		if (string_to_num(qs, q)) {
			o << "error: invalid number" << endl;
			accept = false;
		}
		switch (sn[0]) {
		case '+':
			accept = !edit_deck_add(k, q);
			break;
		case '-':
			accept = !edit_deck_sub(k, q);
			break;
		default:
			accept = false;
			break;
		}	// end switch
		break;
	}
	default:
		accept = false;
		break;
	}	// end switch
	} while (i && !accept);
}	// end for all cards
	return !i;
}	// end edit_deck_all

//=============================================================================
}	// end namespace blackjack

