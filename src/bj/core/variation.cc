// "bj/core/variation.cc"

#include <iostream>
#include <fstream>
#include "bj/core/variation.hh"

#include "util/configure_option.hh"
#include "util/command.tcc"
#include "util/value_saver.hh"

namespace blackjack {
typedef	util::Command<variation>	VariationCommand;
}
namespace util {
template class command_registry<blackjack::VariationCommand>;
}

namespace blackjack {
using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::map;
using std::ofstream;
using std::ifstream;
using std::getline;
using util::string_list;
using util::Command;
using util::CommandStatus;
using util::value_saver;
using util::tokenize;
using util::yn;

typedef	util::command_registry<VariationCommand>
						variation_command_registry;

//-----------------------------------------------------------------------------
// class variation method definitions

/**
	Human-readable summary of options.
 */
ostream&
variation::dump(ostream& o) const {
	o << "number of decks: " << num_decks << endl;
	o << "maximum penetration: " << maximum_penetration << endl;
	o << "dealer soft 17: " << (H17 ? "hits" : "stands") << endl;
	o << "player early surrender: " << yn(surrender_early) << endl;
	o << "player late surrender: " << yn(surrender_late) << endl;
	o << "dealer peek on 10: " << yn(peek_on_10) << endl;
	o << "dealer peek on A: " << yn(peek_on_Ace) << endl;
	o << "player loses ties: " << yn(ties_lose) << endl;
	o << "player wins ties: " << yn(ties_win) << endl;
	o << "player may double-down on 9: " << yn(double_H9) << endl;
	o << "player may double-down on 10: " << yn(double_H10) << endl;
	o << "player may double-down on 11: " << yn(double_H11) << endl;
	o << "player may double-down on other: " << yn(double_other) << endl;
	o << "player double after split: " << yn(double_after_split) << endl;
#if IMPROVED_SPLIT_ANALYSIS
	o << "maximum number of split hands: " << max_split_hands << endl;
#else
	o << "player may split: " << yn(split) << endl;
	o << "player may re-split: " << yn(resplit) << endl;
#endif
	o << "player may hit on split aces: " << yn(hit_split_aces) << endl;
	o << "player may re-split aces: " << yn(resplit_aces) << endl;
	o << "dealer 22 pushes against player: " << yn(push22) << endl;
	o << "blackjack pays: " << bj_payoff << endl;
	o << "insurance pays: " << insurance << endl;
	o << "surrender penalty: " << surrender_penalty << endl;
	o << "double-down multiplier: " << double_multiplier << endl;
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	set individual options: set key value
	configure-all: one-by-one
 */
void
variation::configure(void) {
	cout <<
"Configuring blackjack rule variations.\n"
"Type 'help' or '?' for a list of commands." << endl;
	const value_saver<string>
		tmp1(variation_command_registry::prompt, "variation> ");
	variation_command_registry::interpret(*this);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int
variation::command(const string_list& cmd) {
	return variation_command_registry::execute(*this, cmd);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace variation_commands {

#define DECLARE_VARIATION_COMMAND_CLASS(class_name, _cmd, _brief)	\
        DECLARE_AND_INITIALIZE_COMMAND_CLASS(variation, class_name, _cmd, _brief)

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_VARIATION_COMMAND_CLASS(Help, "help",
	"[cmd] : list variation command(s)")
int
Help::main(variation&, const string_list& args) {
switch (args.size()) {
case 1:
	variation_command_registry::list_commands(cout);
	cout <<
"Note: [bool] values are entered as 0 or 1\n"
"[int] and [real] values must be non-negative" << endl;
	break;
default: {
	string_list::const_iterator i(++args.begin()), e(args.end());
	for ( ; i!=e; ++i) {
		if (!variation_command_registry::help_command(cout, *i)) {
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
DECLARE_VARIATION_COMMAND_CLASS(Help2, "?",
	"[cmd] : list variation command(s)")
int
Help2::main(variation& v, const string_list& args) {
	return Help::main(v, args);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_VARIATION_COMMAND_CLASS(ShowAll, "show-all",
	": show all rule variations")
int
ShowAll::main(variation& v, const string_list&) {
	v.dump(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_VARIATION_COMMAND_CLASS(Exit, "exit",
	": finish and return")
int
Exit::main(variation&, const string_list&) {
	return CommandStatus::END;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_VARIATION_COMMAND_CLASS(Done, "done",
	": finish and return")
int
Done::main(variation&, const string_list&) {
	return CommandStatus::END;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_VARIATION_COMMAND_CLASS(Done2, "quit",
	": finish and return")
int
Done2::main(variation&, const string_list&) {
	return CommandStatus::END;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_VARIATION_COMMAND_CLASS(Reset, "reset",
	": reset to default rule variation")
int
Reset::main(variation& v, const string_list&) {
	static const variation default_variation;
	v = default_variation;
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_VARIATION_COMMAND_CLASS(Save, "save",
	"file : save rule variation to file")
int
Save::main(variation& v, const string_list& args) {
if (args.size() != 2) {
	cerr << "usage: " << name << " filename" << endl;
	return CommandStatus::SYNTAX;
} else {
	ofstream ofs(args.back().c_str());
	if (ofs) {
		v.save(ofs);
	} else {
		cerr << "error: couldn't open file for writing" << endl;
		return CommandStatus::BADARG;
	}
	return CommandStatus::NORMAL;
}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_VARIATION_COMMAND_CLASS(Load, "load",
	"file : load rule variations from file")
int
Load::main(variation& v, const string_list& args) {
if (args.size() != 2) {
	cerr << "usage: " << name << " filename" << endl;
	return CommandStatus::SYNTAX;
} else {
	ifstream ifs(args.back().c_str());
	if (ifs) {
		string hdr;
		getline(ifs, hdr);
		if (hdr == "BEGIN-variation") {
			v.load(ifs);
		} else {
			cerr <<
			"error: bad-file, expecting \"BEGIN-variation\""
				<< endl;
			return CommandStatus::BADARG;
		}
	} else {
		cerr << "error: couldn't open file for reading" << endl;
		return CommandStatus::BADARG;
	}
	return CommandStatus::NORMAL;
}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_PRINTER_MAP(variation)

#define	DEFINE_VARIATION_MEMBER_COMMAND(mem, str, desc)			\
	DEFINE_GENERIC_OPTION_MEMBER_COMMAND(variation, mem, str, desc)

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// game play
DEFINE_VARIATION_MEMBER_COMMAND(
	num_decks, "num-decks", "[int]: number of decks")
DEFINE_VARIATION_MEMBER_COMMAND(maximum_penetration,
	"max-penetration", "[real]: maximum penetration before reshuffling")

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DEFINE_VARIATION_MEMBER_COMMAND(H17, "H17", "[bool]: dealer hits on soft-17")

// surrender
DEFINE_VARIATION_MEMBER_COMMAND(
	surrender_early, "surrender-early",
	"[bool]: allow early (pre-peek) surrender")
DEFINE_VARIATION_MEMBER_COMMAND(
	surrender_late, "surrender-late",
	"[bool]: allow late (post-peek) surrender")
DEFINE_VARIATION_MEMBER_COMMAND(
	surrender_penalty, "surrender-penalty",
	"[real]: surrender penalty")

// peek
DEFINE_VARIATION_MEMBER_COMMAND(
	peek_on_10, "peek-10",
	"[bool]: dealer peeks on 10 for blackjack")
DEFINE_VARIATION_MEMBER_COMMAND(
	peek_on_Ace, "peek-ace",
	"[bool]: dealer peeks on Ace for blackjack")

// ties
DEFINE_VARIATION_MEMBER_COMMAND(
	ties_lose, "ties-lose", "[bool]: player loses ties")
DEFINE_VARIATION_MEMBER_COMMAND(
	ties_win, "ties-win", "[bool]: player wins ties")
DEFINE_VARIATION_MEMBER_COMMAND(
	push22, "push-22", "[bool]: dealer pushes on 22")

// double-downs
DEFINE_VARIATION_MEMBER_COMMAND(
	double_H9, "double-9", "[bool]: allow player double-down on 9")
DEFINE_VARIATION_MEMBER_COMMAND(
	double_H10, "double-10", "[bool]: allow player double-down on 10")
DEFINE_VARIATION_MEMBER_COMMAND(
	double_H11, "double-11", "[bool]: allow player double-down on 11")
DEFINE_VARIATION_MEMBER_COMMAND(
	double_after_split, "DAS", "[bool]: player double-down after split")
DEFINE_VARIATION_MEMBER_COMMAND(
	double_multiplier, "double-multiplier", "[real]: double-down cost")

// splitting
#if IMPROVED_SPLIT_ANALYSIS
DEFINE_VARIATION_MEMBER_COMMAND(
	max_split_hands, "max-split-hands", "[int]: allow split up to N hands")
#else
DEFINE_VARIATION_MEMBER_COMMAND(
	split, "split", "[bool]: allow player split")
DEFINE_VARIATION_MEMBER_COMMAND(
	resplit, "resplit", "[bool]: allow player more than one split")
#endif
DEFINE_VARIATION_MEMBER_COMMAND(
	hit_split_aces,
	"hit-split-aces", "[bool]: allow player hit on split Aces")
DEFINE_VARIATION_MEMBER_COMMAND(
	resplit_aces,
	"resplit-aces", "[bool]: allow player re-split Aces")

// payoffs and penalties
DEFINE_VARIATION_MEMBER_COMMAND(
	bj_payoff, "blackjack-pays", "[real]: player blackjack payout")

#undef	DECLARE_VARIATION_COMMAND_CLASS
}	// end namespace variation_commands

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
variation::save(ostream& o) const {
	o << "BEGIN-variation" << endl;
	variation_commands::variation_printer_map_type::const_iterator
		i(variation_commands::variation_member_var_names.begin()),
		e(variation_commands::variation_member_var_names.end());
	for ( ; i!=e; ++i) {
		o << i->first << " ";
		(*i->second)(o, *this);
		o << endl;
	}
	o << "END-variation" << endl;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
variation::load(istream& i) {
//	variation_command_registry::source(i);
	// already read in a line that says "BEGIN-variation"
	string line;
	while (i) {
		getline(i, line);
		if (i) {
			if (line == "END-variation") {
				break;
			} else {
				string_list cmd;
				tokenize(line, cmd);
				variation_command_registry::execute(*this, cmd);
			}
		}
	}
	if (i.bad()) {
		cerr << "error: EOF reached before seeing \"END-variation\""
			<< endl;
	}
}

//=============================================================================
}	// end namespace blackjack

