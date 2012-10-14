/**
	\file "util/algorithm.hh"
	$Id: $
	more algorithms beyond <algorithm>
 */

#ifndef	__UTIL_ALGORITHM_HH__
#define	__UTIL_ALGORITHM_HH__

// #include <algorithm>

namespace util {

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\param Iter1, Iter2 input interator types
	\return 0 if equal, +N if sequence 1 is > sequence 2,
		-N if sequence 1 < sequence 2.
		if N != 0, N is the iterator position of the differentiator.
 */
template <class Iter1, class Iter2>
// static
inline
int
dictionary_compare(Iter1 __first1, const Iter1 __last1,
	Iter2 __first2, const Iter2 __last2) {
	int pos = 1;
	for (; __first1 != __last1 && __first2 != __last2;
			++__first1, ++__first2, ++pos) {
		if (*__first1 < *__first2)
			return -pos;
		if (*__first2 < *__first1)
			return pos;
	}
	if (__first1 != __last1)
		return pos;
	if (__first2 != __last2)
		return -pos;
	return 0;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	TODO: use a tri-state comparator, -N for less-than.
	\param Iter1, Iter2 input interator types
	\param Comp a less-than comparator, like std::less
	\return 0 if equal, +N if sequence 1 is greater-than sequence 2,
		-N if sequence 1 less-than sequence 2.
		if N != 0, N is the 1-indexed iterator position of the differentiator.
 */
template <class Iter1, class Iter2, class Comp>
// static
inline
int
dictionary_compare(Iter1 __first1, const Iter1 __last1,
	Iter2 __first2, const Iter2 __last2, Comp __comp) {
	int pos = 1;
	for (; __first1 != __last1 && __first2 != __last2;
			++__first1, ++__first2, ++pos) {
	// this could be replaced w/ a single tri-state comparison
		if (__comp(*__first1, *__first2))
			return -pos;
		if (__comp(*__first2, *__first1))
			return pos;
	}
	if (__first1 != __last1)
		return pos;
	if (__first2 != __last2)
		return -pos;
	return 0;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace util

#endif	// __UTIL_ALGORITHM_HH__
