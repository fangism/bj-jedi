// "util/member_select_iterator.hh"

#ifndef	__UTIL_MEMBER_SELECT_ITERATOR_HH__
#define	__UTIL_MEMBER_SELECT_ITERATOR_HH__

#include <iterator>

namespace util {
using std::iterator;
using std::iterator_traits;

#define	MEMBER_SELECT_ITERATOR_TEMPLATE_SIGNATURE			\
template <typename _Iterator, typename T, T iterator_traits<_Iterator>::value_type::*mem>

/**
	Selects a fixed member.
 */
MEMBER_SELECT_ITERATOR_TEMPLATE_SIGNATURE
class member_select_iterator :
	public iterator<typename iterator_traits<_Iterator>::iterator_category,
	      T,	// remove_const?
	      typename iterator_traits<_Iterator>::difference_type,
	      T*,
	      T&> {
	typedef iterator_traits<_Iterator>		base_traits;
	typedef iterator<typename base_traits::iterator_category,
	      T,	// remove_const?
	      typename base_traits::difference_type,
	      T*,
	      T&>					traits;
protected:
	_Iterator					current;
public:
	typedef	_Iterator				iterator_type;
	typedef	typename traits::difference_type	difference_type;
	typedef	typename traits::reference		reference;
	typedef	typename traits::pointer		pointer;

public:
	member_select_iterator() : current() { }

	explicit
	member_select_iterator(const iterator_type i) : current(i) { }

	// default copy-ctor

	iterator_type
	base(void) const { return current; }

	reference
	operator * () const {
		return (*current).*mem;
//		return current->*mem;
	}

	pointer
	operator -> () const {
		return &((*current).*mem);
//		return &(current->*mem);
	}

	member_select_iterator&
	operator ++ () {
		++current;
		return *this;
	}

	member_select_iterator
	operator ++ (int) {
		const member_select_iterator ret(*this);
		++current;
		return ret;
	}

	member_select_iterator&
	operator -- () {
		--current;
		return *this;
	}

	member_select_iterator
	operator -- (int) {
		const member_select_iterator ret(*this);
		--current;
		return ret;
	}

	member_select_iterator
	operator + (difference_type __n) const {
		return member_select_iterator(current - __n);
	}

	member_select_iterator&
	operator += (difference_type __n) {
		current -= __n;
		return *this;
	}

	member_select_iterator
	operator - (difference_type __n) const {
		return member_select_iterator(current + __n);
	}

	member_select_iterator&
	operator -= (difference_type __n) {
		current += __n;
		return *this;
	}

	reference
	operator [] (difference_type __n) const {
		return *(*this + __n);
	}

};	// end class member_select_iterator

// comparison functions

MEMBER_SELECT_ITERATOR_TEMPLATE_SIGNATURE
inline bool
operator == (const member_select_iterator<_Iterator,T,mem>& __x,
       const member_select_iterator<_Iterator,T,mem>& __y)
{ return __x.base() == __y.base(); }

MEMBER_SELECT_ITERATOR_TEMPLATE_SIGNATURE
inline bool
operator < (const member_select_iterator<_Iterator,T,mem>& __x,
      const member_select_iterator<_Iterator,T,mem>& __y)
{ return __x.base() < __y.base(); }

MEMBER_SELECT_ITERATOR_TEMPLATE_SIGNATURE
inline bool
operator != (const member_select_iterator<_Iterator,T,mem>& __x,
       const member_select_iterator<_Iterator,T,mem>& __y)
{ return !(__x == __y); }

MEMBER_SELECT_ITERATOR_TEMPLATE_SIGNATURE
inline bool
operator > (const member_select_iterator<_Iterator,T,mem>& __x,
      const member_select_iterator<_Iterator,T,mem>& __y)
{ return __y < __x; }

MEMBER_SELECT_ITERATOR_TEMPLATE_SIGNATURE
inline bool
operator <= (const member_select_iterator<_Iterator,T,mem>& __x,
	const member_select_iterator<_Iterator,T,mem>& __y)
{ return !(__y < __x); }

MEMBER_SELECT_ITERATOR_TEMPLATE_SIGNATURE
inline bool
operator >= (const member_select_iterator<_Iterator,T,mem>& __x,
       const member_select_iterator<_Iterator,T,mem>& __y)
{ return !(__x < __y); }

MEMBER_SELECT_ITERATOR_TEMPLATE_SIGNATURE
inline typename member_select_iterator<_Iterator,T,mem>::difference_type
operator - (const member_select_iterator<_Iterator,T,mem>& __x,
      const member_select_iterator<_Iterator,T,mem>& __y)
{ return __x.base() - __y.base(); }

MEMBER_SELECT_ITERATOR_TEMPLATE_SIGNATURE
inline member_select_iterator<_Iterator,T,mem>
operator + (typename member_select_iterator<_Iterator,T,mem>::difference_type __n,
      const member_select_iterator<_Iterator,T,mem>& __x)
{ return member_select_iterator<_Iterator,T,mem>(__x.base() + __n); }

//=============================================================================
#define	VAR_MEMBER_SELECT_ITERATOR_TEMPLATE_SIGNATURE			\
template <typename _Iterator, typename T>

/**
	Variable member select iterator.
 */
VAR_MEMBER_SELECT_ITERATOR_TEMPLATE_SIGNATURE
class var_member_select_iterator :
	public iterator<typename iterator_traits<_Iterator>::iterator_category,
	      T,	// remove const?
	      typename iterator_traits<_Iterator>::difference_type,
	      T*,
	      T&> {
	typedef	iterator_traits<_Iterator>		traits;
	typedef	typename traits::value_type		base_value_type;
protected:
	_Iterator					current;
	T base_value_type::*mem;
public:
	typedef	_Iterator				iterator_type;
	typedef	typename traits::difference_type	difference_type;
	typedef	typename traits::reference		reference;
	typedef	typename traits::pointer		pointer;

public:
	explicit
	var_member_select_iterator(const iterator_type i,
			T base_value_type::*m) :
			current(i), mem(m) { }

	// default copy-ctor

	iterator_type
	base(void) const { return current; }

	T base_value_type::*
	member(void) const { return mem; }

	reference
	operator * () const {
		return *current.*mem;
//		return current->*mem;
	}

	pointer
	operator -> () const {
		return &(*current.*mem);
//		return &(current->*mem);
	}

	var_member_select_iterator&
	operator ++ () {
		++current;
		return *this;
	}

	var_member_select_iterator
	operator ++ (int) {
		const var_member_select_iterator ret(*this);
		++current;
		return ret;
	}

	var_member_select_iterator&
	operator -- () {
		--current;
		return *this;
	}

	var_member_select_iterator
	operator -- (int) {
		const var_member_select_iterator ret(*this);
		--current;
		return ret;
	}

	var_member_select_iterator
	operator + (difference_type __n) const {
		return var_member_select_iterator(current - __n);
	}

	var_member_select_iterator&
	operator += (difference_type __n) {
		current -= __n;
		return *this;
	}

	var_member_select_iterator
	operator - (difference_type __n) const {
		return var_member_select_iterator(current + __n);
	}

	var_member_select_iterator&
	operator -= (difference_type __n) {
		current += __n;
		return *this;
	}

	reference
	operator [] (difference_type __n) const {
		return *(*this + __n);
	}

};	// end class var_member_select_iterator

// comparison functions -- should these also compare pointer-to-member?

VAR_MEMBER_SELECT_ITERATOR_TEMPLATE_SIGNATURE
inline bool
operator == (const var_member_select_iterator<_Iterator,T>& __x,
       const var_member_select_iterator<_Iterator,T>& __y)
{ return __x.base() == __y.base(); }

VAR_MEMBER_SELECT_ITERATOR_TEMPLATE_SIGNATURE
inline bool
operator < (const var_member_select_iterator<_Iterator,T>& __x,
      const var_member_select_iterator<_Iterator,T>& __y)
{ return __x.base() < __y.base(); }

VAR_MEMBER_SELECT_ITERATOR_TEMPLATE_SIGNATURE
inline bool
operator != (const var_member_select_iterator<_Iterator,T>& __x,
       const var_member_select_iterator<_Iterator,T>& __y)
{ return !(__x == __y); }

VAR_MEMBER_SELECT_ITERATOR_TEMPLATE_SIGNATURE
inline bool
operator > (const var_member_select_iterator<_Iterator,T>& __x,
      const var_member_select_iterator<_Iterator,T>& __y)
{ return __x < __y; }

VAR_MEMBER_SELECT_ITERATOR_TEMPLATE_SIGNATURE
inline bool
operator <= (const var_member_select_iterator<_Iterator,T>& __x,
	const var_member_select_iterator<_Iterator,T>& __y)
{ return !(__y < __x); }

VAR_MEMBER_SELECT_ITERATOR_TEMPLATE_SIGNATURE
inline bool
operator >= (const var_member_select_iterator<_Iterator,T>& __x,
       const var_member_select_iterator<_Iterator,T>& __y)
{ return !(__x < __y); }

VAR_MEMBER_SELECT_ITERATOR_TEMPLATE_SIGNATURE
inline typename var_member_select_iterator<_Iterator,T>::difference_type
operator - (const var_member_select_iterator<_Iterator,T>& __x,
      const var_member_select_iterator<_Iterator,T>& __y)
{ return __x.base() - __y.base(); }

VAR_MEMBER_SELECT_ITERATOR_TEMPLATE_SIGNATURE
inline var_member_select_iterator<_Iterator,T>
operator + (typename var_member_select_iterator<_Iterator,T>::difference_type __n,
      const var_member_select_iterator<_Iterator,T>& __x)
{ return var_member_select_iterator<_Iterator,T>(__x.base() + __n); }

}	// end namespace util

#endif	// __UTIL_MEMBER_SELECT_ITERATOR_HH__
