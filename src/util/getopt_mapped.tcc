/**
	\file "util/getopt_mapped.tcc"
	$Id: getopt_mapped.tcc,v 1.1 2006/07/30 05:50:14 fang Exp $
 */

#ifndef	__UTIL_GETOPT_MAPPED_TCC__
#define	__UTIL_GETOPT_MAPPED_TCC__

#include <iostream>
#include <cassert>
#include "util/getopt_mapped.hh"
#include "util/getopt_portable.h"

namespace util {
using std::string;
using std::cout;
using std::cerr;
using std::endl;

//=============================================================================
#if 0
template <class O>
void
getopt_map<O>::_unknown_option(option_type&) {
	unknown_option(optopt);
	throw getopt_exception(1);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <class O>
void
getopt_map<O>::_missing_argument(option_type&) {
	cerr << "Expected but missing option-argument." << endl;
	throw getopt_exception(1);
}
#endif

//=============================================================================
/**
	Default constructor.  
 */
template <class O>
getopt_map<O>::getopt_map() :
		_is_posix(true), 
		_cache_valid(false), optstring("+"), 
		option_map(), option2_map() {
	// this->add_option('?', &_unknown_option);
	// this->add_option(':', &_missing_argument);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <class O>
getopt_map<O>::~getopt_map() { }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <class O>
void
getopt_map<O>::add_option(const getopt_char_type c,
		const function_ptr_type f) {
	assert(c != ':');		// reserved
	assert(c != '?');		// reserved
	this->option_map[c] = f;
	this->option2_map.erase(c);
	this->_cache_valid = false;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <class O>
void
getopt_map<O>::add_option(const getopt_char_type c,
		const function2_ptr_type f) {
	assert(c != ':');		// reserved
	assert(c != '?');		// reserved
	this->option2_map[c] = f;
	this->option_map.erase(c);
	this->_cache_valid = false;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <class O>
void
getopt_map<O>::clear_option(const getopt_char_type c) {
	this->option_map.erase(c);
	this->option2_map.erase(c);
	this->_cache_valid = false;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Recaches the optstring if needed.  
	Should be called automatically. 
 */
template <class O>
void
getopt_map<O>::update(void) const {
	if (!this->_cache_valid) {
		string optstr(this->_is_posix ? "+" : "");
		{
		// for options without arguments
		typedef	typename map_type::const_iterator	const_iterator;
		const_iterator i(this->option_map.begin()), 
			e(this->option_map.end());
		// accumulate : _Select1st
		for ( ; i!=e; ++i) {
			optstr += i->first;
		}
		}{
		// for options with arguments
		typedef	typename map2_type::const_iterator	const_iterator;
		const_iterator i(this->option2_map.begin()), 
			e(this->option2_map.end());
		// accumulate : _Select1st
		for ( ; i!=e; ++i) {
			optstr += i->first;
			optstr += ':';
		}
		}
		this->optstring = optstr;
		this->_cache_valid = true;
	}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	The main getopt workhorse.  
	\return exit status, 0 for success
 */
template <class O>
int
getopt_map<O>::operator () (const int argc, char* const argv[], 
		option_type& opt) const {
	this->update();
try {
	getopt_char_type c;
	while ((c = getopt(argc, argv, this->optstring.c_str())) != -1) {
		// search both option maps
		{
		typedef	typename map_type::const_iterator	const_iterator;
		const const_iterator f(this->option_map.find(c));
		if (f != this->option_map.end()) {
			(*f->second)(opt);
			continue;
		}
		}{
		typedef	typename map2_type::const_iterator	const_iterator;
		const const_iterator f(this->option2_map.find(c));
		if (f != this->option2_map.end()) {
			(*f->second)(opt, optarg);
			continue;
		}
		}
		// else error (unlikely)
		switch (c) {
		case ':':
			cerr << "Expected but missing option-argument." << endl;
			// "... after -x"
			return 1;
		case '?':
			unknown_option(cerr, optopt);
			return 1;
		default:
			// impossible
			throw getopt_exception(2);
		}	// end switch
	}	// end while
} catch (const getopt_exception& e) {
	cerr << e.message << endl;
	return e.exit_status;
} catch (...) {
	// else re-throw
	cerr << "Error: unhandled exception in getopt." << endl;
	throw;
}
	return 0;
}

//=============================================================================
}	// end namespace util

#endif	// __UTIL_GETOPT_MAPPED_TCC__

