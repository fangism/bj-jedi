// "expectations.hh"

#ifndef	__BOC2_EXPECTATIONS_HH__
#define	__BOC2_EXPECTATIONS_HH__

#include <iosfwd>
#include <utility>			// for std::pair
#include "deck.hh"
#include "enums.hh"

namespace blackjack {
using std::pair;
using std::istream;
using std::ostream;
using cards::edge_type;

typedef	player_choice			action_preference[4];

/**
	Each field's value is the expected outcome of the decision, 
	with -1 being a loss, +1 being a win, etc...
	'hit' is initialized -1 for the sake of computing
	optimal decisions.  
 */
struct expectations {
	edge_type			stand;
	edge_type			hit;
	edge_type			double_down;
	edge_type			split;
#if 0
	static const edge_type		surrender;
#endif
	action_preference		actions;

	expectations() : stand(0.0), hit(-1.0),
		double_down(0.0), 
		split(-2.0) {
	actions[0] = actions[1] = actions[2] = actions[3] = NIL;
	}

	expectations(const edge_type s, const edge_type h, 
		const edge_type d, const edge_type p) :
		stand(s), hit(h), double_down(d), split(p) {
	}

	const edge_type&
	value(const player_choice c, const edge_type& s) const;

	pair<player_choice, player_choice>
	best_two(const bool d = true,
		const bool s = true,
		const bool r = true) const;

	player_choice
	best(const bool d = true,
		const bool s = true,
		const bool r = true) const {
		return best_two(d, s, r).first;
	}

	expectations
	operator + (const expectations& e) const {
		return expectations(stand +e.stand, 
			hit +e.hit, double_down +e.double_down, 
			split +e.split);
	}

	void
	optimize(const edge_type& r);

	// compare two choices
	edge_type
	margin(const player_choice& f, const player_choice& s, 
			const edge_type& r) const {
		return value(f, r) -value(s, r);
	}

	ostream&
	dump_choice_actions(ostream&, const edge_type&) const;

	static
	ostream&
	dump_choice_actions_2(ostream&, const expectations&,
		const expectations&, const edge_type&, 
		const bool, const bool, const bool);

};	// end struct expectations

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BOC2_EXPECTATIONS_HH__

