// "variation.cc"

#include <iostream>
#include "variation.hh"

#include "util/string.tcc"	// for string_to_num
#include "util/command.tcc"


namespace blackjack {
using std::endl;

//-----------------------------------------------------------------------------
// class variation method definitions

static
const char*
yn(const bool y) {
	return y ? "yes" : "no";
}

ostream&
variation::dump(ostream& o) const {
	o << "number of decks: " << num_decks << endl;
	o << "maximum penetration: " << maximum_penetration << endl;
	o << "dealer soft 17: " << (H17 ? "hits" : "stands") << endl;
	o << "player early surrender: " << yn(surrender_early) << endl;
	o << "player late surrender: " << yn(surrender_late) << endl;
	o << "dealer peek on 10: " << yn(peek_on_10) << endl;
	o << "dealer peek on A: " << yn(peek_on_Ace) << endl;
	o << "player loses ties: " << yn(ties_lose) << endl;
	o << "player wins ties: " << yn(ties_win) << endl;
	o << "player may double-down on 9: " << yn(double_H9) << endl;
	o << "player may double-down on 10: " << yn(double_H10) << endl;
	o << "player may double-down on 11: " << yn(double_H11) << endl;
	o << "player may double-down on other: " << yn(double_other) << endl;
	o << "player double after split: " << yn(double_after_split) << endl;
	o << "player may split: " << yn(split) << endl;
	o << "player may re-split: " << yn(resplit) << endl;
	o << "player receives only one card on each split ace: "
		<< yn(one_card_on_split_aces) << endl;
	o << "dealer 22 pushes against player: " << yn(push22) << endl;
	o << "blackjack pays: " << bj_payoff << endl;
	o << "insurance pays: " << insurance << endl;
	o << "surrender penalty: " << surrender_penalty << endl;
	o << "double-down multiplier: " << double_multiplier << endl;
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	set individual options: set key value
	configure-all: one-by-one
 */
void
variation::configure(istream&, ostream& o) {
	o << "TODO: finish variation::configure" << endl;
}

//=============================================================================
}	// end namespace blackjack

