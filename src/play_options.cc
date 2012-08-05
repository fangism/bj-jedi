// "play_options.cc"

#include <iostream>
#include <fstream>
#include "play_options.hh"

#include "util/configure_option.hh"
#include "util/command.tcc"
#include "util/value_saver.hh"

namespace blackjack {
typedef	util::Command<play_options>		OptionsCommand;
}
namespace util {
template class command_registry<blackjack::OptionsCommand>;
}
namespace blackjack {
typedef	util::command_registry<OptionsCommand>		option_command_registry;

using std::map;
using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::ifstream;
using std::ofstream;
using std::getline;
using util::value_saver;
using util::Command;
using util::CommandStatus;
using util::string_list;
using util::tokenize;
using util::yn;
using util::strings::string_to_num;

//=============================================================================
// class play_options method definitions

play_options::play_options() :
	continuous_shuffle(false),
	pick_cards(false),
	use_dynamic_strategy(true), 
	use_exact_strategy(false), 
	dealer_plays_only_against_live(true),
	always_show_status(true),
	always_show_dynamic_edge(false),
	always_show_count_at_action(false),
	always_show_count_at_hand(false),
	always_suggest(false),
	notify_when_wrong(true),
	notify_dynamic(true),
	notify_exact(true),
	notify_with_count(true),
	show_edges(true),
	auto_play(false),
	face_cards(true),
	count_face_cards(false),
	count_used_cards(true),
	bookmark_wrong(true),
	bookmark_dynamic(true),
	bookmark_exact(true),
	quiz_count_before_shuffle(true),
	quiz_count_frequency(5),
	bet(1.0) {
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream&
play_options::dump(ostream& o) const {
	o << "face cards: " << yn(face_cards) << endl;
	o << "count face cards separately: " << yn(count_face_cards) << endl;
	o << "show used cards count: " << yn(count_used_cards) << endl;
	o << "continuous shuffle: " << yn(continuous_shuffle) << endl;
	o << "user picks cards: " << yn(pick_cards) << endl;
	o << "use dynamic strategy: " << yn(use_dynamic_strategy) << endl;
	o << "use exact strategy: " << yn(use_exact_strategy) << endl;
	o << "dealer plays only vs. live player: " <<
		yn(dealer_plays_only_against_live) << endl;
	o << "always show bankroll and bet: " << yn(always_show_status) << endl;
	o << "always show dynamic edge: " << yn(always_show_dynamic_edge) << endl;
	o << "always show count before hand: " <<
		yn(always_show_count_at_hand) << endl;
	o << "always show count at action: " <<
		yn(always_show_count_at_action) << endl;
	o << "always suggest best action: " << yn(always_suggest) << endl;
	o << "notify when wrong: " << yn(notify_when_wrong) << endl;
	o << "notify when dynamic strategy beats basic: " <<
		yn(notify_dynamic) << endl;
	o << "notify when exact strategy beats basic: " <<
		yn(notify_exact) << endl;
	o << "notify with count: " << yn(notify_with_count) << endl;
	o << "show edge values with analysis: " << yn(show_edges) << endl;
	o << "automatically play optimally: " << yn(auto_play) << endl;
	o << "bookmark wrong decisions: " << yn(bookmark_wrong) << endl;
	o << "bookmark when dynamic strategy != basic: " <<
		yn(bookmark_dynamic) << endl;
	o << "bookmark when exact strategy != basic: " <<
		yn(bookmark_exact) << endl;
	o << "quiz count before reshuffle: " <<
		yn(quiz_count_before_shuffle) << endl;
	o << "quiz count every N hands: " << quiz_count_frequency << endl;
	o << "current bet: " << bet << endl;
	return o;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
play_options::configure(void) {
	cout <<
"Configuring play-time options.\n"
"Type 'help' or '?' for a list of commands." << endl;
	const value_saver<string>
		tmp1(option_command_registry::prompt, "options> ");
	option_command_registry::interpret(*this);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int
play_options::command(const string_list& cmd) {
	return option_command_registry::execute(*this, cmd);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace option_commands {

#define	DECLARE_OPTION_COMMAND_CLASS(class_name, _cmd, _brief)       \
	DECLARE_AND_INITIALIZE_COMMAND_CLASS(play_options, class_name, _cmd, _brief)

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_OPTION_COMMAND_CLASS(Help, "help",
	"[cmd] : list option command(s)")
int
Help::main(play_options&, const string_list& args) {
switch (args.size()) {
case 1:
	option_command_registry::list_commands(cout);
	cout <<
"Note: [bool] values are entered as 0 or 1\n"
"[int] and [real] values must be non-negative" << endl;
	break;
default: {
	string_list::const_iterator i(++args.begin()), e(args.end());
	for ( ; i!=e; ++i) {
		if (!option_command_registry::help_command(cout, *i)) {
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
DECLARE_OPTION_COMMAND_CLASS(Help2, "?",
	"[cmd] : list option command(s)")
int
Help2::main(play_options& v, const string_list& args) {
	return Help::main(v, args);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_OPTION_COMMAND_CLASS(ShowAll, "show-all",
	": show all rule options")
int
ShowAll::main(play_options& v, const string_list&) {
	v.dump(cout);
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_OPTION_COMMAND_CLASS(Exit, "exit",
	": finish and return")
int
Exit::main(play_options&, const string_list&) {
	return CommandStatus::END;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_OPTION_COMMAND_CLASS(Done, "done",
	": finish and return")
int
Done::main(play_options&, const string_list&) {
	return CommandStatus::END;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_OPTION_COMMAND_CLASS(Reset, "reset",
	": reset to default play options")
int
Reset::main(play_options& p, const string_list&) {
	static const play_options default_options;
	p = default_options;
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_OPTION_COMMAND_CLASS(Done2, "quit",
	": finish and return")
int
Done2::main(play_options&, const string_list&) {
	return CommandStatus::END;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_OPTION_COMMAND_CLASS(Save, "save",
	"file : save play options to file")
int
Save::main(play_options& v, const string_list& args) {
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
DECLARE_OPTION_COMMAND_CLASS(Load, "load",
	"file : load play options from file")
int
Load::main(play_options& v, const string_list& args) {
if (args.size() != 2) {
	cerr << "usage: " << name << " filename" << endl;
	return CommandStatus::SYNTAX;
} else {
	ifstream ifs(args.back().c_str());
	if (ifs) {
		string hdr;
		getline(ifs, hdr);
		if (hdr == "BEGIN-options") {
			v.load(ifs);
		} else {
			cerr <<
			"error: bad-file, expecting \"BEGIN-options\""
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
DECLARE_OPTION_COMMAND_CLASS(Precision, "precision",
	": set numeric output precision")
int
Precision::main(play_options&, const string_list& args) {
if (args.size() != 2) {
	cerr << "usage: " << name << " digits" << endl;
	cerr << "current precision: " << cout.precision() << endl;
	return CommandStatus::SYNTAX;
} else {
	size_t s;
	if (string_to_num(args.back(), s)) {
		cerr << "Error parsing integer (digits)." << endl;
		return CommandStatus::BADARG;
	} else {
		cout.precision(s);
	}
	return CommandStatus::NORMAL;
}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_PRINTER_MAP(play_options)

#define DEFINE_OPTION_MEMBER_COMMAND(mem, str, desc)			\
        DEFINE_GENERIC_OPTION_MEMBER_COMMAND(play_options, mem, str, desc)

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DEFINE_OPTION_MEMBER_COMMAND(
	face_cards, "face-cards", "[bool]: show face cards symbols (not just 'T')")

DEFINE_OPTION_MEMBER_COMMAND(
	count_face_cards, "count-face-cards", "[bool]: count face cards separately")

DEFINE_OPTION_MEMBER_COMMAND(
	count_used_cards, "count-used-cards", "[bool]: show used card counts")

DEFINE_OPTION_MEMBER_COMMAND(
	pick_cards, "cards-pick", "[bool]: user chooses card or randomly draw")

DEFINE_OPTION_MEMBER_COMMAND(
	continuous_shuffle, "continuous-shuffle",
	"[bool]: re-shuffle after every hand")

DEFINE_OPTION_MEMBER_COMMAND(
	use_dynamic_strategy, "use-dynamic-strategy",
	"[bool]: advise and grade using count-based strategy")

DEFINE_OPTION_MEMBER_COMMAND(
	use_exact_strategy, "use-exact-strategy",
	"[bool]: advise and grade using count-based strategy")

DEFINE_OPTION_MEMBER_COMMAND(
	dealer_plays_only_against_live, "dealer-only-plays-live",
	"[bool]: dealer plays only when player is live")

DEFINE_OPTION_MEMBER_COMMAND(
	always_show_status, "always-show-status",
	"[bool]: show bankroll and bet between each hand")

DEFINE_OPTION_MEMBER_COMMAND(
	always_show_dynamic_edge, "always-show-dynamic-edge",
	"[bool]: show dynamic edge between each hand")

DEFINE_OPTION_MEMBER_COMMAND(
	always_show_count_at_hand, "always-show-count-before-hand",
	"[bool]: show count between each hand")

DEFINE_OPTION_MEMBER_COMMAND(
	always_show_count_at_action, "always-show-count-before-action",
	"[bool]: show count before each action")

DEFINE_OPTION_MEMBER_COMMAND(
	always_suggest, "always-suggest",
	"[bool]: suggest optimal action at each prompt")

DEFINE_OPTION_MEMBER_COMMAND(
	notify_when_wrong, "notify-when-wrong",
	"[bool]: alert player when choice was sub-optimal")

DEFINE_OPTION_MEMBER_COMMAND(
	notify_dynamic, "notify-dynamic",
	"[bool]: alert player when dynamic strategy beats basic")

DEFINE_OPTION_MEMBER_COMMAND(
	notify_exact, "notify-exact",
	"[bool]: alert player when exact strategy beats basic")

DEFINE_OPTION_MEMBER_COMMAND(
	notify_with_count, "notify-with-count",
	"[bool]: show count when notifying about strategy")

DEFINE_OPTION_MEMBER_COMMAND(
	show_edges, "show-edges",
	"[bool]: show mathematical edges with analysis")

DEFINE_OPTION_MEMBER_COMMAND(
	auto_play, "auto-play",
	"[bool]: automatically play optimally")

DEFINE_OPTION_MEMBER_COMMAND(
	bookmark_wrong, "bookmark-wrong",
	"[bool]: bookmark when user is wrong")

DEFINE_OPTION_MEMBER_COMMAND(
	bookmark_dynamic, "bookmark-dynamic",
	"[bool]: bookmark when dynamic strategy beats basic strategy")

DEFINE_OPTION_MEMBER_COMMAND(
	bookmark_exact, "bookmark-exact",
	"[bool]: bookmark when exact strategy beats basic strategy")

DEFINE_OPTION_MEMBER_COMMAND(
	quiz_count_before_shuffle, "quiz-count-before-shuffle",
	"[bool]: quiz player on hi-lo count before reshuffling")

DEFINE_OPTION_MEMBER_COMMAND(
	quiz_count_frequency, "quiz-count-frequency",
	"[int]: quiz player on hi-lo count after N hands")

#undef	DECLARE_OPTION_COMMAND_CLASS
}	// end namespace option_commands

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
play_options::save(ostream& o) const {
	o << "BEGIN-options" << endl;
	option_commands::play_options_printer_map_type::const_iterator
		i(option_commands::play_options_member_var_names.begin()),
		e(option_commands::play_options_member_var_names.end());
	for ( ; i!=e; ++i) {
		o << i->first << " ";
		(*i->second)(o, *this);
		o << endl;
	}
	o << "END-options" << endl;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
play_options::load(istream& i) {
//      option_command_registry::source(i);
	// already read in a line that says "BEGIN-options"
	string line;
	while (i) {
		getline(i, line);
		if (i) {
			if (line == "END-options") {
				break;
			} else {
				string_list cmd;
				tokenize(line, cmd);
				option_command_registry::execute(*this, cmd);
			}
		}
	}
	if (i.bad()) {
		cerr << "error: EOF reached before seeing \"END-options\""
			<< endl;
	}
}

//=============================================================================
}	// end namespace blackjack

