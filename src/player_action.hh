// "player_action.hh"

#ifndef	__BOC2_PLAYER_ACTION_HH__
#define	__BOC2_PLAYER_ACTION_HH__

#include <iosfwd>
#include "enums.hh"
#include "devel_switches.hh"

namespace blackjack {
using std::istream;
using std::ostream;

/**
	Bitfield representing which actions are allowed.
	A set bit means the associated action is allowed; 
	actions include stand, hit, double, split, surrender.
	Bit positions follow the player_choice enums, 
	starting with STAND representing bit 0.
	a 1-byte char would be sufficient.
	Don't bother correcting for offset of __FIRST_EVAL_ACTION,
	we have bits to spare; but we mask out the NIL action.
 */
class action_mask {
	// unsigned char		_bits;
	size_t				_bits;
public:
	static const action_mask		stand;
	static const action_mask		stand_hit;
	static const action_mask		all;
	static const action_mask		no_stand;

	struct dp_tag { };
	struct dpr_tag { };

	action_mask() : _bits(0) { }

protected:
	explicit
	action_mask(const size_t m) : _bits(m) { }

public:
	explicit
	action_mask(const player_choice p1) :
		_bits(1 << p1) { }

	action_mask(const player_choice p1, const player_choice p2) :
		_bits((1 << p1) | (1 << p2)) { }

	action_mask(const player_choice p1, const player_choice p2, 
		const player_choice p3) :
		_bits((1 << p1) | (1 << p2) | (1 << p3)) { }

	action_mask(const dp_tag, const bool d, const bool p) :
		_bits(stand_hit._bits) {
		if (d) *this += DOUBLE;
		if (p) *this += SPLIT;
	}

	action_mask(const dpr_tag, const bool d, const bool p, const bool r) :
		_bits(stand_hit._bits) {
		if (d) *this += DOUBLE;
		if (p) *this += SPLIT;
		if (r) *this += SURRENDER;
	}

	action_mask
	operator & (const action_mask& m) const {
		return action_mask(_bits & m._bits);
	}

#if 0
	action_mask
	operator | (const action_mask& m) const {
		return action_mask(_bits | m._bits);
	}
#endif

	action_mask&
	operator &= (const action_mask& m) {
		_bits &= m._bits;
		return *this;
	}

#if 0
	action_mask&
	operator |= (const action_mask& m) {
		_bits |= m._bits;
		return *this;
	}
#endif

	action_mask
	operator + (const player_choice p) const {
		return (p != NIL) ? action_mask(_bits | (1 << p)) : *this;
	}

	action_mask
	operator - (const player_choice p) const {
		return action_mask(_bits & ~(1 << p));
	}

	action_mask&
	operator += (const player_choice p) {
		if (p != NIL) {
			_bits |= (1 << p);
		}
		return *this;
	}

	action_mask&
	operator -= (const player_choice p) {
		_bits &= ~(1 << p);
		return *this;
	}

	bool
	operator == (const action_mask& m) const {
		return _bits == m._bits;
	}

	bool
	operator != (const action_mask& m) const {
		return _bits != m._bits;
	}

	bool
	operator < (const action_mask& m) const {
		return _bits < m._bits;
	}

	bool
	action_permitted(const player_choice p) const {
		return _bits & (1 << p);
	}

	bool
	can_hit(void) const {
		return action_permitted(HIT);
	}

	// this should really always be true
	bool
	can_stand(void) const {
		return action_permitted(STAND);
	}

	bool
	can_double_down(void) const {
		return action_permitted(DOUBLE);
	}

	bool
	can_split(void) const {
		return action_permitted(SPLIT);
	}

	bool
	can_surrender(void) const {
		return action_permitted(SURRENDER);
	}

	bool
	has_multiple_choice(void) const {
		return _bits & no_stand._bits;
	}

	player_choice
	prompt(istream&, ostream&, const bool) const;

	ostream&
	dump_debug(ostream&) const;

	ostream&
	dump_verbose(ostream&) const;

};	// end struct action_mask

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}	// end namespace blackjack

#endif	// __BOC2_PLAYER_ACTION_HH__

