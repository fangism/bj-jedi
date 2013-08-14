/**
	\file "bj/core/analysis.hh"
	Probability analysis routines.
	$Id: $
 */

#ifndef	__BJ_CORE_ANALYSIS_HH__
#define	__BJ_CORE_ANALYSIS_HH__

#include <iosfwd>
#include "bj/core/num.hh"

namespace blackjack {
using std::ostream;
using cards::probability_type;

/**
	Enumeration representing analysis mode.
 */
enum analysis_mode {
	/**
		Basic strategy, assumes a standard, static card distribution,
		analysis assumes an infinite-deck or finite deck with 
		replacement.
		Analysis is fast and approximate.
	 */
	ANALYSIS_BASIC,
	/**
		Dynamic strategy, assumes the current card distribution,
		with replacement (so infinite in the same proportion).
		Analysis is moderately fast, more accurate.
	 */
	ANALYSIS_DYNAMIC,
	/**
		Dynamic strategy, assuming current card distribution,
		without replacement (finite, and removing cards).
		Analysis is slow, but *exact*.
	 */
	ANALYSIS_EXACT
};

extern
ostream&
operator << (ostream& o, const analysis_mode);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Bundle of analysis parameters.
	TODO: use independent settings for dealer calculation
	and player calculation.
 */
struct analysis_parameters {
	/**
		Analysis mode, depending on accuracy/speed.
	 */
	analysis_mode				mode;
	/**
		The cumulative probability to reach this state.
		This probability is used to dynamically switch modes.
		The least probable sub-trees can get away with
		lesser accuracy.
	 */
	probability_type			parent_prob;
	/**
		The threshold at which to switch from exact to
		dynamic analysis strategy.
	 */
	probability_type			dynamic_threshold;
//	probability_type			basic_threshold;
#if 0
	/**
		Control the verbosity of the analysis.
	 */
	bool					verbose;
#endif

	analysis_parameters() : mode(ANALYSIS_BASIC),
		parent_prob(1.0),
		dynamic_threshold(1e-4) { }

	/**
		Downgrade one level of analysis accuracy when
		probability falls below threshold.
	 */
	analysis_mode
	get_auto_mode(void) const {
		if (parent_prob < dynamic_threshold) {
			if (mode == ANALYSIS_EXACT)
				return ANALYSIS_DYNAMIC;
			else if (mode == ANALYSIS_DYNAMIC)
				return ANALYSIS_BASIC;
		}
		return mode;
	}

	ostream&
	dump(ostream&) const;

};	// end struct analysis_parameters

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BJ_CORE_ANALYSIS_HH__
