/**
	\file "util/string.hh"
	Configure-detected string library header.  
	For now, this is really reserved for C++.
	$Id: string.hh,v 1.1 2010/11/22 02:42:36 fang Exp $
 */

#ifndef	__UTIL_STRING_HH__
#define	__UTIL_STRING_HH__

#include <string>
#include <cstring>

namespace util {
namespace strings {

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\param C is the character pointer/iterator type, 
	may be const, may be wchar_t.  
 */
template <class C>
inline
C
eat_whitespace(C& s) {
	if (s) {
		while (*s && isspace(*s))
			++s;
	}
	return s;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/// Return false if conversion-assignment to int is successful.  
template <class I>
bool
string_to_num(const std::string&, I&);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// regex-like functions
// waiting for std::regex <c++0x>

/**
	String global substitution.
	This is only based on exact-matching, not regular expressions!
	\param t the target string (modified)
	\param s the search pattern
	\param r the replacement string
	\return number of substitutions performed
 */
size_t
strgsub(std::string& t, const std::string& s, const std::string& r);


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t
strip_prefix(std::string& t, const std::string& s);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool
string_begins_with(const std::string&, const std::string&);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <typename Trans>
std::string
transform_string(const std::string&, Trans);

std::string
string_tolower(const std::string&);

std::string
string_toupper(const std::string&);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace strings
}	// end namespace util

#endif	/* __UTIL_STRING_HH__ */

