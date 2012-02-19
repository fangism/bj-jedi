/**
	\file "util/command.tcc"
	$Id: command.tcc,v 1.2 2012/02/19 21:37:52 fang Exp $
 */

#ifndef	__UTIL_COMMAND_TCC__
#define	__UTIL_COMMAND_TCC__

#include <iostream>
#include <iterator>
#include "util/command.hh"
#include "util/readline.h"
#include "util/readline_wrap.hh"
#include "util/string.hh"

namespace util {
using std::cout;
using std::cerr;
using std::endl;
using strings::eat_whitespace;

//=============================================================================
// class Command method definitions

template <class State>
int
Command<State>::main(state_type& s, const string_list& args) const {
        if (_main) {
                return (*_main)(s, args);
        } else {
                cerr << "command is undefined." << endl;
                return CommandStatus::UNKNOWN;
        }
}

//=============================================================================
// class command_registry function definitions, globals

template <class Cmd>
typename command_registry<Cmd>::command_map_type
command_registry<Cmd>::command_map;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <class Cmd>
typename command_registry<Cmd>::history_type
command_registry<Cmd>::history;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <class Cmd>
string
command_registry<Cmd>::prompt;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <class Cmd>
template <class C>
size_t
command_registry<Cmd>::register_command(void) {
	typedef	C		command_class;
	const Cmd temp(command_class::name, command_class::brief, 
		&command_class::main);	// no usage, just brief help
	typedef	typename command_map_type::mapped_type	mapped_type;
	typedef	typename command_map_type::value_type	value_type;
	typedef	typename command_map_type::iterator	iterator;
	const string& s(command_class::name);
	const std::pair<iterator, bool>
		p(command_map.insert(value_type(s, temp)));
	if (!p.second) {
		cerr << "command \'" << s << "\' has already been "
			"registered globally." << endl;
		throw std::exception();
	}
	// else was properly inserted
	return command_map.size();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <class Cmd>
void
command_registry<Cmd>::list_commands(ostream& o) {
	typedef typename command_map_type::const_iterator      const_iterator;
	o << "Available commands: " << endl;
	const_iterator i(command_map.begin());
	const const_iterator e(command_map.end());
	for ( ; i!=e; ++i) {
		o << i->second.name() << ": " << i->second.brief() << endl;
	}               
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	For now, is always interactive.
 */
template <class Cmd>
int
command_registry<Cmd>::interpret(state_type& s) {
	readline_wrapper rl(prompt.c_str());
	// do NOT delete this line string, it is already managed.
	const char* line = NULL;
	int status = CommandStatus::NORMAL;
	size_t lineno = 0;
	do {
		++lineno;
		line = rl.gets();       // already eaten leading whitespace
		// GOTCHA: readline eats '\t' characters!?
	if (line) {
		status = interpret_line(s, line);
		if (status != CommandStatus::NORMAL &&
				status != CommandStatus::END) {
			cerr << "error at line " << lineno << endl;
		}
	}
	} while (line && (status != CommandStatus::END));
	// end-line for neatness
	if (!line)      cout << endl;
#if 0
	// not sure if the following is good idea
	if (status && !interactive) {
		// useful for tracing errors in sourced files
		cerr << "Error detected at line " << lineno <<
			", aborting commands." << endl;
		return status;
	} else  
#endif
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <class Cmd>
int
command_registry<Cmd>::interpret_line(state_type& s, const string& line) {
	const char* cursor = line.c_str();
	if (*cursor == '!') {
		++cursor;
		eat_whitespace(cursor);
//		if (record) {
			history.push_back(string("!") +cursor);
//		}
		const int es = system(cursor);
		if (es) {
			cerr << "*** Exit " << es << endl;
		}
		return CommandStatus::NORMAL;
	} else {
		string_list toks;
		tokenize(line, toks);
		history.push_back(line);
		if (!toks.empty()) {
			return execute(s, toks);
		}
	}
	return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <class Cmd>
int
command_registry<Cmd>::execute(state_type& st, const string_list& args) {
	const size_t s = args.size();
if (s) {
	const string_list::const_iterator argi(args.begin());
	const string& cmd(*argi);
	if (!cmd.size() || cmd[0] == '#') {
		// ignore comments
		return CommandStatus::NORMAL;
	}
	typedef	typename command_map_type::const_iterator	const_iterator;
	const const_iterator p(command_map.find(cmd));
	if (p != command_map.end()) {
		// not echoing commands
		return p->second.main(st, args);
	} else {
		string cmdp(cmd);
		++cmdp[cmdp.size() -1];	// bump the last character for prefix matching
		const const_iterator l(command_map.lower_bound(cmd));
		const const_iterator u(command_map.upper_bound(cmdp));
		const size_t d = std::distance(l, u);
		if (d == 1) {
			cout << "unique-matched: " << l->first << endl;
			return l->second.main(st, args);
		} else {
			cerr << "Unknown command: " << cmd << endl;
			return CommandStatus::SYNTAX;
		}
	}
} else  return CommandStatus::NORMAL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <class Cmd>
bool
command_registry<Cmd>::help_command(ostream& o, const string& c) {
	typedef typename command_map_type::const_iterator       const_iterator;
	const_iterator probe(command_map.find(c));
	if (probe != command_map.end()) { 
		o << probe->second.name() << ": " << probe->second.brief();
		return true;
	} else  return false;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <class Cmd>
char*
command_registry<Cmd>::command_generator(const char* _text, int state) {
	typedef typename command_map_type::const_iterator       const_iterator;
	static const_iterator match_begin, match_end;
	assert(_text);
	if (!state) {
	if (_text[0]) {
		const string text(_text);
		string next(text);
		++next[next.length() -1];
		// for lexicographical bounding
		match_begin = command_map.lower_bound(text),
		match_end = command_map.lower_bound(next);
	} else {
		// empty string, return all commands
		match_begin = command_map.begin();
		match_end = command_map.end();
	}
	}
	if (match_begin != match_end) {
		const const_iterator i(match_begin);
		++match_begin;
		return strdup(i->first.c_str());        // malloc
	}       // match_begin == match_end
	return NULL;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <class Command>
char**
command_registry<Command>::completion(const char* text, int start, int) {
	typedef	typename command_map_type::const_iterator	const_iterator;
#ifdef  USE_READLINE
	// rl_attempted_completion_over = true;
	// don't fallback to readline's default completer even 
	// if this returns no matches

	// restore some default that may have been overridden
	rl_completion_append_character = ' ';
	rl_completion_display_matches_hook = NULL;

	// TODO: use rl_line_buffer to parse entire line
	// use tokenize
	// eat leading whitespace
	const char* buf = rl_line_buffer;
	eat_whitespace(buf);
	if (!start || (rl_line_buffer +start == buf)) {
		// beginning-of-line or leading whitespace: complete command
		return rl_completion_matches(text, command_generator);
	} else if (rl_line_buffer +start != buf) {
		// then we have at least one whole token for the command
		// attempt command-specific completion
		string_list toks;
		tokenize(buf, toks);
		const string& key(toks.front());
		if (key[0] == '!') {
			// shell command, fallback to filename completion
			return NULL;
		}
		const const_iterator f(command_map.find(key));
		if (f == command_map.end()) {
			// invalid command
			cerr << "\nNo such command: " << key << endl;
			return NULL;
		}
		rl_completion_append_character = '\0';
#ifdef  HAVE_BSDEDITLINE
#define VOID_FUNCTION_CAST(x)           reinterpret_cast<void (*)(void)>(x)
#else
// editline completely fubar'd this type
#define VOID_FUNCTION_CAST(x)           x
#endif
#if 0
		rl_completion_display_matches_hook =
		VOID_FUNCTION_CAST(display_hierarchical_matches_hook);
#endif
#undef  VOID_FUCNTION_CAST
		return rl_completion_matches(text, &command_generator);
	}
#endif
	return NULL;
}


//=============================================================================
#if 0
// class command_registry::readline_init method definitions

template <class Cmd>
command_registry<Cmd>::readline_init::readline_init() :
		_compl(rl_attempted_completion_function, completion) {
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <class Cmd>
command_registry<Cmd>::readline_init::~readline_init() { }
#endif

//=============================================================================
}	// end namespace util

#endif	// __UTIL_COMMAND_TCC__
