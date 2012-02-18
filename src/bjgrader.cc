/**
	Interactive blackjack grader program.
	Implements a game/simulator and in-game analyzer.
	TODO: name this drbj (Dr. BJ)
	Also rates how good you are vs. how lucky you are.  
	$Id: bjgrader.cc,v 1.7 2012/02/18 21:13:36 fang Exp $
 */

#include <iostream>
#include <map>
#include <numeric>		// for partial_sum
#include "blackjack.hh"

using std::cin;
using std::istream;
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
"r,rules -- print the rule variation for BlackJack\n"
"p,play -- sit down at a table and start playing\n"
"q,quit,exit,bye -- leave the casino" << endl;
}

static
void
lobby_loop(istream& i, ostream& o) {
	// command-line prompt
	// TODO: GNU readline
	blackjack::variation var;
// outer loop: setup game variation, initialize bankroll
	o << "Welcome to The BlackJack Trainer!" << endl;
	o << "You stand in the front lobby a casino." << endl;
	o << "Type 'help' for a list of lobby commands." << endl;
do {
	string line;
	do {
		// you are in the lobby of the casino...
		o << "lobby> ";
		i >> line;
	} while (line.empty() && i);
	// TODO: use a map of commands that manipulate the game state
	if (line == "?" || line == "help") {
		lobby_help(o);
	} else if (line == "r" || line == "rules") {	// variation
		// dump variation
		var.dump(o);
	} else if (line == "c" || line == "configure") {	// variation
		var.configure(i, o);
	} else if (line == "p" || line == "play") {
		// simulate, train, analyze
		// inner loop: play game
		blackjack::grader G(var);
		G.play_hand(i, o);
	} else if (line == "q" || line == "exit" ||
			line == "quit" || line == "bye") {
		break;
	} else {
		o << "Unknown command: \"" << line <<
			"\".\nType 'help' for a list of commands." << endl;
	}
} while (i);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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
	lobby_loop(cin, cout);
	cout << "Thanks for playing!" << endl;
	return 0;
}

