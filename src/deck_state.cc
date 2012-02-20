// "deck_state.cc"

#include <string>
#include <iostream>
#include <iomanip>
#include "deck_state.hh"
#include "variation.hh"
#include "util/array.tcc"
#include "util/probability.tcc"

namespace blackjack {
using std::string;
using std::setw;
using std::endl;
using util::normalize;
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
		num_decks(v.num_decks), 
		card_probabilities(card_values), 
		need_update(true) {
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
	need_update = true;
	update_probabilities();
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
void
deck_state::update_probabilities(void) {
	if (need_update) {
		assert(card_probabilities.size() == cards.size());
		normalize(card_probabilities, cards);
		need_update = false;
	}
}

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
	need_update = true;
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
	Does NOT re-compute probabilities.
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
	Gives user to choose her own card.
	Useful for specific scenario testing.
	\param m if true, prompt user to choose a specific card.
 */
size_t
deck_state::option_draw_uncounted(const bool m, istream& i, ostream& o) {
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
		o << "drawing randomly." << endl;
		return quick_draw_uncounted();
	}
} else {
	return quick_draw_uncounted();
}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t
deck_state::option_draw(const bool m, istream& i, ostream& o) {
	const size_t ret = option_draw_uncounted(m, i, o);
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
	update_probabilities();
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
	hole_card = option_draw_uncounted(m, i, o);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	This should only be called once per hole card draw.
 */
size_t
deck_state::reveal_hole_card(void) {
	magic_draw(hole_card);
//	update_probabilities();
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
	o << "\t" << cards_remaining << "\t(" <<
		double(cards_remaining) *100.0 / (num_decks *52) << "%)\n";
	// hi-lo count summary, details of true count, running count
	int hi_lo_count = 0;
	hi_lo_count += used_cards[1] +used_cards[2] +used_cards[3]
		+used_cards[4] +used_cards[5];	// 2 through 6
	hi_lo_count -= used_cards[ACE] +used_cards[TEN];
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

//=============================================================================
}	// end namespace blackjack

