/**
	Interactive blackjack grader program.
	Implements a game/simulator and in-game analyzer.
	TODO: name this drbj (Dr. BJ)
	Also rates how good you are vs. how lucky you are.  
	$Id: bjgrader.cc,v 1.9 2012/02/19 21:37:51 fang Exp $
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

	// maybe keep track of bankroll here?
	// load saved users and accounts?

	int
	main(void);

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
DECLARE_LOBBY_COMMAND_CLASS(Help2, "?", "list all commands")
int
Help2::main(lobby&, const string_list&) {
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
DECLARE_LOBBY_COMMAND_CLASS(Exit, "exit", "exit program")
int
Exit::main(lobby&, const string_list&) {
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
int
lobby::main(void) {
	const value_saver<string>
		tmp1(lobby_command_registry::prompt, "lobby> ");
	const value_saver<util::completion_function_ptr>
		tmp(rl_attempted_completion_function,
			&lobby_command_registry::completion);
	lobby_command_registry::interpret(*this);
	return 0;
}

//=============================================================================
int
main(int, char*[]) {
	cout <<
"Welcome to The BlackJack Trainer!\n"
"You walk into in the lobby a casino.\n"
"Type 'help' or '?' for a list of lobby commands." << endl;
	lobby L;
	L.main();
	cout << "Thanks for playing!" << endl;
	return 0;
}

