/**
	Interactive blackjack grader program.
	Implements a game/simulator and in-game analyzer.
	Also rates how good you are vs. how lucky you are.  
	$Id: bjgrader.cc,v 1.4 2012/01/23 08:40:26 fang Exp $
 */

#include <iostream>
#include <map>
#include <numeric>		// for partial_sum
#include "blackjack.hh"

using std::cin;
using std::ostream;
using std::cout;
using std::endl;
using std::getline;

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
lobby_help(ostream& o) {
o <<
"In the casino lobby, the following commands are available:\n"
"?,help -- print this help\n"
"r,rules -- print the rule variation for blackjack\n"
"p,play -- sit down at a table and start playing\n"
"q,quit,exit,bye -- leave the casino" << endl;
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
	// command-line prompt
	// TODO: GNU readline
	blackjack::variation var;
// outer loop: setup game variation, initialize bankroll
	cout << "Welcome to The BlackJack Trainer!" << endl;
	cout << "Type 'help' for list of commands." << endl;
do {
	string line;
	do {
		// you are in the lobby of the casino...
		cout << "lobby> ";
		cin >> line;
	} while (line.empty());
	// TODO: use a map of commands that manipulate the game state
	if (line == "?" || line == "help") {
		lobby_help(cout);
	} else if (line == "r" || line == "rules") {	// variation
		// dump variation
		var.dump(cout);
		// TODO: option to change rules
	} else if (line == "p" || line == "play") {
		// simulate, train, analyze
		// inner loop: play game
		blackjack::grader G(var);
		G.play(cin, cout);
	} else if (line == "q" || line == "exit" ||
			line == "quit" || line == "bye") {
		cout << "Thanks for playing!" << endl;
		break;
	}
} while (cin);
	return 0;
}

