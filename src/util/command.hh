/**
	\file "util/command.hh"
	$Id: command.hh,v 1.2 2012/02/20 23:08:14 fang Exp $
 */

#ifndef	__UTIL_COMMAND_HH__
#define	__UTIL_COMMAND_HH__

#include <iosfwd>
#include <string>
#include <map>
#include <vector>
#include "util/tokenize.hh"
#include "util/value_saver.hh"

//=============================================================================
// useful macros

#define	DECLARE_COMMAND_CLASS(state_type, class_name)			\
struct class_name {							\
public:									\
	static const char		name[];				\
	static const char		brief[];			\
	static int		main(state_type&, const string_list&);	\
private:								\
	static const size_t		receipt_id;			\
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#define	INITIALIZE_COMMAND_CLASS(_state_type, _class, _cmd, _brief)	\
const char _class::name[] = _cmd;					\
const char _class::brief[] = _brief;					\
const size_t _class::receipt_id =					\
util::command_registry<Command<_state_type> >::register_command<_class >();

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#define DECLARE_AND_INITIALIZE_COMMAND_CLASS(_state, _class, _cmd, _brief) \
        DECLARE_COMMAND_CLASS(_state, _class)				\
        INITIALIZE_COMMAND_CLASS(_state, _class, _cmd, _brief)

//-----------------------------------------------------------------------------
namespace util {
using std::string;
using std::ostream;

/**
	Enumeration of command statuses
 */
struct CommandStatus {
enum {
	FATAL = -5,	///< terminate immediately (e.g. assert fail)
	INTERACT = -4,	///< open a sub-shell to inspect state
	BADFILE = -3,	///< source file not found
	SYNTAX = -2,	///< bad syntax
	UNKNOWN = -1,	///< unknown command
	NORMAL = 0,	///< command executed fine
	BADARG = 1,	///< other error with input
	END = 0xFF	///< normal exit, such as EOF
};      // end enum CommandStatus
};      // end struct command_error_codes

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	Command class, used by registry.
 */
template <class State>
class Command {
	typedef Command<State>          this_type;
public:
	typedef State                   state_type;
	typedef int (main_type) (state_type&, const string_list&);
	typedef main_type*              main_ptr_type;
private:
	/// command-key
	string				_name;
	/// help description
	string				_brief;
	/// command execution
	main_ptr_type                   _main;
public:
	Command();

	Command(const string& _n, const string& _b,
		const main_ptr_type m = NULL) :
		_name(_n), _brief(_b), _main(m) { }

	// default copy-ctor

	~Command() { }

	operator bool () const { return this->_main; }

	const string&
	name(void) const { return _name; }

	const string&
	brief(void) const { return _brief; }

	int
	main(state_type&, const string_list&) const;

};      // end class Command

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	TODO: make this non-static only.
	Allow for multiple interpreter instances?
 */
template <class Cmd>
class command_registry {
	typedef	command_registry<Cmd>		this_type;
public:
	typedef	Cmd				command_type;
	typedef	typename command_type::state_type	state_type;
private:
	typedef	std::map<string, command_type>	command_map_type;
	typedef	typename command_map_type::const_iterator
						command_iterator;
public:
	typedef	typename command_type::main_ptr_type	main_ptr_type;
//	typedef	typename command_type::usage_ptr_type	usage_ptr_type;

	class readline_init {
	// type is util::completion_function_ptr from readline_wrap.hh
		value_saver<char** (*)(const char*, int, int)>
						_compl;
	public:
		readline_init();
		~readline_init();
	} __ATTRIBUTE_UNUSED__;
private:
	static	command_map_type		command_map;
	typedef	std::vector<string>		history_type;
	static	history_type			history;
public:
	static	string				prompt;
private:
	command_registry();
	command_registry(const this_type&);
public:
	template <class C>
	static
	size_t
	register_command(void);

	static
	void
	list_commands(ostream&);

	static
	int
	interpret(state_type&);

#if 0
	static
	int
	interpret_stdin(state_type&, istream&);
#endif

	static
	int
	interpret_line(state_type&, const string&);

#if 0
	static
	int
	source(state_type&, const string&);
#endif

	static
	int
	execute(state_type&, const string_list&);

	static
	bool
	help_command(ostream&, const string&);

	static
	char**
	completion(const char*, int, int);

private:
#if 0
	static
	int
	__source(istream&, state_type&);
#endif

	static
	char*
	command_generator(const char*, int);

};	// end class command_registry

}	// end namespace util

#endif	// __UTIL_COMMAND_HH__
