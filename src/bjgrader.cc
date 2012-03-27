/**
	Interactive blackjack grader program.
	Implements a game/simulator and in-game analyzer.
	TODO: name this drbj (Dr. BJ)
		or blackjack-jedi
	Also rates how good you are vs. how lucky you are.  
	$Id: bjgrader.cc,v 1.12 2012/02/21 00:00:32 fang Exp $
 */

#include <iostream>
#include "lobby.hh"

using std::cout;
using std::endl;

//=============================================================================
int
main(int, char*[]) {
	cout <<
"Welcome to The BlackJack Trainer!\n"
"You walk into the lobby of a casino.\n"
"Type 'help' or '?' for a list of lobby commands." << endl;
	blackjack::lobby L;
	L.main();
	cout << "Thanks for playing!" << endl;
	return 0;
}

