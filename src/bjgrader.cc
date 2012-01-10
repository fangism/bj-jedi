/**
	Interactive blackjack grader program.
	Implements a game/simulator and in-game analyzer.
	Also rates how good you are vs. how lucky you are.  
	$Id: bjgrader.cc,v 1.2 2012/01/10 13:08:48 fang Exp $
 */

#include <iostream>
#include <map>
#include <numeric>		// for partial_sum
#include "blackjack.hh"

using std::ostream;
using std::cout;
using std::endl;

/**
	initialization:
	bankroll -- units of bets

	top-level commands:
	reset -- reset counts, start over
	configure -- prompt for number of decks, rule variations
	deck -- show, edit deck distribution
	count --  print card distribution count/odds
	table -- analyze/print strategy table given current odds/count
	stats -- rate your play
	play -- 
	autoplay [N] -- play optimally for some number of hands
	exit|quit
 */
static
void
help(ostream& o) {
	o << "Help is on its way!" << endl;
}

/**
	In-play commands:
	h|hit
	s|stand
	d|double-down
	p|split
	r|surrender
	!|hint -- analyze and lookup strategy table
		show action based on count and basic strategy
	a|auto -- using counted, or basic strategy?
	?|help
 */

/**
	* prompt for number of decks (0 for infinite)
	* prompt for rule variations
	* between plays, ask if other cards were seen (to adjust count)
 */
int
main(int, char*[]) {
	help(cout);
	return 0;
}
