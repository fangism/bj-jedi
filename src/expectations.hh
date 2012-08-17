// "expectations.hh"

#ifndef	__BOC2_EXPECTATIONS_HH__
#define	__BOC2_EXPECTATIONS_HH__

#include <iosfwd>
#include <utility>			// for std::pair
#include "num.hh"
#include "enums.hh"
#include "devel_switches.hh"

namespace blackjack {
using std::pair;
using std::istream;
using std::ostream;
using cards::edge_type;
#if BITMASK_ACTION_OPTIONS
struct action_mask;
#endif

/**
	Define to 1 to store action expectation edges as an array.
	Rationale: direct indexing faster than switch-case construct.
	Goal: 1
	Status: tested
 */
#define	INDEX_ACTION_EDGES			1

// TODO: this could be encoded compactly with 4 chars or so
typedef	player_choice			action_preference[__NUM_EVAL_ACTIONS];

/**
	Each field's value is the expected outcome of the decision, 
	with -1 being a loss, +1 being a win, etc...
	'hit' is initialized -1 for the sake of computing
	optimal decisions.  
	TODO: convert these members over to an array indexed
		by action enum from enums.hh.
		Positions and ordering should be kept consistent.
	Omit surrender, assuming that its value is constant.
 */
struct expectations {
#if INDEX_ACTION_EDGES
	edge_type			__action_edge[__NUM_EVAL_ACTIONS];
#else
	edge_type			_stand;
	edge_type			_hit;
	edge_type			_double_down;
	edge_type			_split;
#endif
#if 0
	static const edge_type		surrender;
#endif
	/**
		Ranking from best to worst decisions.
	 */
	action_preference		actions;

	expectations()
#if !INDEX_ACTION_EDGES
		: stand(0.0), hit(-1.0),
		double_down(0.0), 
		split(-2.0)
#endif
		{
#if INDEX_ACTION_EDGES
		stand() = 0.0;
		hit() = -1.0;
		double_down() = -2.0;
		split() = -2.0;
#endif
		actions[0] = actions[1] = actions[2] = actions[3] = NIL;
	}

	expectations(const edge_type s, const edge_type h, 
		const edge_type d, const edge_type p)
#if !INDEX_ACTION_EDGES
		: stand(s), hit(h), double_down(d), split(p)
#endif
		{
#if INDEX_ACTION_EDGES
		stand() = s;
		hit() = h;
		double_down() = d;
		split() =  p;
#endif
	}

#if INDEX_ACTION_EDGES
	// accessors
	edge_type&
	action_edge(const player_choice p) {
		return __action_edge[p -__FIRST_EVAL_ACTION];
	}

	const edge_type&
	action_edge(const player_choice p) const {
		return __action_edge[p -__FIRST_EVAL_ACTION];
	}

	edge_type&
	stand(void) { return action_edge(STAND); }

	const edge_type&
	stand(void) const { return action_edge(STAND); }

	edge_type&
	hit(void) { return action_edge(HIT); }

	const edge_type&
	hit(void) const { return action_edge(HIT); }

	edge_type&
	double_down(void) { return action_edge(DOUBLE); }

	const edge_type&
	double_down(void) const { return action_edge(DOUBLE); }

	edge_type&
	split(void) { return action_edge(SPLIT); }

	const edge_type&
	split(void) const { return action_edge(SPLIT); }

	// surrender
#else
	edge_type&
	stand(void) { return _stand; }

	const edge_type&
	stand(void) const { return _stand; }

	edge_type&
	hit(void) { return _hit; }

	const edge_type&
	hit(void) const { return _hit; }

	edge_type&
	double_down(void) { return _double_down; }

	const edge_type&
	double_down(void) const { return _double_down; }

	edge_type&
	split(void) { return _split; }

	const edge_type&
	split(void) const { return _split; }
#endif

	const edge_type&
	value(const player_choice c, const edge_type& s) const;

	pair<player_choice, player_choice>
	best_two(
#if BITMASK_ACTION_OPTIONS
		const action_mask& m
#else
		const bool d = true,
		const bool s = true,
		const bool r = true
#endif
		) const;

	player_choice
	best(
#if BITMASK_ACTION_OPTIONS
		const action_mask& m
#else
		const bool d = true,
		const bool s = true,
		const bool r = true
#endif
		) const {
#if BITMASK_ACTION_OPTIONS
		return best_two(m).first;
#else
		return best_two(d, s, r).first;
#endif
	}

#if BITMASK_ACTION_OPTIONS
	player_choice
	best(void) const;
#endif

	expectations
	operator + (const expectations& e) const {
		// TODO: use transform or valarray operator?
		// TODO: define arithmetic operators in util::array?
#if INDEX_ACTION_EDGES
		return expectations(stand() +e.stand(), 
			hit() +e.hit(), double_down() +e.double_down(), 
			split() +e.split());
#else
		return expectations(stand +e.stand, 
			hit +e.hit, double_down +e.double_down, 
			split +e.split);
#endif
	}

	expectations&
	operator += (const expectations& e) {
#if INDEX_ACTION_EDGES
		stand() += e.stand();
		hit() += e.hit();
		double_down() += e.double_down();
		split() += e.split();
#else
		stand += e.stand;
		hit += e.hit;
		double_down += e.double_down;
		split += e.split;
#endif
		return *this;
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
#if BITMASK_ACTION_OPTIONS
		const action_mask&,
#else
		const bool, const bool, const bool,
#endif
		const char* = NULL);

};	// end struct expectations

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BOC2_EXPECTATIONS_HH__

