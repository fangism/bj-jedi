// "outcome.hh"

#ifndef	__BOC2_OUTCOME_HH__
#define	__BOC2_OUTCOME_HH__

#include <iosfwd>
#include "num.hh"
#include "enums.hh"
#include "util/array.hh"

namespace blackjack {
using std::ostream;
using util::array;
using cards::probability_type;


/**
	Define to 1 to have win/push/lose as an enum-indexed array.
	Status: perm'd
 */
// #define	INDEXED_WLP				1

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Dealer vs. player showdown outcomes.  
	Enumeration values matter for outcome indexed field.
 */
enum outcome {
	WIN = 0,
	PUSH = 1,
	LOSE = 2
};

extern
ostream&
operator << (ostream&, const outcome);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Represents the edge of a given situation with probability
	of winning, losing, tying.
	Sum of probabilities should be 1.0.
	TODO: array of size 3, using enum in enums.hh.
 */
struct outcome_odds {
	probability_type		_prob[3];

	// initialize ctor
	outcome_odds() { _prob[0] = _prob[1] = _prob[2] = 0.0; }

	probability_type&
	prob(const outcome o) {
		return _prob[o];
	}

	const probability_type&
	prob(const outcome o) const {
		return _prob[o];
	}

	probability_type&
	win(void) { return _prob[WIN]; }

	const probability_type&
	win(void) const { return _prob[WIN]; }

	probability_type&
	push(void) { return _prob[PUSH]; }

	const probability_type&
	push(void) const { return _prob[PUSH]; }

	probability_type&
	lose(void) { return _prob[LOSE]; }

	const probability_type&
	lose(void) const { return _prob[LOSE]; }

	probability_type
	edge(void) const { return win() -lose(); }

	// asymmetric edge when payoff is != investment
	probability_type
	weighted_edge(const probability_type& w,
			const probability_type& l) const {
		return w*win() -l*lose();
	}

	// since win + lose <= 1, we may discard the tie (push) factor
	probability_type
	ratioed_edge(void) const { return (win() -lose())/(win() +lose()); }

	void
	check(void);
};	// end struct outcome_odds

typedef	array<outcome_odds, p_final_states>	outcome_vector;

extern
ostream&
dump_outcome_vector(const outcome_vector&, ostream&);

extern
ostream&
operator << (ostream&, const outcome_odds&);

typedef array<probability_type, d_final_states>	dealer_final_vector;

extern
ostream&
dump_dealer_final_vector(ostream&, const dealer_final_vector&);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BOC2_OUTCOME_HH__

