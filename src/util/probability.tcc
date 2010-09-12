#ifndef	__UTIL_PROBABILITY_TCC__
#define	__UTIL_PROBABILITY_TCC__

#include <algorithm>
#include <numeric>
#include <functional>
#include "util/probability.hh"

namespace util {
/**
	Make the sums add up to 1.0.
 */
template <class A>
void
normalize(A& a) {
	typedef	typename A::value_type	value_type;
	const value_type sum = std::accumulate(a.begin(), a.end(), 0.0);
	std::transform(a.begin(), a.end(), a.begin(), 
		std::bind2nd(std::divides<value_type>(), sum));
}

}	// end namespace util
#endif	// __UTIL_PROBABILITY_TCC__

