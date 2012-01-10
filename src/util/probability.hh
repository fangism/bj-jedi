// "util/probability.hh"

#ifndef	__UTIL_PROBABILITY_HH__
#define	__UTIL_PROBABILITY_HH__

namespace util {
//=============================================================================
// probability calculation

/**
	A must be a container of real-valued types.
 */
template <class A>
void
normalize(A&);

template <class R, class S>
void
normalize(R&, const S&);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// random number generation

// integer method
template <class S>
size_t
random_draw_from_integer_pdf(const S&);

template <class S>
size_t
random_draw_from_integer_cdf(const S&);

// real method
template <class R>
size_t
random_draw_from_real_pdf(const R&);

template <class R>
size_t
random_draw_from_real_cdf(const R&);

//=============================================================================
}	// end namespace util

#endif	// __UTIL_PROBABILITY_HH__

