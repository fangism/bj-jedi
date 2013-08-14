/**
	\file "bj/core/analysis.cc"
	Implementation of edge analysis classes.  
	Includes variants of edge analysis: basic, dynamic, exact.
	$Id: $
 */

#include <iostream>

#include "bj/core/analysis.hh"

namespace blackjack {

//=============================================================================
ostream&
operator << (ostream& o, const analysis_mode mode) {
	switch (mode) {
	case ANALYSIS_EXACT: o << "exact"; break;
	case ANALYSIS_DYNAMIC: o << "dynam"; break;
	case ANALYSIS_BASIC: o << "basic"; break;
	}
	return o;
}

//=============================================================================
// class analysis_parameters method definitions

ostream&
analysis_parameters::dump(ostream& o) const {
	return o << mode << ", " <<
		parent_prob << '/' << dynamic_threshold;
}

//=============================================================================
}	// end namespace blackjack

