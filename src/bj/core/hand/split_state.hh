// "bj/core/hand.hh"

#ifndef	__BJ_CORE_HAND_SPLIT_STATE_HH__
#define	__BJ_CORE_HAND_SPLIT_STATE_HH__

#include <iosfwd>

#include "config.h"
#if defined(HAVE_UNISTD_H)
#include <unistd.h>			// for <sys/types.h>: ssize_t
#elif defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>			// for ssize_t
#endif

namespace blackjack {
using std::ostream;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	To compute the exact expected value of splitting and resplitting,
	the exact state of the hand must also capture:
	1) the number of splits remaining (depending on maximum)
	2) the number unplayed splittable hands
	Each time a hand is split, splits-remaining decrements.
	The number of splittable-hands is determined by
	the new upcards on the split hands (probability vector).  

	Consider a paired hand X,X.
	After splitting, there are new hands X,Y and X,Z.
	The order in which these are played does not affect the overall 
	expected outcome.
	Depending on Y and Z, the new state may have 0,1,2 newly 
	resplittable hands, but number of splits remaining is one less.
	See math text.
 */
struct split_state {
	ssize_t			splits_remaining;
	ssize_t			paired_hands;
	ssize_t			unpaired_hands;

	/// initialize to arbitrary values, should call initialize()
	split_state() : splits_remaining(1), paired_hands(0), 
		unpaired_hands(1) { }

	void
	initialize_default(void) {
		splits_remaining = 1;
		paired_hands = 0;
		unpaired_hands = 1;
	}

	void
	initialize(const ssize_t, const bool);

	bool
	is_splittable(void) const {
		return splits_remaining && paired_hands;
	}

	ssize_t
	total_hands(void) const { return paired_hands +unpaired_hands; }

	ostream&
	dump_code(ostream&) const;

	void
	post_split_states(split_state&, split_state&, split_state&) const;

	ssize_t
	nonsplit_pairs(void) const {
		if (paired_hands > splits_remaining) {
			return paired_hands -splits_remaining;
		}
		return paired_hands;
	}

	int
	compare(const split_state& r) const {
	{
		const ssize_t d = r.splits_remaining -splits_remaining;
		if (d) return d;
	}{
		const ssize_t d = r.paired_hands -paired_hands;
		if (d) return d;
	}
		return r.unpaired_hands -unpaired_hands;
	}

	bool
	operator < (const split_state& r) const {
		return compare(r) < 0;
	}

	ostream&
	generate_graph(ostream&) const;

};	// end struct split_state

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BJ_CORE_HAND_SPLIT_STATE_HH__

