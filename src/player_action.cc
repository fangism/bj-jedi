// "player_action.cc"

#include <iostream>
#include <string>
#include "player_action.hh"
#include "util/configure_option.hh"

namespace blackjack {
using std::endl;
using std::string;
using util::yn;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const action_mask action_mask::stand(STAND);
const action_mask action_mask::stand_hit(STAND, HIT);
const action_mask action_mask::all(stand_hit +DOUBLE +SPLIT +SURRENDER);
const action_mask action_mask::no_stand(all -STAND);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\param d if double-down is permitted.
	\param p is splitting is permitted.
	\param r if surrendering is permitted.
	\param a if should just automatically play (OPTIM)
 */
player_choice
action_mask::prompt(istream& i, ostream& o, const bool a) const
	{
	const bool d = can_double_down();
	const bool p = can_split();
	const bool r = can_surrender();
	player_choice c = NIL;
do {
//	int ch;
	string line;
	do {
		// prompt for legal choices
		o << "action? [hs";
		if (d) o << 'd';
		if (p) o << 'p';
		if (r) o << 'r';
		o << "bc?!]> ";
		if (!a) {
//		ch = getchar();
		// TODO: ncurses getch();
		// i >> line;
		getline(i, line);
		}
	} while (line.empty() && i && !a);
	if (a) {
		o << '!' << endl;
		return OPTIM;
	}
	switch (line[0])
//	switch (ch)
	{
	case 'h':
	case 'H':
		c = HIT; break;
	case 'd':
	case 'D':
		if (d) { c = DOUBLE; } break;
	case 'p':
	case 'P':
		if (p) { c = SPLIT; } break;
	case 's':
	case 'S':
		c = STAND; break;
	case 'r':
	case 'R':
		if (r) { c = SURRENDER; } break;
	case 'b':
	case 'B':
		c = BOOKMARK; break;	// show count
	// information commands
	case 'c':
	case 'C':
		c = COUNT; break;	// show count
	case '?':
		c = HINT; break;
		// caller should provide detailed hint with edges
		// for both basic and dynamic strategies
	case '!':
		c = OPTIM; break;
	default: break;
	}
} while (c == NIL && i);
	return c;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
ostream&
action_mask::dump_debug(ostream& o) const {
	return o << "s,h,d,p,r=" <<
		can_stand() << ',' <<
		can_hit() << ',' <<
		can_double_down() << ',' <<
		can_split() << ',' <<
		can_surrender();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
ostream&
action_mask::dump_verbose(ostream& o) const {
	return o << "double:" << yn(can_double_down())
		<< ", split:" << yn(can_split())
		<< ", surrender:" << yn(can_surrender()) << endl;
}

//=============================================================================
}	// end namespace blackjack

