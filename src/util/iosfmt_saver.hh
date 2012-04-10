/**
	\file "util/iosfmt_saver.hh"
 */

#ifndef	__UTIL_IOSFMT_SAVER_HH__
#define	__UTIL_IOSFMT_SAVER_HH__

#include <ios>
#include "util/attributes.h"

namespace util {
using std::ios_base;
using std::streamsize;


//=============================================================================
/**
	Saves set ios_base flag(s) for duration of scope.
 */
class setf_saver {
	ios_base&				ios;
	const ios_base::fmtflags		flags;
public:
	// save and set flags
	explicit
	setf_saver(ios_base& io, const ios_base::fmtflags f) :
		ios(io), flags(io.setf(f)) { }

	// restore former flags on destruction
	~setf_saver() { ios.setf(flags); }
} __ATTRIBUTE_UNUSED__ ;	// end class setf_saver

//-----------------------------------------------------------------------------
/**
	Saves set ios_base precision for duration of scope.
 */
class precision_saver {
	ios_base&				ios;
	const streamsize			prec;
public:
	// save and set flags
	explicit
	precision_saver(ios_base& io, const streamsize f) :
		ios(io), prec(io.precision(f)) { }

	// restore former flags on destruction
	~precision_saver() { ios.precision(prec); }
} __ATTRIBUTE_UNUSED__ ;	// end class precision_saver

//-----------------------------------------------------------------------------
/**
	Saves set ios_base width for duration of scope.
 */
class width_saver {
	ios_base&				ios;
	const streamsize			wd;
public:
	// save and set flags
	explicit
	width_saver(ios_base& io, const streamsize f) :
		ios(io), wd(io.width(f)) { }

	// restore former flags on destruction
	~width_saver() { ios.width(wd); }
} __ATTRIBUTE_UNUSED__ ;	// end class width_saver

//=============================================================================
}	// end namespace util

#endif	// __UTIL_IOSFMT_SAVER_HH__
