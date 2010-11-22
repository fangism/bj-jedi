/**
	"util/tokenize.hh"
	General string parsing using separator.
 */
#ifndef	__UTIL_TOKENIZE_HH__
#define	__UTIL_TOKENIZE_HH__

#include <string>
#include <list>

namespace util {
typedef	std::list<std::string>			string_list;


// separate a string by whitespace (quick and dirty)
extern void
tokenize(const std::string&, string_list&);

/**
        Pass in a different set of separation characters.
 */
extern void
tokenize(const std::string&, string_list&, const char*);

/**
	Separate using a single character.
 */
extern
void
tokenize_char(const std::string&, string_list&, const char);



}	// end namespace util

#endif	// __UTIL_TOKENIZE_HH__
