/**
	Interactive blackjack grader program.
	Main menu in the 'lobby'.
	$Id: lobby.cc,v 1.6 2012/03/10 23:26:36 fang Exp $
 */

#include <iostream>
#include <iterator>
#include "lobby.hh"
#include "grader.hh"
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
using std::ostream_iterator;
using std::cout;
using std::endl;

typedef	command_registry<LobbyCommand>		lobby_command_registry;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int
lobby::main(void) {
	const value_saver<string>
		tmp1(lobby_command_registry::prompt, "lobby> ");
	lobby_command_registry::interpret(*this);
	return 0;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace lobby_commands {
#define	DECLARE_LOBBY_COMMAND_CLASS(class_name, _cmd, _brief)		\
	DECLARE_AND_INITIALIZE_COMMAND_CLASS(lobby, class_name, _cmd, _brief)

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_LOBBY_COMMAND_CLASS(Help, "help",
	"[cmd] : list lobby command(s)")
int
Help::main(lobby& g, const string_list& args) {
switch (args.size()) {
case 1:
	lobby_command_registry::list_commands(cout);
	cout <<
"Menu commands run without arguments will enter the menu.\n"
"Anything that follows a menu command will be issued as a command to the\n"
"menu without entering the menu." << endl;
	break;
default: {
	string_list::const_iterator i(++args.begin()), e(args.end());
	for ( ; i!=e; ++i) {
		if (!lobby_command_registry::help_command(cout, *i)) {
			cout << "Command not found: " << *i << endl;
		} else {
			cout << endl;
		}
	}
}
}
	return CommandStatus::NORMAL;
}
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_LOBBY_COMMAND_CLASS(Help2, "?",
	"[cmd] : list lobby command(s)")
int
Help2::main(lobby& v, const string_list& args) {
	return Help::main(v, args);
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
DECLARE_LOBBY_COMMAND_CLASS(Echo, "echo", ": print arguments")
int
Echo::main(lobby&, const string_list& args) {
	copy(++args.begin(), args.end(), ostream_iterator<string>(cout, " "));
	cout << endl;
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if 0
DECLARE_LOBBY_COMMAND_CLASS(Rules, "rules", ": show rule variations")
int
Rules::main(lobby& L, const string_list&) {
	L.var.dump(cout);
	return CommandStatus::NORMAL;
}
#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_LOBBY_COMMAND_CLASS(Variation, "variation",
	"[cmd] : (menu) rule variations")
int
Variation::main(lobby& L, const string_list& args) {
if (args.size() <= 1) {
	// enter menu
	L.var.configure();
	return CommandStatus::NORMAL;
} else {
	// execute a single variation command
	const string_list rem(++args.begin(), args.end());
	return L.var.command(rem);
}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_LOBBY_COMMAND_CLASS(PlayOption, "options",
	"[cmd] : (menu) game-play options")
int
PlayOption::main(lobby& L, const string_list& args) {
if (args.size() <= 1) {
	// enter menu
	L.opt.configure();
	return CommandStatus::NORMAL;
} else {
	// execute a single play_option command
	const string_list rem(++args.begin(), args.end());
	return L.opt.command(rem);
}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_LOBBY_COMMAND_CLASS(Play, "play", ": start playing blackjack")
int
Play::main(lobby& L, const string_list&) {
	grader G(L.var, L.opt, cin, cout);
	G.main();
	return CommandStatus::NORMAL;
}

#undef	DECLARE_LOBBY_COMMAND_CLASS
}	// end namespace lobby_commands
//=============================================================================
}	// end namespace blackjack
