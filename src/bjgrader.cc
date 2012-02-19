/**
	Interactive blackjack grader program.
	Implements a game/simulator and in-game analyzer.
	TODO: name this drbj (Dr. BJ)
	Also rates how good you are vs. how lucky you are.  
	$Id: bjgrader.cc,v 1.8 2012/02/19 20:56:04 fang Exp $
 */

#include <iostream>
#include <map>
#include <numeric>		// for partial_sum
#include "util/readline.h"
#include "blackjack.hh"
#include "util/command.tcc"
#include "util/value_saver.hh"

using std::cin;
using std::istream;
using std::ostream;
using std::cout;
using std::endl;
using std::getline;
using util::Command;
using util::CommandStatus;
using util::command_registry;
using util::string_list;
using util::value_saver;

struct lobby {
	// only in the lobby is the variation/rules allowed to change
	blackjack::variation		var;
};	// end struct lobby

typedef	Command<lobby>		LobbyCommand;

namespace util {
// explicitly instantiate
template class command_registry<LobbyCommand>;
}

typedef	command_registry<LobbyCommand>		lobby_command_registry;

#define	DECLARE_LOBBY_COMMAND_CLASS(class_name, _cmd, _brief)		\
	DECLARE_AND_INITIALIZE_COMMAND_CLASS(lobby, class_name, _cmd, _brief)

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_LOBBY_COMMAND_CLASS(Help, "help", "list all commands")
int
Help::main(lobby&, const string_list&) {
	command_registry<LobbyCommand>::list_commands(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_LOBBY_COMMAND_CLASS(Quit, "quit", "exit program")
int
Quit::main(lobby&, const string_list&) {
	return CommandStatus::END;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_LOBBY_COMMAND_CLASS(Rules, "rules", "show rule variations")
int
Rules::main(lobby& L, const string_list&) {
	L.var.dump(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_LOBBY_COMMAND_CLASS(Configure, "configure", "set rule variations")
int
Configure::main(lobby& L, const string_list&) {
	L.var.configure(cin, cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_LOBBY_COMMAND_CLASS(Play, "play", "start playing blackjack")
int
Play::main(lobby& L, const string_list&) {
	blackjack::grader G(L.var);
	G.play_hand(cin, cout);
	return CommandStatus::NORMAL;
}

//=============================================================================
#if 0
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
#endif

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
#if 0
	lobby_loop(cin, cout);
#else
	lobby L;
	lobby_command_registry::prompt = "lobby> ";
	const value_saver<util::completion_function_ptr>
		tmp(rl_attempted_completion_function,
			&lobby_command_registry::completion);
	lobby_command_registry::interpret(L);
#endif
	cout << "Thanks for playing!" << endl;
	return 0;
}

