// "variation.cc"

#include <iostream>
#include <fstream>
#include <map>
#include "variation.hh"

#include "util/string.tcc"	// for string_to_num
#include "util/tokenize.hh"
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
using util::strings::string_to_num;
using util::tokenize;

typedef	util::command_registry<VariationCommand>
						variation_command_registry;

//-----------------------------------------------------------------------------
// class variation method definitions

static
const char*
yn(const bool y) {
	return y ? "yes" : "no";
}

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
	o << "player may split: " << yn(split) << endl;
	o << "player may re-split: " << yn(resplit) << endl;
	o << "player receives only one card on each split ace: "
		<< yn(one_card_on_split_aces) << endl;
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
		tmp1(variation_command_registry::prompt, "rules> ");
	const value_saver<util::completion_function_ptr>
		tmp2(rl_attempted_completion_function,
			&variation_command_registry::completion);
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
DECLARE_VARIATION_COMMAND_CLASS(Help, "help", ": list all variation commands")
int
Help::main(variation&, const string_list&) {
	variation_command_registry::list_commands(cout);
	cout <<
"Note: [bool] values are entered as 0 or 1\n"
"[int] and [real] values must be non-negative" << endl;
	return CommandStatus::NORMAL;
}
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_VARIATION_COMMAND_CLASS(Help2, "?", ": list all variation commands")
int
Help2::main(variation&, const string_list&) {
	variation_command_registry::list_commands(cout);
	return CommandStatus::NORMAL;
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
DECLARE_VARIATION_COMMAND_CLASS(Done, "done",
	": finish and return")
int
Done::main(variation&, const string_list&) {
	return CommandStatus::END;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static
int
configure_value(variation& v, const string_list& args,
		const char* name, bool variation::*mem) {
switch (args.size()) {
case 1: cout << name << " = " << yn(v.*mem) << endl; break;
default: {
	if (string_to_num(args.back(), v.*mem)) {
		cerr << "Error: invalid boolean value, expecting 0 or 1" << endl;
		return CommandStatus::BADARG;
	}
}
}
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static
int
configure_value(variation& v, const string_list& args,
		const char* name, size_t variation::*mem) {
switch (args.size()) {
case 1: cout << name << " = " << v.*mem << endl; break;
default: {
	if (string_to_num(args.back(), v.*mem)) {
		cerr << "Error: invalid integer value" << endl;
		return CommandStatus::BADARG;
	}
}
}
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static
int
configure_value(variation& v, const string_list& args,
		const char* name, double variation::*mem) {
switch (args.size()) {
case 1: cout << name << " = " << v.*mem << endl; break;
default: {
	double t;
	if (string_to_num(args.back(), t)) {
		cerr << "Error: invalid real value." << endl;
		return CommandStatus::BADARG;
	}
	if (t < 0.0) {
		cerr << "Error: value must be >= 0.0" << endl;
		return CommandStatus::BADARG;
	}
	v.*mem = t;
}
}
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
typedef void (*printer_fun_ptr)(ostream&, const variation&);

static
map<string, printer_fun_ptr>
member_var_names;

static
int
add_var_name(const string& s, const printer_fun_ptr p) {
	member_var_names[s] = p;
	return member_var_names.size();
}

#define	DEFINE_VARIATION_MEMBER_COMMAND(mem, str, desc)			\
DECLARE_VARIATION_COMMAND_CLASS(mem, str, desc)				\
int									\
mem::main(variation& v, const string_list& args) {			\
	return configure_value(v, args, name, &variation::mem);		\
}									\
static void								\
__printer_ ## mem (ostream& o, const variation& v) {			\
	o << v.mem;							\
}									\
static									\
int __init_name_ ## mem = add_var_name(str, & __printer_ ## mem );
// __ATTRIBUTE_UNUSED__

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
DEFINE_VARIATION_MEMBER_COMMAND(
	split, "split", "[bool]: allow player split")
DEFINE_VARIATION_MEMBER_COMMAND(
	resplit, "resplit", "[bool]: allow player more than one split")
DEFINE_VARIATION_MEMBER_COMMAND(
	one_card_on_split_aces,
	"spit-aces-one-card", "[bool]: split Aces get only one card")

// payoffs and penalties
DEFINE_VARIATION_MEMBER_COMMAND(
	bj_payoff, "blackjack-pays", "[real]: player blackjack payout")

#undef	DECLARE_VARIATION_COMMAND_CLASS
}	// end namespace variation_commands

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
variation::save(ostream& o) const {
	o << "BEGIN-variation" << endl;
	map<string, variation_commands::printer_fun_ptr>::const_iterator
		i(variation_commands::member_var_names.begin()),
		e(variation_commands::member_var_names.end());
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

