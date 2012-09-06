/**
	\file "counter.hh"
	$Id: $
 */

#ifndef	__BJ_COUNTER_HH__
#define	__BJ_COUNTER_HH__

#include <iosfwd>
#include <utility>
#include <string>

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
	Standard counting systems.
	Weighting coefficients for standard hi-lo count.
	A,T count as +1
	2-6 count as -1
 */
extern
const count_signature_type			hi_lo_signature;
// extern
// const count_signature_type			KO_signature;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Just a wrapper class around the count_signature.
 */
class counter_base {
protected:
	count_signature_type			signature;

public:
	// constructor: initialize with deck counts, optional offset?
	explicit
	counter_base(const int[card_values]);

	counter_base(const count_signature_type&);

	bool
	signature_balanced(void) const;

	int
	evaluate(const deck_count_type&) const;

	int
	evaluate(const extended_deck_count_type&) const;

	ostream&
	dump_count(ostream&, const char*, const size_t, 
		const int) const;

	ostream&
	dump_count(ostream&, const char*, const size_t, 
		const deck_count_type&) const;

};	// end class counter_base

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	A counter class maintains a running count based on cards seen.
	The running count is a one-dimensional scalar.
 */
class counter : public counter_base {
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
	update(const extended_deck_count_type&);

	void
	update(const deck_count_type&);

	int
	get_running_count(void) const { return running_count; }

	void
	incremental_count_card(const size_t);

	bool
	deck_signature_balanced(const extended_deck_count_type&) const;

	ostream&
	dump(ostream&, const char*, const size_t) const;

};	// end class counter

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
typedef	std::pair<std::string, counter>		named_counter;

}	// end namespace cards

#endif	// __BJ_COUNTER_HH__
