/**
	\file "bookmark.cc"
	$Id: $
 */

#include <iostream>
#include "bookmark.hh"
#include "variation.hh"
#include "counter.hh"
#include "util/configure_option.hh"

namespace blackjack {
using cards::card_name;
using std::endl;
using util::yn;

//=============================================================================
// struct bookmark method definitions

bookmark::bookmark() :
		dealer_reveal(cards::ACE), p_hand(), cards() {
}
bookmark::bookmark(const size_t dr, const player_hand& h, const perceived_deck_state& c) :
		dealer_reveal(dr), p_hand(h), cards(c) {
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bookmark::~bookmark() { }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Similar to grader::dump_situation.
 */
ostream&
bookmark::dump(ostream& o) const {
//	cards.show_count(o, false, false);
	cards.show_count(o);
	// don't distinguish face cards, only show remaining, not used cards
	o << "dealer: " << card_name[dealer_reveal] << ", ";
	p_hand.dump_player(o) << endl;
	p_hand.player_options.dump_verbose(o << "options: ");
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Similar to grader::dump_situation.
 */
ostream&
bookmark::dump(ostream& o, const char* cname, 
		const cards::counter_base& c) const {
//	cards.show_count(o, false, false);
	cards.show_count(o);
	// TODO: fix hard-coding
	c.dump_count(o, cname, cards.actual_remaining(), cards.get_counts())
		<< endl;
	// don't distinguish face cards, only show remaining, not used cards
	o << "dealer: " << card_name[dealer_reveal] << ", ";
	p_hand.dump_player(o) << endl;
	p_hand.player_options.dump_verbose(o << "options: ");
	return o;
}

//=============================================================================
}	// end namespace blackjack

