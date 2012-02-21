/**
	Interactive blackjack grader program.
	Main menu.
	$Id: lobby.cc,v 1.1 2012/02/21 00:00:32 fang Exp $
 */

#include <iostream>
#include "lobby.hh"
#include "grader.hh"
#include "util/readline.h"
#include "util/command.tcc"
#include "util/value_saver.hh"

namespace blackjack {
using util::Command;
using util::CommandStatus;
using util::command_registry;
using util::string_list;
using util::value_saver;

typedef	Command<lobby>		LobbyCommand;
}
namespace util {
// explicitly instantiate
template class command_registry<blackjack::LobbyCommand>;
}
namespace blackjack {
using std::string;
using std::cin;
using std::istream;
using std::ostream;
using std::cout;
using std::endl;

typedef	command_registry<LobbyCommand>		lobby_command_registry;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace lobby_commands {
#define	DECLARE_LOBBY_COMMAND_CLASS(class_name, _cmd, _brief)		\
	DECLARE_AND_INITIALIZE_COMMAND_CLASS(lobby, class_name, _cmd, _brief)

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_LOBBY_COMMAND_CLASS(Help, "help", ": list all lobby commands")
int
Help::main(lobby&, const string_list&) {
	lobby_command_registry::list_commands(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_LOBBY_COMMAND_CLASS(Help2, "?", ": list all lobby commands")
int
Help2::main(lobby&, const string_list&) {
	lobby_command_registry::list_commands(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_LOBBY_COMMAND_CLASS(Quit, "quit", ": exit program")
int
Quit::main(lobby&, const string_list&) {
	return CommandStatus::END;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_LOBBY_COMMAND_CLASS(Exit, "exit", ": exit program")
int
Exit::main(lobby&, const string_list&) {
	return CommandStatus::END;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_LOBBY_COMMAND_CLASS(Rules, "rules", ": show rule variations")
int
Rules::main(lobby& L, const string_list&) {
	L.var.dump(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_LOBBY_COMMAND_CLASS(Configure, "configure", ": set rule variations")
int
Configure::main(lobby& L, const string_list&) {
	L.var.configure();
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_LOBBY_COMMAND_CLASS(Play, "play", ": start playing blackjack")
int
Play::main(lobby& L, const string_list&) {
	blackjack::grader G(L.var);
	G.main();
	return CommandStatus::NORMAL;
}

#undef	DECLARE_LOBBY_COMMAND_CLASS
}	// end namespace lobby_commands
//=============================================================================
}	// end namespace blackjack
