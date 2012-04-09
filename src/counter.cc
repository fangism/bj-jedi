/**
	\file "counter.cc"
	$Id: $
 */

#include <iostream>
#include <algorithm>
#include <numeric>
#include "counter.hh"
#include "util/array.tcc"

namespace cards {
using std::transform;
using std::inner_product;

//=============================================================================
// class counter method definitions

const int __hi_lo_signature[card_values] =
	{ 1, -1, -1, -1, -1, -1, 0, 0, 0, 1 };

const count_signature_type
hi_lo_signature(__hi_lo_signature);

/**
	\param c coefficients (weights), typically positive for good, 
		negative for bad, in terms of remaining proportion in deck.
 */
counter::counter(const int c[card_values]) : signature(c), running_count(0) {
	// TODO: check for balanced count
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
counter::counter(const count_signature_type& c) : signature(c), running_count(0) {
	// TODO: check for balanced count
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\return true if the signature weights are balanced.
 */
bool
counter::signature_balanced(void) const {
	return !inner_product(signature.begin(), signature.end(), 
		standard_deck_count_reduced.begin(), 0);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\param d is a count for a full deck.
 */
bool
counter::deck_signature_balanced(const extended_deck_count_type& d) const {
	deck_count_type rd;
	simplify_deck_count(d, rd);	// collect 10-valued cards
	return !inner_product(signature.begin(), signature.end(), rd.begin(), 0);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\param c card seen (and thus discarded and counted).
	c may include face cards J,Q,K, which get mapped to T.
 */
void
counter::incremental_count_card(const size_t c) {
	const size_t d = card_value_map[c];
	running_count -= signature[d];
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\param d distribution of cards remaining in deck.
 */
void
counter::initialize(const deck_count_type& d) {
	running_count = inner_product(signature.begin(), signature.end(),
		d.begin(), 0);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\param d distribution of cards (extended) remaining in deck.
 */
void
counter::initialize(const extended_deck_count_type& d) {
	deck_count_type rd;
	simplify_deck_count(d, rd);	// collect 10-valued cards
	initialize(rd);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\param name an identifier for this counter.
	\param rem total number of cards remaining, used to obtain true-count.
 */
ostream&
counter::dump(ostream& o, const char* name, const size_t rem) const {
	const double true_count = (double(running_count) * 52) / double(rem);
	o << name << ": " << running_count <<
	// true count adjusts for number of cards remaining
	// normalized to 1 deck
		", true-count: " << true_count;
}

//=============================================================================
}	// end namespace cards
