/**
	Interactive blackjack grader program.
	Rates how good you are and how lucky you are.  
	$Id: bjgrader.cc,v 1.1 2010/11/22 02:42:34 fang Exp $
 */

#include <iostream>
#include <map>
#include "blackjack.hh"

/**
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
void
help(ostream&) {
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
	return 0;
}
