/**
	\file "counter.cc"
	$Id: $
 */

#include <iostream>
#include <algorithm>
#include <numeric>
#include "counter.hh"
#include "util/array.tcc"
#include "util/iosfmt_saver.hh"

namespace cards {
using std::transform;
using std::inner_product;

//=============================================================================
// class counter_base method definitions

counter_base::counter_base(const count_signature_type& c) : signature(c) {
	// TODO: check for balanced count
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\param c coefficients (weights), typically positive for good, 
		negative for bad, in terms of remaining proportion in deck.
 */
counter_base::counter_base(const int c[card_values]) : signature(c) { }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\return true if the signature weights are balanced.
 */
bool
counter_base::signature_balanced(void) const {
	return !inner_product(signature.begin(), signature.end(), 
		standard_deck_count_reduced.begin(), 0);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\return the count representing the card imbalance, based on 
		the number of cards remaining.
 */
int
counter_base::evaluate(const deck_count_type& d) const {
	return inner_product(signature.begin(), signature.end(), d.begin(), 0);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int
counter_base::evaluate(const extended_deck_count_type& e) const {
	deck_count_type d;
	simplify_deck_count(e, d);	// combin T,J,Q,K
	return inner_product(signature.begin(), signature.end(), d.begin(), 0);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\param name an identifier for this counter.
	\param rem total number of cards remaining, used to obtain true-count.
		This is not necessarily == the sum, due to perceived_deck_state.
 */
ostream&
counter_base::dump_count(ostream& o, const char* name, const size_t rem, 
		const int running_count) const {
	const util::precision_saver p(o, 2);	// fix precision
	const double true_count = (double(running_count) * 52) / double(rem);
	o << name << ": " << running_count <<
	// true count adjusts for number of cards remaining
	// normalized to 1 deck
		", true-count: " << true_count;
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Auto-computes running-count based on deck distribution.
 */
ostream&
counter_base::dump_count(ostream& o, const char* name, const size_t rem, 
		const deck_count_type& d) const {
	return dump_count(o, name, rem, evaluate(d));
}

//=============================================================================
// class counter method definitions

// order depends on card_value enumeration (ACE, TEN)
const int __hi_lo_signature[card_values] =
	{ 1, -1, -1, -1, -1, -1, 0, 0, 0, 1 };

const count_signature_type
hi_lo_signature(__hi_lo_signature);

/**
	\param c coefficients (weights), typically positive for good, 
		negative for bad, in terms of remaining proportion in deck.
 */
counter::counter(const int c[card_values]) :
		counter_base(c), running_count(0) {
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
counter::counter(const count_signature_type& c) :
		counter_base(c), running_count(0) {
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
	Need to re-update count after editing the deck.
 */
void
counter::update(const deck_count_type& d) {
	running_count = evaluate(d);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Need to re-update count after editing the deck.
 */
void
counter::update(const extended_deck_count_type& d) {
	running_count = evaluate(d);
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
#if 0
	const util::precision_saver p(o, 2);	// fix precision
	const double true_count = (double(running_count) * 52) / double(rem);
	o << name << ": " << running_count <<
	// true count adjusts for number of cards remaining
	// normalized to 1 deck
		", true-count: " << true_count;
	return o;
#else
	return dump_count(o, name, rem, running_count);
#endif
}

//=============================================================================
}	// end namespace cards
