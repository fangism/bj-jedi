/**
	\file "counter.hh"
	$Id: $
 */

#ifndef	__BJ_COUNTER_HH__
#define	__BJ_COUNTER_HH__

#include <iosfwd>
#include "enums.hh"
#include "util/array.hh"
#include "deck.hh"

namespace cards {
using std::ostream;

/**
	A count signature consists of integer weights for each
	card value, which may be positive or negative.  
	Count is based on *remaining* cards in deck, 
	as opposed to seen/used cards.
	A balanced count signature has coefficients add up to 0.
	TODO: support fractional weights?
 */
typedef	util::array<int, card_values>		count_signature_type;

/**
	A,T count as +1
	2-6 count as -1
 */
extern
const count_signature_type			hi_lo_signature;

/**
	A counter class maintains a running count based on cards seen.
	The running count is a one-dimensional scalar.
 */
class counter {
	count_signature_type			signature;
	int					running_count;
public:
	// constructor: initialize with deck counts, optional offset?
	explicit
	counter(const int[card_values]);

	explicit
	counter(const count_signature_type&);

	void
	initialize(const extended_deck_count_type&);

	void
	initialize(const deck_count_type&);

	void
	incremental_count_card(const size_t);

	bool
	signature_balanced(void) const;

	bool
	deck_signature_balanced(const extended_deck_count_type&) const;

	ostream&
	dump(ostream&, const char*, const size_t) const;

};	// end class counter

}	// end namespace cards

#endif	// __BJ_COUNTER_HH__
