// "expectations.cc"

#include <iostream>
#include <map>
#include <cassert>
#include "expectations.hh"
#include "player_action.hh"
#include "util/iosfmt_saver.hh"

#define	DEBUG_OPT				0

#if DEBUG_OPT
#define	DEBUG_OPT_PRINT(o, x)			o << x
#else
#define	DEBUG_OPT_PRINT(o, x)
#endif

namespace blackjack {
using std::cout;
using std::cerr;
using std::endl;
using std::multimap;
using std::make_pair;

//=============================================================================
// struct expectation method definitions

/**
	TODO: use an array of pointer-to-members
 */
const edge_type&
expectations::value(const player_choice c, 
		const edge_type& surrender) const {
#if INDEX_ACTION_EDGES
	return c == SURRENDER ? surrender : action_edge(c);
#else
	switch (c) {
	case STAND:	return stand;
	case HIT:	return hit;
	case DOUBLE:	return double_down;
	case SPLIT:	return split;
	case SURRENDER:	return surrender;
	default:	cerr << "Invalid player choice." << endl; assert(0);
	}
#endif
	return stand();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if BITMASK_ACTION_OPTIONS
player_choice
expectations::best(void) const {
	return best(action_mask::all);
}
#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Assumes all choices (double, split, surrender) available.
	It is up to the best() method to filter out invalid choices.  
	\param surrender penalty should be < 0
 */
void
expectations::optimize(const edge_type& surrender) {
	typedef	std::multimap<edge_type, player_choice>	sort_type;
	sort_type s;
	s.insert(make_pair(stand(), STAND));
	s.insert(make_pair(hit(), HIT));
	s.insert(make_pair(double_down(), DOUBLE));
	s.insert(make_pair(split(), SPLIT));
	s.insert(make_pair(surrender, SURRENDER));
	sort_type::const_iterator i(s.begin()), e(s.end());
	// sort by order of preference
	++i;	actions[3] = i->second;
	++i;	actions[2] = i->second;
	++i;	actions[1] = i->second;
	++i;	actions[0] = i->second;	// best choice
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\return the best two player choices (first better than second), 
	given constraints:
	\param d whether double-down is a valid option
	\param s whether split is a valid option
	\param r whether surrender is a valid option
	Invalid options are skipped in the response.  
	TODO: pass choice mask bitfield
 */
pair<player_choice, player_choice>
expectations::best_two(
#if BITMASK_ACTION_OPTIONS
	const action_mask& m
#else
	const bool d, const bool s, const bool r
#endif
	) const {
#if BITMASK_ACTION_OPTIONS
	const bool h = m.can_hit();
	const bool d = m.can_double_down();
	const bool s = m.can_split();
	const bool r = m.can_surrender();
#else
	const bool h = true;
#endif
	DEBUG_OPT_PRINT(cout, "best_two: h,d,p,r=" <<
		h << ',' << d << ',' << s << ',' << r << endl);
	player_choice ret[2];
	const player_choice* i(&actions[0]), *e(&actions[4]);
	player_choice* p = ret;
	player_choice* const pe = &ret[2];
	for ( ; i!=e && p!=pe; ++i) {
	switch (*i) {
	case STAND:
		// stand is always allowed
		*p = *i;
		++p;
		break;
	case HIT:
		// hit is allowed on non-terminal states
		if (h) {
			*p = *i;
			++p;
		}
		break;
	case DOUBLE:
		if (d) {
			*p = *i;
			++p;
		}
		break;
	case SPLIT:
		if (s) {
			*p = *i;
			++p;
		}
		break;
	case SURRENDER:
		if (r) {
			*p = *i;
			++p;
		}
		break;
	default: break;
	}
	}
	if (p != pe) {
	if (ret[0] == HIT) {
		ret[1] = STAND;
	} else if (p[0] == STAND) {
		ret[1] = HIT;
	}
	// else ???
	}
	return pair<player_choice, player_choice>(ret[0], ret[1]);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
expectations::dump_choice_actions(ostream& o,
		const edge_type& surr) const {
//	const util::precision_saver p(o, 4);
	return o << "stand: " << stand()
	<< "\nhit  : " << hit()
	<< "\ndbl  : " << double_down()
	<< "\nsplit: " << split()
	<< "\nsurr.: " << surr << endl;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Prints 2 expectation groups, side-by-side.
	Only show edges for the legal moves in this context, 
	as given by d, p, r.
 */
ostream&
expectations::dump_choice_actions_2(ostream& o,
		const expectations& e1, const expectations& e2,
		const edge_type& surr, 
#if BITMASK_ACTION_OPTIONS
		const action_mask& m,
#else
		const bool d, const bool p, const bool r, 
#endif
		const char* _indent) {
//	const util::precision_saver p(o, 4);
#if BITMASK_ACTION_OPTIONS
	const bool d = m.can_double_down();
	const bool p = m.can_split();
	const bool r = m.can_surrender();
#endif
	const char* indent = _indent ? _indent : "";
	o << indent << "stand: " << e1.stand() << '\t' << e2.stand()
		<< '\n' << indent << "hit  : " << e1.hit() << '\t' << e2.hit();
	if (d) {
		o << '\n' << indent << "dbl  : " << e1.double_down() << '\t' << e2.double_down();
	}
	if (p) {
		o << '\n' << indent << "split: " << e1.split() << '\t' << e2.split();
	}
	if (r) {
		o << '\n' << indent << "surr.: " << surr << '\t' << surr;
	}
	return o << endl;
}

//=============================================================================
}	// end namespace blackjack

