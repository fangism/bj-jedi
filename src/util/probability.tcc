#ifndef	__UTIL_PROBABILITY_TCC__
#define	__UTIL_PROBABILITY_TCC__

#include <algorithm>
#include <numeric>
#include <limits>
#include <functional>
#include <cstdlib>		// for ldiv, rand48 family
#include "util/probability.hh"

namespace util {
using std::numeric_limits;
//=============================================================================
/**
	Make the sums add up to 1.0.
	\param a sequence of non-negative real numbers with sum > 0.0.
 */
template <class A>
void
normalize(A& a) {
	typedef	typename A::value_type	value_type;
	const value_type sum = std::accumulate(a.begin(), a.end(), 0.0);
	std::transform(a.begin(), a.end(), a.begin(), 
		std::bind2nd(std::divides<value_type>(), sum));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Takes a sequence of non-negative integer-values and produces
	a result of normalized probabilities.
 */
template <class R, class S>
void
normalize(R& r, const S& s) {
	std::copy(s.begin(), s.end(), r.begin());
	normalize(r);
}

//=============================================================================
// random number generation from distributions

template <class S>
size_t
random_draw_from_integer_pdf(const S& p) {
	S c(p);
	std::partial_sum(p.begin(), p.end(), c.begin());
	return random_draw_from_integer_cdf(c);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <class S>
size_t
random_draw_from_integer_cdf(const S& s) {
	static const int64_t lrand_max = 1LL <<32;
	const int64_t total = long(s.back());
	const int64_t b_rem = lrand_max % total;
	const int64_t upper = lrand_max - b_rem;
	long rn;
	do {
		rn = lrand48();
	} while (rn >= upper);
	// re-roll when number is in the upper residue (low prob.)
	const int64_t roll = rn % total;
	typedef	typename S::const_iterator	const_iterator;
	const const_iterator f(std::upper_bound(s.begin(), s.end(), roll));
	// uses binary_search
	return std::distance(s.begin(), f);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <class R>
size_t
random_draw_from_real_pdf(const R& p) {
	R c(p);
	std::partial_sum(p.begin(), p.end(), c.begin());
	return random_draw_from_real_cdf(c);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <class R>
size_t
random_draw_from_real_cdf(const R& s) {
	const probability_type roll = drand48();
	typedef	typename R::const_iterator	const_iterator;
	const const_iterator f(std::upper_bound(s.begin(), s.end(), roll));
	return std::distance(s.begin(), f);
}

//=============================================================================
}	// end namespace util
#endif	// __UTIL_PROBABILITY_TCC__

