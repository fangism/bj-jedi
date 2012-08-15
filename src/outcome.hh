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
 */
#define	INDEXED_WLP				1

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Dealer vs. player showdown outcomes.  
 */
enum outcome {
	WIN = 0,
	PUSH = 1,
	LOSE = 2
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Represents the edge of a given situation with probability
	of winning, losing, tying.
	Sum of probabilities should be 1.0.
	TODO: array of size 3, using enum in enums.hh.
 */
struct outcome_odds {
#if INDEXED_WLP
	probability_type		_prob[3];
#else
	probability_type		_win;
	probability_type		_push;
	probability_type		_lose;
#endif

	// initialize ctor
#if INDEXED_WLP
	outcome_odds() { _prob[0] = _prob[1] = _prob[2] = 0.0; }
#else
	outcome_odds() : _win(0.0), _push(0.0), _lose(0.0) { }
#endif

#if INDEXED_WLP
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
#else
	probability_type&
	win(void) { return _win; }

	const probability_type&
	win(void) const { return _win; }

	probability_type&
	push(void) { return _push; }

	const probability_type&
	push(void) const { return _push; }

	probability_type&
	lose(void) { return _lose; }

	const probability_type&
	lose(void) const { return _lose; }
#endif

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

typedef	array<outcome_odds, player_states>	outcome_vector;

extern
ostream&
dump_outcome_vector(const outcome_vector&, ostream&);

extern
ostream&
operator << (ostream&, const outcome_odds&);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BOC2_OUTCOME_HH__

